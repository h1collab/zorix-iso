#!/usr/bin/python3
# SPDX-License-Identifier: MIT
"""Zorix boot splash: static frame + low-cost partial spinner updates."""
import argparse,fcntl,mmap,pathlib,signal,struct,time,math
from PIL import Image,ImageDraw,ImageFont
stop=False

def terminate(*_):
 global stop; stop=True

def make_base(w,h,fontpath,logopath='/usr/share/zorix/branding/zorix-logo.png'):
 im=Image.new('RGB',(w,h),(5,9,17)); d=ImageDraw.Draw(im)
 cx,cy=w//2,h//2-40
 for r,a in [(230,16),(170,22),(120,30)]:
  col=(5+a//3,18+a//2,34+a)
  d.ellipse((cx-r,cy-r,cx+r,cy+r),fill=col)
 try:
  logo=Image.open(logopath).convert('RGBA'); logo.thumbnail((142,142),Image.Resampling.LANCZOS)
  im.paste(logo,(cx-logo.width//2,cy-logo.height//2-12),logo)
 except Exception:
  d.rounded_rectangle((cx-58,cy-70,cx+58,cy+46),radius=30,fill=(18,124,183))
 try: font=ImageFont.truetype(fontpath,23)
 except Exception: font=ImageFont.load_default()
 text='Zorix'; box=d.textbbox((0,0),text,font=font)
 d.text(((w-box[2])/2,cy+86),text,font=font,fill=(237,246,252))
 return im,(cx,cy+136)

def spinner_patch(base,center,phase):
 cx,cy=center; radius=34
 left=max(0,cx-radius); top=max(0,cy-radius); right=min(base.width,cx+radius+1); bottom=min(base.height,cy+radius+1)
 patch=base.crop((left,top,right,bottom)); d=ImageDraw.Draw(patch)
 for i in range(8):
  a=phase*2.8+i*math.tau/8
  x=cx+math.cos(a)*24-left; y=cy+math.sin(a)*24-top
  strength=(i+int(phase*4))%8; v=90+strength*19
  d.ellipse((x-3,y-3,x+3,y+3),fill=(min(225,v),min(245,v+20),min(255,v+28)))
 return patch,(left,top,right,bottom)

def main():
 p=argparse.ArgumentParser();p.add_argument('--preview',type=pathlib.Path);p.add_argument('--font',default='/run/zorix-fonts/ZorixSans.ttf');p.add_argument('--logo',default='/usr/share/zorix/branding/zorix-logo.png');a=p.parse_args()
 if a.preview:
  base,center=make_base(1280,720,a.font,a.logo); patch,box=spinner_patch(base,center,.6); base.paste(patch,(box[0],box[1])); base.save(a.preview); return
 signal.signal(signal.SIGTERM,terminate); signal.signal(signal.SIGINT,terminate)
 try: fd=open('/dev/fb0','r+b',buffering=0)
 except OSError: return
 with fd:
  v=bytearray(160);f=bytearray(80);fcntl.ioctl(fd,0x4600,v,True);fcntl.ioctl(fd,0x4602,f,True)
  w,h,xv,yv,xoff,yoff,bpp,gray=struct.unpack_from('8I',v);length=struct.unpack_from('I',f,24)[0];stride=struct.unpack_from('I',f,48)[0]
  ro,rl=struct.unpack_from('II',v,32);go,gl=struct.unpack_from('II',v,44);bo,bl=struct.unpack_from('II',v,56)
  if bpp!=32 or (ro,rl,go,gl,bo,bl)!=(16,8,8,8,0,8) or not (1<=w<=7680 and 1<=h<=4320): return
  if (yoff+h-1)*stride+(xoff+w)*4>length: return
  base,center=make_base(w,h,a.font,a.logo)
  raw=base.tobytes('raw','BGRX')
  with mmap.mmap(fd.fileno(),length) as buf:
   for y in range(h):
    off=(y+yoff)*stride+xoff*4; buf[off:off+w*4]=raw[y*w*4:(y+1)*w*4]
   start=time.monotonic()
   while not stop:
    patch,box=spinner_patch(base,center,time.monotonic()-start)
    pdata=patch.tobytes('raw','BGRX'); pw=patch.width
    for row in range(patch.height):
     y=box[1]+row; off=(y+yoff)*stride+(box[0]+xoff)*4
     buf[off:off+pw*4]=pdata[row*pw*4:(row+1)*pw*4]
    time.sleep(.125)
if __name__=='__main__': main()
