/* SPDX-License-Identifier: MIT
 * Software X11 -> Linux framebuffer bridge for Zorix portable Live mode.
 * 1.3.0 keeps bounded software rendering but raises the default interaction
 * rate to 20 FPS. --fps allows a guarded 10..30 FPS tuning range.
 */
#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static volatile sig_atomic_t stop;
static void stopped(int s){(void)s;stop=1;}
static void mark_ready(const char *path){ if(!path||!*path)return; int rfd=open(path,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0644); if(rfd>=0){ssize_t wr=write(rfd,"ready\n",6);(void)wr;close(rfd);} }
int main(int argc,char**argv){
 const char *ready_file=NULL; int size_only=0,fps=20;
 for(int i=1;i<argc;i++){ if(!strcmp(argv[i],"--size"))size_only=1; else if(!strcmp(argv[i],"--ready-file")&&i+1<argc)ready_file=argv[++i]; else if(!strcmp(argv[i],"--fps")&&i+1<argc){fps=atoi(argv[++i]);if(fps<10||fps>30){fputs("fps must be between 10 and 30\n",stderr);return 64;}} else {fprintf(stderr,"usage: %s [--size] [--ready-file PATH] [--fps 10..30]\n",argv[0]);return 64;} }
 int fd=open("/dev/fb0",O_RDWR|O_CLOEXEC);if(fd<0){perror("/dev/fb0");return 1;}
 struct fb_fix_screeninfo fix;struct fb_var_screeninfo var;
 if(ioctl(fd,FBIOGET_FSCREENINFO,&fix)||ioctl(fd,FBIOGET_VSCREENINFO,&var)){perror("framebuffer info");return 2;}
 if(size_only){printf("%ux%u\n",var.xres,var.yres);close(fd);return 0;}
 if(!var.xres||!var.yres||var.red.length>8||var.green.length>8||var.blue.length>8||var.xres>7680||var.yres>4320||(var.bits_per_pixel!=32&&var.bits_per_pixel!=16&&var.bits_per_pixel!=24)){fputs("Unsupported framebuffer format\n",stderr);return 3;}
 unsigned char*map=mmap(NULL,fix.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(map==MAP_FAILED){perror("mmap fb");return 4;}
 Display*d=NULL;for(int i=0;i<100&&!d;i++){d=XOpenDisplay(NULL);if(!d)usleep(100000);}if(!d){fputs("X display unavailable\n",stderr);return 5;}
 Window root=DefaultRootWindow(d);unsigned width=var.xres,height=var.yres;if(width>(unsigned)DisplayWidth(d,DefaultScreen(d)))width=DisplayWidth(d,DefaultScreen(d));if(height>(unsigned)DisplayHeight(d,DefaultScreen(d)))height=DisplayHeight(d,DefaultScreen(d));
 unsigned bytes=(var.bits_per_pixel+7)/8;
 if((unsigned long long)(var.yoffset+height-1)*fix.line_length+(var.xoffset+width)*bytes>fix.smem_len){fputs("Framebuffer bounds invalid\n",stderr);return 6;}
 signal(SIGTERM,stopped);signal(SIGINT,stopped);signal(SIGHUP,stopped);
 int first=1;
 while(!stop){XImage*image=XGetImage(d,root,0,0,width,height,AllPlanes,ZPixmap);if(!image){usleep(100000);continue;}
 for(unsigned y=0;y<height;y++){unsigned char*dst=map+(y+var.yoffset)*fix.line_length+var.xoffset*bytes;
 if(bytes==4&&image->bits_per_pixel==32&&image->byte_order==LSBFirst&&var.red.offset==16&&var.green.offset==8&&var.blue.offset==0){memcpy(dst,image->data+y*image->bytes_per_line,width*4);}else for(unsigned x=0;x<width;x++){unsigned long p=XGetPixel(image,x,y);unsigned r=(p>>16)&255,g=(p>>8)&255,b=p&255;unsigned val=((r>>(8-var.red.length))<<var.red.offset)|((g>>(8-var.green.length))<<var.green.offset)|((b>>(8-var.blue.length))<<var.blue.offset);for(unsigned j=0;j<bytes;j++)dst[x*bytes+j]=(val>>(8*j))&255;}}
 /* XGetImage excludes the server cursor. Draw the Zorix Glass pointer into
 * portable framebuffer mode. The 2-bit rows are generated from the original
 * SVG cursor: 0 transparent, 1 dark outline, 2 pearl white, 3 cyan accent. */
 static const unsigned long long zcur[32]={0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000001540ULL,0x0000000000005940ULL,0x0000000000016940ULL,0x000000000005a940ULL,0x000000000016a940ULL,0x00000000015be940ULL,0x0000000005afa940ULL,0x0000000016bea940ULL,0x000000005afaa940ULL,0x000000016beaa940ULL,0x00000005afaaa940ULL,0x00000056faaaa940ULL,0x0000016beaaaa940ULL,0x000005afaaaaa940ULL,0x000016aeaaaaa940ULL,0x00001aaaaaaaa940ULL,0x0000156aaaaaa940ULL,0x000001556aaaa940ULL,0x00000001aaaaa940ULL,0x00000005aaaaa940ULL,0x00000006aaa6a940ULL,0x00000016aa95a940ULL,0x0000011aaa916940ULL,0x0000005aaa505940ULL,0x0000016aaa401540ULL,0x0000016aa9400000ULL,0x0000015aa9000000ULL,0x00000015a5000000ULL,0x0000000154000000ULL,0x0000000054000000ULL};
 Window rr,cc;int px,py,wx,wy;unsigned mask;
 if(XQueryPointer(d,root,&rr,&cc,&px,&py,&wx,&wy,&mask)){
  for(int cy=0;cy<32;cy++)for(int cx=0;cx<32;cx++){
   unsigned code=(unsigned)((zcur[cy]>>(cx*2))&3ULL);if(!code)continue;
   int xx=px+cx-4,yy=py+cy-3;if(xx<0||yy<0||(unsigned)xx>=width||(unsigned)yy>=height)continue;
   unsigned r=code==1?7:code==2?244:80,g=code==1?17:code==2?251:231,b=code==1?29:255;
   unsigned val=((r>>(8-var.red.length))<<var.red.offset)|((g>>(8-var.green.length))<<var.green.offset)|((b>>(8-var.blue.length))<<var.blue.offset);
   unsigned char*dst=map+(yy+var.yoffset)*fix.line_length+(xx+var.xoffset)*bytes;
   for(unsigned k=0;k<bytes;k++)dst[k]=(val>>(8*k))&255;
  }
 }
 XDestroyImage(image); if(first){mark_ready(ready_file);first=0;} usleep((useconds_t)(1000000/fps));
 }
 XCloseDisplay(d);munmap(map,fix.smem_len);close(fd);return 0;
}
