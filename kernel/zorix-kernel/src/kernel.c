/* SPDX-License-Identifier: MIT
 * Zorix Kernel 0.1
 *
 * Independent x86-64 kernel prototype.
 * This file contains no Linux kernel code and does not boot a Linux kernel.
 * UEFI is used only for firmware handoff; after ExitBootServices() the
 * framebuffer and serial console are owned directly by Zorix Kernel.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef unsigned long long UN;
typedef U16 CHAR16;
typedef U64 STATUS;
typedef void *HANDLE;

#define EFIAPI __attribute__((ms_abi))
#define NULL ((void*)0)
#define EFI_ERROR_BIT 0x8000000000000000ULL
#define EFI_BUFFER_TOO_SMALL (EFI_ERROR_BIT | 5ULL)
#define FAILED(s) (((s) & EFI_ERROR_BIT) != 0)

typedef struct { U32 a; U16 b,c; U8 d[8]; } GUID;
typedef struct { U64 sig; U32 rev,size,crc,reserved; } HDR;

typedef struct {
    U32 ver;
    U32 width;
    U32 height;
    U32 format;
    U32 masks[4];
    U32 stride;
} GOPINFO;

typedef struct {
    U32 max;
    U32 mode;
    GOPINFO *info;
    UN info_size;
    U64 fb;
    UN fbsize;
} GOPMODE;

typedef struct GOP GOP;
struct GOP {
    void *query;
    void *set;
    void *blt;
    GOPMODE *mode;
};

typedef struct {
    void *reset;
    STATUS (EFIAPI *print)(void*, CHAR16*);
    void *test;
    void *query;
    void *setmode;
    void *attr;
    void *clear;
    void *cursor;
    void *enable;
    void *mode;
} OUT;

typedef struct BS BS;
struct BS {
    HDR h;
    void *raise;
    void *restore;
    void *allocpages;
    void *freepages;
    STATUS (EFIAPI *memmap)(UN*, void*, UN*, UN*, U32*);
    STATUS (EFIAPI *alloc)(U32, UN, void**);
    STATUS (EFIAPI *free)(void*);
    void *createevent;
    void *settimer;
    void *waitevent;
    void *signal;
    void *closeevent;
    void *checkevent;
    void *install;
    void *reinstall;
    void *uninstall;
    void *protocol;
    void *reserved;
    void *notify;
    void *locatehandle;
    void *locatedp;
    void *installtable;
    void *load;
    void *start;
    void *exit;
    void *unload;
    STATUS (EFIAPI *exitbs)(HANDLE, UN);
    void *monotonic;
    void *stall;
    void *watchdog;
    void *connect;
    void *disconnect;
    void *openprotocol;
    void *closeprotocol;
    void *protocolinfo;
    void *protocols;
    void *locatebuffer;
    STATUS (EFIAPI *locate)(GUID*, void*, void**);
    void *installmulti;
    void *uninstallmulti;
    void *crc;
    void *copy;
    void *set;
    void *eventex;
};

typedef struct {
    HDR h;
    CHAR16 *vendor;
    U32 rev;
    HANDLE hin;
    void *in;
    HANDLE hout;
    OUT *out;
    HANDLE herr;
    OUT *err;
    void *runtime;
    BS *bs;
    UN tables;
    void *config;
} ST;

_Static_assert(__builtin_offsetof(BS,memmap) == 56, "UEFI GetMemoryMap ABI");
_Static_assert(__builtin_offsetof(BS,exitbs) == 232, "UEFI ExitBootServices ABI");
_Static_assert(__builtin_offsetof(BS,locate) == 320, "UEFI LocateProtocol ABI");
_Static_assert(__builtin_offsetof(ST,bs) == 96, "UEFI SystemTable ABI");

static GUID gop_guid = {
    0x9042a9de, 0x23dc, 0x4a38,
    {0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}
};

extern void zk_out8(U16 port, U8 value);
extern U8 zk_in8(U16 port);
extern void zk_cli(void);
extern void zk_hlt(void);

static ST *g_st;
static BS *g_bs;
static GOP *g_gop;

static void uefi_print(CHAR16 *s) {
    if (g_st && g_st->out && g_st->out->print) {
        g_st->out->print(g_st->out, s);
    }
}

static void serial_init(void) {
    const U16 p = 0x3f8;
    zk_out8(p + 1, 0x00);
    zk_out8(p + 3, 0x80);
    zk_out8(p + 0, 0x01);
    zk_out8(p + 1, 0x00);
    zk_out8(p + 3, 0x03);
    zk_out8(p + 2, 0xc7);
    zk_out8(p + 4, 0x0b);
}

static void serial_char(char c) {
    const U16 p = 0x3f8;
    for (U32 i = 0; i < 1000000U; ++i) {
        if (zk_in8(p + 5) & 0x20U) {
            zk_out8(p, (U8)c);
            return;
        }
    }
}

static void serial(const char *s) {
    while (*s) {
        if (*s == '\n') serial_char('\r');
        serial_char(*s++);
    }
}

static U32 pixel(U8 r, U8 g, U8 b, U32 format) {
    if (format == 0U) {
        return (U32)r | ((U32)g << 8) | ((U32)b << 16);
    }
    return (U32)b | ((U32)g << 8) | ((U32)r << 16);
}

static void put_rect(U32 x0, U32 y0, U32 x1, U32 y1, U8 r, U8 g, U8 b) {
    if (!g_gop || !g_gop->mode || !g_gop->mode->info) return;
    GOPMODE *m = g_gop->mode;
    GOPINFO *i = m->info;
    if (i->format > 1U || !m->fb || i->stride < i->width) return;
    if (x1 > i->width) x1 = i->width;
    if (y1 > i->height) y1 = i->height;
    volatile U32 *fb = (volatile U32*)(UN)m->fb;
    const U32 c = pixel(r,g,b,i->format);
    for (U32 y = y0; y < y1; ++y) {
        volatile U32 *row = fb + (UN)y * i->stride;
        for (U32 x = x0; x < x1; ++x) row[x] = c;
    }
}

static void draw_kernel_screen(void) {
    if (!g_gop || !g_gop->mode || !g_gop->mode->info) return;
    GOPINFO *i = g_gop->mode->info;
    U32 w = i->width, h = i->height;
    if (w < 640U || h < 400U) return;

    put_rect(0,0,w,h,8,14,24);
    put_rect(0,0,w,42,14,23,38);

    U32 box = h / 3U;
    if (box > 260U) box = 260U;
    if (box < 160U) box = 160U;
    U32 x0 = (w - box) / 2U;
    U32 y0 = (h - box) / 2U;

    put_rect(x0,y0,x0+box,y0+box,20,129,240);

    U32 t = box / 7U;
    put_rect(x0+box/5U, y0+box/5U, x0+box*4U/5U, y0+box/5U+t, 245,250,255);
    put_rect(x0+box/5U, y0+box*4U/5U-t, x0+box*4U/5U, y0+box*4U/5U, 245,250,255);
    for (U32 n = 0; n < box/2U; ++n) {
        U32 xx = x0 + box*3U/4U - n;
        U32 yy = y0 + box/4U + n;
        put_rect(xx, yy, xx+t, yy+t, 245,250,255);
    }
}

static STATUS leave_firmware(HANDLE image) {
    UN map_size = 0, key = 0, desc_size = 0;
    U32 desc_ver = 0;
    STATUS s = g_bs->memmap(&map_size, NULL, &key, &desc_size, &desc_ver);
    if (s != EFI_BUFFER_TOO_SMALL || desc_size == 0) return s;

    map_size += desc_size * 8U + 4096U;
    void *map = NULL;
    s = g_bs->alloc(2U, map_size, &map);
    if (FAILED(s) || !map) return s;

    for (U32 attempt = 0; attempt < 2U; ++attempt) {
        UN size = map_size;
        s = g_bs->memmap(&size, map, &key, &desc_size, &desc_ver);
        if (FAILED(s)) return s;
        s = g_bs->exitbs(image, key);
        if (!FAILED(s)) return 0;
    }
    return s;
}

static void kernel_main(void) {
    serial("ZORIX_KERNEL_STAGE:kernel-main\n");
    draw_kernel_screen();
    serial("ZORIX_KERNEL_STATUS:native-kernel-running\n");
    zk_cli();
    for (;;) zk_hlt();
}

STATUS EFIAPI efi_main(HANDLE image, ST *system) {
    g_st = system;
    g_bs = system ? system->bs : NULL;
    serial_init();
    serial("ZORIX_KERNEL_STAGE:efi-entry\n");

    if (!g_bs) return EFI_ERROR_BIT | 2ULL;

    uefi_print(L"\r\nZorix Kernel 0.1 - independent native kernel prototype\r\n");
    uefi_print(L"No Linux kernel is loaded by this image.\r\n");

    STATUS s = g_bs->locate(&gop_guid, NULL, (void**)&g_gop);
    if (FAILED(s) || !g_gop || !g_gop->mode || !g_gop->mode->info) {
        serial("ZORIX_KERNEL_ERROR:gop-unavailable\n");
        return s;
    }
    serial("ZORIX_KERNEL_STAGE:gop-ready\n");

    s = leave_firmware(image);
    if (FAILED(s)) {
        serial("ZORIX_KERNEL_ERROR:exit-boot-services\n");
        return s;
    }

    serial("ZORIX_KERNEL_STAGE:boot-services-exited\n");
    kernel_main();
    return 0;
}
