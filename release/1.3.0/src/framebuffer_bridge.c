/* SPDX-License-Identifier: MIT
 * Software X11 -> Linux framebuffer bridge for Zorix portable Live mode.
 * 1.3.0 composites visible X11 windows explicitly instead of assuming that
 * XGetImage(root) contains reparented client surfaces.
 */
#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t stop;
static void stopped(int s){(void)s;stop=1;}
static int xerror(Display*d,XErrorEvent*e){(void)d;(void)e;return 0;}

static void mark_ready(const char *path){
 if(!path||!*path)return;
 int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0644);
 if(fd>=0){ssize_t wr=write(fd,"ready\n",6);(void)wr;close(fd);}
}

static unsigned long fb_value(const struct fb_var_screeninfo *v,unsigned r,unsigned g,unsigned b){
 return ((r>>(8-v->red.length))<<v->red.offset)|
        ((g>>(8-v->green.length))<<v->green.offset)|
        ((b>>(8-v->blue.length))<<v->blue.offset);
}

static void blit(XImage *im,int dx,int dy,unsigned char *map,
 const struct fb_fix_screeninfo *fix,const struct fb_var_screeninfo *var,
 unsigned width,unsigned height,unsigned bytes){
 if(!im)return;
 int sx=0,sy=0,w=im->width,h=im->height;
 if(dx<0){sx=-dx;w-=sx;dx=0;}
 if(dy<0){sy=-dy;h-=sy;dy=0;}
 if(dx+w>(int)width)w=(int)width-dx;
 if(dy+h>(int)height)h=(int)height-dy;
 if(w<=0||h<=0)return;
 if(bytes==4&&im->bits_per_pixel==32&&im->byte_order==LSBFirst&&
    var->red.offset==16&&var->green.offset==8&&var->blue.offset==0){
  for(int y=0;y<h;y++){
   unsigned char *dst=map+(dy+y+var->yoffset)*fix->line_length+(dx+var->xoffset)*bytes;
   unsigned char *src=(unsigned char*)im->data+(sy+y)*im->bytes_per_line+sx*4;
   memcpy(dst,src,(size_t)w*4);
  }
  return;
 }
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  unsigned long p=XGetPixel(im,sx+x,sy+y);
  unsigned r=(p>>16)&255,g=(p>>8)&255,b=p&255;
  unsigned long val=fb_value(var,r,g,b);
  unsigned char *dst=map+(dy+y+var->yoffset)*fix->line_length+(dx+x+var->xoffset)*bytes;
  for(unsigned k=0;k<bytes;k++)dst[k]=(unsigned char)((val>>(8*k))&255);
 }
}

static void paint_window(Display*d,Window root,Window win,int depth,int *painted,
 unsigned char *map,const struct fb_fix_screeninfo *fix,const struct fb_var_screeninfo *var,
 unsigned width,unsigned height,unsigned bytes){
 if(depth>8||*painted>=96)return;
 XWindowAttributes a;
 if(!XGetWindowAttributes(d,win,&a)||a.class!=InputOutput||a.map_state!=IsViewable)return;
 int rx=0,ry=0;Window child=None;
 if(!XTranslateCoordinates(d,win,root,0,0,&rx,&ry,&child))return;
 if(a.width>0&&a.height>0&&a.width<16384&&a.height<16384){
  XImage *im=XGetImage(d,win,0,0,(unsigned)a.width,(unsigned)a.height,AllPlanes,ZPixmap);
  if(im){blit(im,rx,ry,map,fix,var,width,height,bytes);XDestroyImage(im);(*painted)++;}
 }
 Window rr=None,parent=None,*children=NULL;unsigned n=0;
 if(XQueryTree(d,win,&rr,&parent,&children,&n)){
  for(unsigned i=0;i<n&&*painted<96;i++)
   paint_window(d,root,children[i],depth+1,painted,map,fix,var,width,height,bytes);
  if(children)XFree(children);
 }
}

static int paint_desktop(Display*d,Window root,unsigned char *map,
 const struct fb_fix_screeninfo *fix,const struct fb_var_screeninfo *var,
 unsigned width,unsigned height,unsigned bytes){
 XImage *base=XGetImage(d,root,0,0,width,height,AllPlanes,ZPixmap);
 if(!base)return 0;
 blit(base,0,0,map,fix,var,width,height,bytes);
 XDestroyImage(base);
 int painted=0;
 Window rr=None,parent=None,*children=NULL;unsigned n=0;
 if(XQueryTree(d,root,&rr,&parent,&children,&n)){
  /* XQueryTree returns children in bottom-to-top stacking order. */
  for(unsigned i=0;i<n&&painted<96;i++)
   paint_window(d,root,children[i],0,&painted,map,fix,var,width,height,bytes);
  if(children)XFree(children);
 }
 return painted;
}

int main(int argc,char**argv){
 const char *ready_file=NULL;int size_only=0,fps=20;
 for(int i=1;i<argc;i++){
  if(!strcmp(argv[i],"--size"))size_only=1;
  else if(!strcmp(argv[i],"--ready-file")&&i+1<argc)ready_file=argv[++i];
  else if(!strcmp(argv[i],"--fps")&&i+1<argc){
   fps=atoi(argv[++i]);if(fps<10||fps>30){fputs("fps must be between 10 and 30\n",stderr);return 64;}
  }else{fprintf(stderr,"usage: %s [--size] [--ready-file PATH] [--fps 10..30]\n",argv[0]);return 64;}
 }
 int fd=open("/dev/fb0",O_RDWR|O_CLOEXEC);if(fd<0){perror("/dev/fb0");return 1;}
 struct fb_fix_screeninfo fix;struct fb_var_screeninfo var;
 if(ioctl(fd,FBIOGET_FSCREENINFO,&fix)||ioctl(fd,FBIOGET_VSCREENINFO,&var)){perror("framebuffer info");close(fd);return 2;}
 if(size_only){printf("%ux%u\n",var.xres,var.yres);close(fd);return 0;}
 if(!var.xres||!var.yres||var.red.length>8||var.green.length>8||var.blue.length>8||
    var.xres>7680||var.yres>4320||(var.bits_per_pixel!=32&&var.bits_per_pixel!=16&&var.bits_per_pixel!=24)){
  fputs("Unsupported framebuffer format\n",stderr);close(fd);return 3;
 }
 unsigned char *map=mmap(NULL,fix.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
 if(map==MAP_FAILED){perror("mmap fb");close(fd);return 4;}
 XSetErrorHandler(xerror);
 Display*d=NULL;for(int i=0;i<100&&!d;i++){d=XOpenDisplay(NULL);if(!d)usleep(100000);}
 if(!d){fputs("X display unavailable\n",stderr);munmap(map,fix.smem_len);close(fd);return 5;}
 Window root=DefaultRootWindow(d);
 unsigned width=var.xres,height=var.yres;
 if(width>(unsigned)DisplayWidth(d,DefaultScreen(d)))width=(unsigned)DisplayWidth(d,DefaultScreen(d));
 if(height>(unsigned)DisplayHeight(d,DefaultScreen(d)))height=(unsigned)DisplayHeight(d,DefaultScreen(d));
 unsigned bytes=(var.bits_per_pixel+7)/8;
 if((unsigned long long)(var.yoffset+height-1)*fix.line_length+(var.xoffset+width)*bytes>fix.smem_len){
  fputs("Framebuffer bounds invalid\n",stderr);XCloseDisplay(d);munmap(map,fix.smem_len);close(fd);return 6;
 }
 signal(SIGTERM,stopped);signal(SIGINT,stopped);signal(SIGHUP,stopped);
 int first=1,last_windows=-1;
 while(!stop){
  int painted=paint_desktop(d,root,map,&fix,&var,width,height,bytes);
  if(painted!=last_windows){
   fprintf(stderr,"ZORIX_FB_WINDOWS:%d\n",painted);
   last_windows=painted;
  }
  static const unsigned long long zcur[32]={0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000001540ULL,0x0000000000005940ULL,0x0000000000016940ULL,0x000000000005a940ULL,0x000000000016a940ULL,0x00000000015be940ULL,0x0000000005afa940ULL,0x0000000016bea940ULL,0x000000005afaa940ULL,0x000000016beaa940ULL,0x00000005afaaa940ULL,0x00000056faaaa940ULL,0x0000016beaaaa940ULL,0x000005afaaaaa940ULL,0x000016aeaaaaa940ULL,0x00001aaaaaaaa940ULL,0x0000156aaaaaa940ULL,0x000001556aaaa940ULL,0x00000001aaaaa940ULL,0x00000005aaaaa940ULL,0x00000006aaa6a940ULL,0x00000016aa95a940ULL,0x0000011aaa916940ULL,0x0000005aaa505940ULL,0x0000016aaa401540ULL,0x0000016aa9400000ULL,0x0000015aa9000000ULL,0x00000015a5000000ULL,0x0000000154000000ULL,0x0000000054000000ULL};
  Window rr,cc;int px,py,wx,wy;unsigned mask;
  if(XQueryPointer(d,root,&rr,&cc,&px,&py,&wx,&wy,&mask)){
   for(int cy=0;cy<32;cy++)for(int cx=0;cx<32;cx++){
    unsigned code=(unsigned)((zcur[cy]>>(cx*2))&3ULL);if(!code)continue;
    int xx=px+cx-4,yy=py+cy-3;if(xx<0||yy<0||(unsigned)xx>=width||(unsigned)yy>=height)continue;
    unsigned r=code==1?7:code==2?244:80,g=code==1?17:code==2?251:231,b=code==1?29:255;
    unsigned long val=fb_value(&var,r,g,b);
    unsigned char *dst=map+(yy+var.yoffset)*fix.line_length+(xx+var.xoffset)*bytes;
    for(unsigned k=0;k<bytes;k++)dst[k]=(unsigned char)((val>>(8*k))&255);
   }
  }
  if(first){mark_ready(ready_file);first=0;}
  usleep((useconds_t)(1000000/fps));
 }
 XCloseDisplay(d);munmap(map,fix.smem_len);close(fd);return 0;
}
