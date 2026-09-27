#!/usr/bin/python3
# SPDX-License-Identifier: MIT
"""Zorix boot splash with partial updates and slow-boot stage diagnostics."""
import argparse,fcntl,mmap,pathlib,signal,struct,time,math
from PIL import Image,ImageDraw,ImageFont
stop=False
def terminate(*_):
 global stop; stop=True
def font_or_default(path,size):
 try:return ImageFont.truetype(path,size)
 except Exception:return ImageFont.load_default()
def make_base(w,h,fontpath,logopath='/usr/share/zorix/branding/zorix-logo.png'):
 im=Image.new('RGB',(w,h),(5,9,17));d=ImageDraw.Draw(im);cx,cy=w//2,h//2-40
 for r,a in [(230,16),(170,22),(120,30)]:
  col=(5+a//3,18+a//2,34+a);d.ellipse((cx-r,cy-r,cx+r,cy+r),fill=col)
 try:
  logo=Image.open(logopath).convert('RGBA');logo.thumbnail((142,142),Image.Resampling.LANCZOS);im.paste(logo,(cx-logo.width//2,cy-logo.height//2-12),logo)
 except Exception:
  d.rounded_rectangle((cx-58,cy-70,cx+58,cy+46),radius=30,fill=(18,124,183))
 font=font_or_default(fontpath,23);text='Zorix';box=d.textbbox((0,0),text,font=font);d.text(((w-box[2])/2,cy+86),text,font=font,fill=(237,246,252))
 return im,(cx,cy+136)
def spinner_patch(base,center,phase):
 cx,cy=center;radius=34;left=max(0,cx-radius);top=max(0,cy-radius);right=min(base.width,cx+radius+1);bottom=min(base.height,cy+radius+1)
 patch=base.crop((left,top,right,bottom));d=ImageDraw.Draw(patch)
 for i in range(8):
  a=phase*2.8+i*math.tau/8;x=cx+math.cos(a)*24-left;y=cy+math.sin(a)*24-top;strength=(i+int(phase*4))%8;v=90+strength*19
  d.ellipse((x-3,y-3,x+3,y+3),fill=(min(225,v),min(245,v+20),min(255,v+28)))
 return patch,(left,top,right,bottom)
def read_stage(path='/run/zorix/stage'):
 try:return pathlib.Path(path).read_text(errors='replace').strip()[:80]
 except OSError:return 'Starting Zorix'
def status_patch(base,center,fontpath,elapsed):
 cx,cy=center;width=min(560,base.width-24);height=48;left=max(0,cx-width//2);top=min(base.height-height,max(0,cy+42))
 patch=base.crop((left,top,left+width,top+height));d=ImageDraw.Draw(patch);stage=read_stage();text=f'{stage}  -  {int(elapsed)}s' if elapsed>=30 else stage
 font=font_or_default(fontpath,15);box=d.textbbox((0,0),text,font=font);d.text(((width-(box[2]-box[0]))/2,12),text,font=font,fill=(160,184,207))
 return patch,(left,top,left+width,top+height)
def write_patch(buf,stride,xoff,yoff,patch,box):
 raw=patch.tobytes('raw','BGRX');pw=patch.width
 for row in range(patch.height):
  y=box[1]+row;off=(y+yoff)*stride+(box[0]+xoff)*4;buf[off:off+pw*4]=raw[row*pw*4:(row+1)*pw*4]
def main():
 p=argparse.ArgumentParser();p.add_argument('--preview',type=pathlib.Path);p.add_argument('--font',default='/run/zorix-fonts/ZorixSans.ttf');p.add_argument('--logo',default='/usr/share/zorix/branding/zorix-logo.png');a=p.parse_args()
 if a.preview:
  base,center=make_base(1280,720,a.font,a.logo);patch,box=spinner_patch(base,center,.6);base.paste(patch,(box[0],box[1]));status,sbox=status_patch(base,center,a.font,8);base.paste(status,(sbox[0],sbox[1]));base.save(a.preview);return
 signal.signal(signal.SIGTERM,terminate);signal.signal(signal.SIGINT,terminate)
 try:fd=open('/dev/fb0','r+b',buffering=0)
 except OSError:return
 with fd:
  v=bytearray(160);f=bytearray(80);fcntl.ioctl(fd,0x4600,v,True);fcntl.ioctl(fd,0x4602,f,True)
  w,h,xv,yv,xoff,yoff,bpp,gray=struct.unpack_from('8I',v);length=struct.unpack_from('I',f,24)[0];stride=struct.unpack_from('I',f,48)[0]
  ro,rl=struct.unpack_from('II',v,32);go,gl=struct.unpack_from('II',v,44);bo,bl=struct.unpack_from('II',v,56)
  if bpp!=32 or (ro,rl,go,gl,bo,bl)!=(16,8,8,8,0,8) or not (1<=w<=7680 and 1<=h<=4320):return
  if (yoff+h-1)*stride+(xoff+w)*4>length:return
  base,center=make_base(w,h,a.font,a.logo);raw=base.tobytes('raw','BGRX')
  with mmap.mmap(fd.fileno(),length) as buf:
   for y in range(h):
    off=(y+yoff)*stride+xoff*4;buf[off:off+w*4]=raw[y*w*4:(y+1)*w*4]
   start=time.monotonic();next_status=start+3
   while not stop:
    now=time.monotonic();elapsed=now-start;patch,box=spinner_patch(base,center,elapsed);write_patch(buf,stride,xoff,yoff,patch,box)
    if now>=next_status:
     status,sbox=status_patch(base,center,a.font,elapsed);write_patch(buf,stride,xoff,yoff,status,sbox);next_status=now+1
    time.sleep(.125)
if __name__=='__main__':main()
