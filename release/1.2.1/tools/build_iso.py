#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Pack a prepared root into a real x86-64 UEFI ISO without privileged mounts.
newc+zstd initramfs, FAT32 EFI system partition, ISO9660+El Torito+MBR.
This constructs media; it is NOT a substitute for a firmware boot test.
"""
from __future__ import annotations
import argparse,calendar,hashlib,json,math,os,pathlib,shutil,stat,struct,subprocess
EPOCH=calendar.timegm((2026,9,24,12,0,0));SECTOR=512;BLOCK=2048
P=struct.pack

def digest(path):
 h=hashlib.sha256()
 with open(path,'rb') as f:
  while b:=f.read(1024*1024):h.update(b)
 return h.hexdigest()

def pack_root(root:pathlib.Path,out:pathlib.Path):
 proc=subprocess.Popen(['zstd','-T2','-10','--no-progress','-f','-o',str(out)],stdin=subprocess.PIPE)
 stream=proc.stdin;entries=[];position=0
 def write(b):
  nonlocal position;stream.write(b);position+=len(b)
 def padding():write(b'\0'*((-position)%4))
 def entry(name,mode,size=0,source=None,data=None,uid=0,gid=0,rmajor=0,rminor=0):
  ino=len(entries)+1;nm=name.encode()+b'\0'
  fields=[ino,mode,uid,gid,1,EPOCH,size,0,0,rmajor,rminor,len(nm),0]
  write(b'070701'+b''.join(f'{v:08x}'.encode() for v in fields));write(nm);padding()
  h=hashlib.sha256()
  if source:
   with open(source,'rb') as f:
    while chunk:=f.read(1024*1024):write(chunk);h.update(chunk)
  elif data is not None:write(data);h.update(data)
  padding();entries.append({'path':name,'mode':oct(mode),'size':size,'sha256':h.hexdigest() if stat.S_ISREG(mode) else None})
 entry('.',stat.S_IFDIR|0o755)
 for path in sorted(root.rglob('*'),key=lambda p:(len(p.relative_to(root).parts),str(p))):
  name=path.relative_to(root).as_posix();s=path.lstat();mode=s.st_mode
  if stat.S_ISSOCK(mode) or stat.S_ISFIFO(mode):continue
  uid=gid=1000 if name=='home/zorix' or name.startswith('home/zorix/') else 0
  if path.is_symlink():
   b=os.readlink(path).encode();entry(name,mode,len(b),data=b,uid=uid,gid=gid)
  elif path.is_dir():entry(name,mode,uid=uid,gid=gid)
  elif path.is_file():entry(name,mode,s.st_size,source=path,uid=uid,gid=gid)
  elif stat.S_ISCHR(mode):entry(name,mode,uid=uid,gid=gid,rmajor=os.major(s.st_rdev),rminor=os.minor(s.st_rdev))
 for name,major,minor,perm in [('console',5,1,0o600),('null',1,3,0o666),('zero',1,5,0o666),('urandom',1,9,0o666)]:
  if not (root/'dev'/name).exists():entry('dev/'+name,stat.S_IFCHR|perm,rmajor=major,rminor=minor)
 entry('TRAILER!!!',0);write(b'\0'*((-position)%512));stream.close()
 if proc.wait()!=0:raise RuntimeError('zstd failed')
 return {'entry_count':len(entries)-1,'uncompressed_bytes':position,'compressed_bytes':out.stat().st_size,'compressed_sha256':digest(out)},entries[:-1]

def make_fat(out,pathmap,hidden=2048):
 size=sum(p.stat().st_size for p in pathmap.values());total=max(600*1024,math.ceil((size+32*1024**2)/(1024**2))*2048)
 reserved=32;spc=8;fatsz=1
 while True:
  clusters=(total-reserved-2*fatsz)//spc;needed=math.ceil((clusters+2)*4/512)
  if needed<=fatsz:break
  fatsz=needed
 assert clusters>=65525
 fat=bytearray(fatsz*512);struct.pack_into('<II',fat,0,0x0ffffff8,0x0fffffff)
 nextcluster=2;alloc={}
 def allocate(name,nbytes):
  nonlocal nextcluster
  count=max(1,math.ceil(nbytes/4096));first=nextcluster;nextcluster+=count
  if nextcluster>clusters+2:raise RuntimeError('FAT full')
  for c in range(first,nextcluster):struct.pack_into('<I',fat,c*4,c+1 if c+1<nextcluster else 0x0fffffff)
  alloc[name]=(first,count,nbytes);return first
 for d in ['ROOT','EFI','BOOT','ZORIX']:allocate(d,4096)
 for name,path in pathmap.items():allocate(name,path.stat().st_size)
 def dirent(name,attr,cluster=0,size=0):
  if attr==8:short=name.ljust(11)
  elif name in ('.','..'):short=name.ljust(11)
  else:
   stem,_,suffix=name.partition('.');assert len(stem)<=8 and len(suffix)<=3;short=stem.ljust(8)+suffix.ljust(3)
  b=bytearray(32);b[:11]=short.encode();b[11]=attr
  date=((2026-1980)<<9)|(9<<5)|24;tm=(12<<11)
  struct.pack_into('<HHH',b,14,tm,date,date);struct.pack_into('<H',b,20,cluster>>16);struct.pack_into('<HHHI',b,22,tm,date,cluster&65535,size);return b
 boot=bytearray(512);boot[0:3]=b'\xeb\x58\x90';boot[3:11]=b'ZORIX121'
 struct.pack_into('<HBHBHHBHHHII',boot,11,512,spc,reserved,2,0,0,0xf8,0,63,255,hidden,total)
 struct.pack_into('<IHHIHH',boot,36,fatsz,0,0,alloc['ROOT'][0],1,6)
 boot[64]=0x80;boot[66]=0x29;struct.pack_into('<I',boot,67,0x5a4f0700);boot[71:82]=b'ZORIX_GLASS';boot[82:90]=b'FAT32   ';boot[510:512]=b'\x55\xaa'
 info=bytearray(512);struct.pack_into('<I',info,0,0x41615252);struct.pack_into('<III',info,484,0x61417272,clusters-(nextcluster-2),nextcluster);info[508:512]=b'\0\0\x55\xaa'
 datastart=(reserved+2*fatsz)*512
 def offset(c):return datastart+(c-2)*4096
 with out.open('wb') as f:
  f.truncate(total*512)
  for sector,b in [(0,boot),(1,info),(6,boot),(7,info)]:f.seek(sector*512);f.write(b)
  for sector in [reserved,reserved+fatsz]:f.seek(sector*512);f.write(fat)
  dirs={
   'ROOT':dirent('ZORIX_GLASS',8)+dirent('EFI',16,alloc['EFI'][0])+dirent('ZORIX',16,alloc['ZORIX'][0]),
   'EFI':dirent('.',16,alloc['EFI'][0])+dirent('..',16,0)+dirent('BOOT',16,alloc['BOOT'][0]),
   'BOOT':dirent('.',16,alloc['BOOT'][0])+dirent('..',16,alloc['EFI'][0]),
   'ZORIX':dirent('.',16,alloc['ZORIX'][0])+dirent('..',16,0)}
  for name,path in pathmap.items():
   parent='BOOT' if name=='BOOTX64.EFI' else 'ZORIX';first,count,length=alloc[name];dirs[parent]+=dirent(name,32,first,length)
   f.seek(offset(first))
   with path.open('rb') as src:shutil.copyfileobj(src,f,1024*1024)
  for name,b in dirs.items():f.seek(offset(alloc[name][0]));f.write(b+b'\0'*(4096-len(b)))
 return {'bytes':total*512,'cluster_count':clusters,'allocated_clusters':nextcluster-2,'file_hashes':{n:digest(p) for n,p in pathmap.items()}}

def make_iso(out,fat,readme,manifest):
 fatlba=512;totalbytes=fatlba*BLOCK+fat.stat().st_size;assert totalbytes%BLOCK==0;blocks=totalbytes//BLOCK
 def both16(v):return P('<H',v)+P('>H',v)
 def both32(v):return P('<I',v)+P('>I',v)
 def rec(name,lba,size,directory=False):
  n=name if isinstance(name,bytes) else name.encode();length=33+len(n)+(len(n)%2==0)
  b=bytearray(length);b[0]=length;b[2:10]=both32(lba);b[10:18]=both32(size);b[18:25]=bytes([126,9,24,12,0,0,0]);b[25]=2 if directory else 0;b[28:32]=both16(1);b[32]=len(n);b[33:33+len(n)]=n;return b
 pvd=bytearray(BLOCK);pvd[:7]=b'\x01CD001\x01';pvd[8:40]=b'ZORIX'.ljust(32);pvd[40:72]=b'ZORIX_GLASS_121'.ljust(32);pvd[80:88]=both32(blocks);pvd[120:124]=both16(1);pvd[124:128]=both16(1);pvd[128:132]=both16(BLOCK);pvd[132:140]=both32(10);struct.pack_into('<I',pvd,140,19);struct.pack_into('>I',pvd,148,20);pvd[156:190]=rec(b'\0',21,BLOCK,True)
 for start,end,value in [(190,318,b'ZORIX_GLASS_121'),(318,446,b'ZORIX EXPERIMENTAL'),(446,574,b'ZORIX OPEN SOURCE BUILD TOOLS'),(574,702,b'ZORIX GLASS LIVE 1.2.1')]:pvd[start:end]=value.ljust(end-start)
 for pos in [813,830]:pvd[pos:pos+17]=b'2026092412000000\0'
 for pos in [847,864]:pvd[pos:pos+17]=b'0000000000000000\0'
 pvd[881]=1
 boot=bytearray(BLOCK);boot[:7]=b'\0CD001\x01';boot[7:39]=b'EL TORITO SPECIFICATION'.ljust(32,b'\0');struct.pack_into('<I',boot,71,22)
 term=bytearray(BLOCK);term[:7]=b'\xffCD001\x01'
 catalog=bytearray(BLOCK);catalog[0]=1;catalog[1]=0xef;catalog[4:28]=b'ZORIX UEFI LIVE'.ljust(24);catalog[30:32]=b'\x55\xaa';struct.pack_into('<H',catalog,28,(-sum(struct.unpack('<16H',catalog[:32])))&65535)
 catalog[32]=0x88;catalog[33]=0;struct.pack_into('<H',catalog,38,0);struct.pack_into('<I',catalog,40,fatlba)
 docs=[('README.TXT;1',readme.encode()),('BUILD.JSN;1',json.dumps(manifest,indent=2).encode())]
 root=rec(b'\0',21,BLOCK,True)+rec(b'\1',21,BLOCK,True)+rec('BOOT.CAT;1',22,BLOCK)+rec('EFI.IMG;1',fatlba,fat.stat().st_size)
 lba=23;extras=[]
 for name,data in docs:root+=rec(name,lba,len(data));extras.append((lba,data));lba+=math.ceil(len(data)/BLOCK)
 if lba>=fatlba or len(root)>BLOCK:raise RuntimeError('ISO metadata area full')
 mbr=bytearray(512);mbr[446:462]=P('<B3sB3sII',0,b'\xfe\xff\xff',0xef,b'\xfe\xff\xff',fatlba*4,fat.stat().st_size//512);mbr[510:512]=b'\x55\xaa'
 with out.open('wb') as f:
  f.truncate(totalbytes);f.write(mbr)
  for i,b in [(16,pvd),(17,boot),(18,term),(19,b'\x01\0'+P('<I',21)+P('<H',1)+b'\0\0'),(20,b'\x01\0'+P('>I',21)+P('>H',1)+b'\0\0'),(21,root),(22,catalog)]+extras:f.seek(i*BLOCK);f.write(b)
  f.seek(fatlba*BLOCK)
  with fat.open('rb') as src:shutil.copyfileobj(src,f,1024*1024)
 return {'bytes':totalbytes,'sha256':digest(out),'firmware':'x86-64 UEFI only; unsigned loader','legacy_bios':False,'secure_boot_validated':False,'boot_tested':False}

def main():
 p=argparse.ArgumentParser();p.add_argument('--root',type=pathlib.Path,required=True);p.add_argument('--kernel',type=pathlib.Path,required=True);p.add_argument('--loader',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--scratch',type=pathlib.Path,required=True);p.add_argument('--docs',type=pathlib.Path,required=True);a=p.parse_args();a.scratch.mkdir(parents=True,exist_ok=True);a.docs.mkdir(parents=True,exist_ok=True)
 initrd=a.scratch/'LIVE.CPI';fat=a.scratch/'EFI.IMG'
 archive,entries=pack_root(a.root,initrd);(a.docs/'rootfs-manifest.json').write_text(json.dumps(entries,indent=2));print('INITRAMFS '+json.dumps(archive),flush=True)
 fs=make_fat(fat,{'BOOTX64.EFI':a.loader,'LINUX.EFI':a.kernel,'LIVE.CPI':initrd});print('FAT32 '+json.dumps(fs),flush=True)
 readme=(a.root/'usr/share/doc/zorix-os/README.txt').read_text()
 build={'product':'Zorix OS 1.2.1 Glass Live Experimental','kernel':'6.12.96+deb13-amd64','archive':archive,'fat32':fs,'boot_validation':'NOT PERFORMED: firmware emulator unavailable. This image is experimental.'}
 iso=make_iso(a.output,fat,readme,build);build['iso']=iso;(a.docs/'image-build.json').write_text(json.dumps(build,indent=2));print('ISO '+json.dumps(iso),flush=True)
 a.output.with_suffix('.iso.sha256').write_text(iso['sha256']+'  '+a.output.name+'\n')
if __name__=='__main__':main()
