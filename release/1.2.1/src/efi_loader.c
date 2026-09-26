/* SPDX-License-Identifier: MIT
 * Zorix Boot 1.2.1: small x86-64 UEFI loader for a Linux EFI-stub kernel.
 * Linux retains responsibility for boot services exit and machine setup.
 * This is unsigned experimental code, not a Secure Boot shim.
 */
typedef unsigned char U8; typedef unsigned short U16; typedef unsigned int U32;
typedef unsigned long long U64; typedef unsigned long long UN;
typedef U16 CHAR16; typedef U64 STATUS; typedef void *HANDLE;
#define EFIAPI __attribute__((ms_abi))
#define NULL ((void*)0)
#define ERR(n) (0x8000000000000000ULL|(n))
#define FAILED(s) (((s)>>63)!=0)
typedef struct {U32 a;U16 b,c;U8 d[8];} GUID;
typedef struct {U64 sig;U32 rev,size,crc,reserved;} HDR;
typedef struct {U8 type,sub;U16 len;} DP;
typedef struct FILE FILE;
struct FILE {U64 rev;STATUS(EFIAPI *open)(FILE*,FILE**,CHAR16*,U64,U64);STATUS(EFIAPI *close)(FILE*);void*del;STATUS(EFIAPI *read)(FILE*,UN*,void*);void*write;void*getpos;STATUS(EFIAPI *setpos)(FILE*,U64);STATUS(EFIAPI *getinfo)(FILE*,GUID*,UN*,void*);};
typedef struct {U64 rev;STATUS(EFIAPI *openvol)(void*,FILE**);} FS;
typedef struct {U32 rev;HANDLE parent;void*system;HANDLE device;DP*path;void*reserved;U32 opts_size;void*opts;void*base;U64 size;U32 code_type,data_type;void*unload;} LI;
typedef struct {U16 scan,unicode;} KEY;
typedef struct {void*reset;STATUS(EFIAPI *read)(void*,KEY*);HANDLE wait;} IN;
typedef struct {void*reset;STATUS(EFIAPI *print)(void*,CHAR16*);void*test;void*query;void*setmode;STATUS(EFIAPI *attr)(void*,UN);STATUS(EFIAPI *clear)(void*);void*cursor;void*enable;void*mode;} OUT;
typedef struct {
 HDR h;void*raise;void*restore;void*allocpages;void*freepages;void*memmap;
 STATUS(EFIAPI *alloc)(U32,UN,void**);STATUS(EFIAPI *free)(void*);
 void*createevent;void*settimer;void*waitevent;void*signal;void*closeevent;void*checkevent;
 STATUS(EFIAPI *install)(HANDLE*,GUID*,U32,void*);void*reinstall;void*uninstall;
 STATUS(EFIAPI *protocol)(HANDLE,GUID*,void**);void*reserved;void*notify;void*locatehandle;void*locatedp;void*installtable;
 STATUS(EFIAPI *load)(U8,HANDLE,DP*,void*,UN,HANDLE*);STATUS(EFIAPI *start)(HANDLE,UN*,CHAR16**);
 void*exit;void*unload;void*exitbs;void*monotonic;STATUS(EFIAPI *stall)(UN);
 STATUS(EFIAPI *watchdog)(UN,U64,UN,CHAR16*);void*connect;void*disconnect;void*openprotocol;void*closeprotocol;void*protocolinfo;void*protocols;void*locatebuffer;
 STATUS(EFIAPI *locate)(GUID*,void*,void**);void*installmulti;void*uninstallmulti;void*crc;void*copy;void*set;void*eventex;
} BS;
typedef struct {HDR h;CHAR16*vendor;U32 rev;HANDLE hin;IN*in;HANDLE hout;OUT*out;HANDLE herr;OUT*err;void*runtime;BS*bs;UN tables;void*config;} ST;
typedef struct {U32 ver,width,height,format;U32 masks[4];U32 stride;} GOPINFO;
typedef struct {U32 max,mode;GOPINFO*info;UN info_size;U64 fb;UN fbsize;} GOPMODE;
typedef struct {U8 b,g,r,a;} PIXEL;
typedef struct GOP GOP;
struct GOP {void*query;void*set;STATUS(EFIAPI *blt)(GOP*,PIXEL*,U32,UN,UN,UN,UN,UN,UN,UN);GOPMODE*mode;};
typedef struct LF2 LF2;struct LF2 {STATUS(EFIAPI *load)(LF2*,DP*,U8,UN*,void*);};
static GUID loaded_guid={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0,0xa0,0xc9,0x69,0x72,0x3b}};
static GUID fs_guid={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static GUID info_guid={0x9576e92,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static GUID dp_guid={0x9576e91,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static GUID lf_guid={0x4006c0c1,0xfcb3,0x403e,{0x99,0x6d,0x4a,0x6c,0x87,0x24,0xe0,0x6d}};
static GUID gop_guid={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
static struct __attribute__((packed)){DP vendor;GUID guid;DP end;} initrd_dp={{4,3,20},{0x5568e427,0x68fc,0x4f3d,{0xac,0x74,0xca,0x55,0x52,0x31,0xcc,0x68}},{0x7f,0xff,4}};
static BS *bs;static ST*st;static FILE*root,*initrd;static UN initrd_size;
_Static_assert(__builtin_offsetof(BS,load)==200,"EFI boot services ABI");
_Static_assert(__builtin_offsetof(BS,locate)==320,"EFI locate ABI");
_Static_assert(__builtin_offsetof(ST,bs)==96,"EFI system table ABI");
_Static_assert(__builtin_offsetof(LI,opts)==56,"EFI loaded image ABI");
static void say(CHAR16*s){if(st->out)st->out->print(st->out,s);}
static STATUS size_of(FILE*f,UN*out){U8 buf[1024];UN n=sizeof(buf);STATUS s=f->getinfo(f,&info_guid,&n,buf);if(FAILED(s))return s;if(n<16)return ERR(7);*out=(UN)*(U64*)(buf+8);return 0;}
static STATUS read_all(FILE*f,UN size,void*dest){STATUS s=f->setpos(f,0);if(FAILED(s))return s;UN done=0;while(done<size){UN n=size-done;if(n>8*1024*1024)n=8*1024*1024;s=f->read(f,&n,(U8*)dest+done);if(FAILED(s))return s;if(!n)return ERR(7);done+=n;}return 0;}
static STATUS EFIAPI provide_initrd(LF2*self,DP*path,U8 policy,UN*size,void*buffer){(void)self;if(policy||!size||!path||path->type!=0x7f||path->sub!=0xff)return ERR(2);if(!buffer||*size<initrd_size){*size=initrd_size;return ERR(5);}*size=initrd_size;return read_all(initrd,initrd_size,buffer);}
static LF2 loadfile={provide_initrd};
static void logo(void){GOP*g=NULL;if(FAILED(bs->locate(&gop_guid,NULL,(void**)&g))||!g||!g->mode||!g->mode->info)return;UN w=g->mode->info->width,h=g->mode->info->height;if(w<100||h<100||w>7680||h>4320)return;PIXEL bg={31,19,10,0};g->blt(g,&bg,0,0,0,0,0,w,h,0);UN dim=150;PIXEL *pixels=NULL;if(FAILED(bs->alloc(2,dim*dim*4,(void**)&pixels)))return;for(UN y=0;y<dim;y++){for(UN x=0;x<dim;x++){PIXEL p=bg;int inside=(x>24&&x<126&&((y>27&&y<48)||(y>102&&y<123)||(x+y>134&&x+y<158&&y>41&&y<108)));if(inside){p.r=(U8)(145+x/3);p.g=(U8)(238-y/4);p.b=(U8)(229+y/8);}pixels[y*dim+x]=p;}}g->blt(g,pixels,2,0,0,(w-dim)/2,(h-dim)/2,dim,dim,dim*4);bs->free(pixels);}
static CHAR16 normal[]=L"rdinit=/init rootfstype=ramfs console=tty0 quiet loglevel=0 vt.global_cursor_default=0 udev.log_level=0 zorix.mode=auto";
static CHAR16 compatibility[]=L"rdinit=/init rootfstype=ramfs console=tty0 quiet loglevel=0 vt.global_cursor_default=0 udev.log_level=0 nomodeset zorix.mode=portable";
static CHAR16 recovery[]=L"rdinit=/init rootfstype=ramfs console=tty0 loglevel=6 nomodeset zorix.mode=recovery";
STATUS EFIAPI efi_main(HANDLE image,ST*system){
 st=system;bs=st->bs;bs->watchdog(0,0,0,NULL);
 if(st->out){st->out->attr(st->out,0x0b);st->out->clear(st->out);}
 say(L"\r\n   Z O R I X   O S   /   LIQUID GLASS LIVE 1.2.1\r\n\r\n");
 say(L"   [1] Zorix Glass - automatic accelerated display (default)\r\n");
 say(L"   [2] Compatibility display - software fallback\r\n   [3] Recovery console\r\n\r\n");
 say(L"   Press 1, 2 or 3. Default begins in 5 seconds.\r\n   Recommended VM memory: 4 GiB or more.\r\n");
 unsigned choice=1;KEY key;for(unsigned i=0;i<50;i++){if(st->in&&!FAILED(st->in->read(st->in,&key))){if(key.unicode>='1'&&key.unicode<='3'){choice=key.unicode-'0';break;}if(key.unicode==13)break;}bs->stall(100000);}
 LI*li=NULL;FS*fs=NULL;STATUS s=bs->protocol(image,&loaded_guid,(void**)&li);if(FAILED(s))goto fail;
 s=bs->protocol(li->device,&fs_guid,(void**)&fs);if(FAILED(s))goto fail;
 s=fs->openvol(fs,&root);if(FAILED(s))goto fail;
 s=root->open(root,&initrd,L"\\ZORIX\\LIVE.CPI",1,0);if(FAILED(s))goto fail;
 s=size_of(initrd,&initrd_size);if(FAILED(s)||!initrd_size){s=ERR(7);goto fail;}
 HANDLE ih=NULL;s=bs->install(&ih,&dp_guid,0,&initrd_dp);if(FAILED(s))goto fail;
 s=bs->install(&ih,&lf_guid,0,&loadfile);if(FAILED(s))goto fail;
 FILE*kernel=NULL;s=root->open(root,&kernel,L"\\ZORIX\\LINUX.EFI",1,0);if(FAILED(s))goto fail;
 UN kernel_size=0;s=size_of(kernel,&kernel_size);if(FAILED(s)||kernel_size<1024||kernel_size>128*1024*1024){s=ERR(7);goto fail;}
 void*buffer=NULL;s=bs->alloc(2,kernel_size,&buffer);if(FAILED(s))goto fail;
 s=read_all(kernel,kernel_size,buffer);kernel->close(kernel);if(FAILED(s))goto fail;
 HANDLE child=NULL;s=bs->load(0,image,NULL,buffer,kernel_size,&child);bs->free(buffer);if(FAILED(s))goto fail;
 LI*cli=NULL;s=bs->protocol(child,&loaded_guid,(void**)&cli);if(FAILED(s))goto fail;
 CHAR16*cmd=choice==2?compatibility:choice==3?recovery:normal;UN chars=0;while(cmd[chars])chars++;
 cli->opts=cmd;cli->opts_size=(U32)((chars+1)*2);
 logo();
 s=bs->start(child,NULL,NULL);
 fail:say(L"\r\n   Zorix could not start. This experimental loader has not passed\r\n   firmware boot validation. Check image integrity and EFI settings.\r\n");
 CHAR16 hex[22]=L"   0x0000000000000000";for(unsigned i=0;i<16;i++){U8 n=(s>>(4*(15-i)))&15;hex[5+i]=n<10?'0'+n:'A'+n-10;}say(hex);say(L"\r\n");bs->stall(10000000);return s;
}
