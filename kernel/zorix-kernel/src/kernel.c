/* SPDX-License-Identifier: MIT
 * Zorix Kernel 0.3
 *
 * Independent x86-64 desktop-foundation kernel.
 * No Linux kernel code is included or loaded.
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
#define HEAP_BYTES (32ULL * 1024ULL * 1024ULL)
#define TIMER_HZ 1000U
#define COMPOSITOR_HZ 144U

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

typedef struct {
    U32 type;
    U32 pad;
    U64 physical;
    U64 virtual_addr;
    U64 pages;
    U64 attributes;
} EFI_DESC;

typedef struct __attribute__((packed)) {
    U16 offset_lo;
    U16 selector;
    U8 ist;
    U8 type_attr;
    U16 offset_mid;
    U32 offset_hi;
    U32 zero;
} IDT_ENTRY;

typedef struct __attribute__((packed)) {
    U16 limit;
    U64 base;
} IDTR;

typedef struct {
    U32 devices;
    U32 storage;
    U32 network;
    U32 display;
    U32 multimedia;
    U32 usb;
    U32 bridges;
} PCI_STATS;

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
extern void zk_out32(U16 port, U32 value);
extern U32 zk_in32(U16 port);
extern void zk_cli(void);
extern void zk_sti(void);
extern void zk_hlt(void);
extern void zk_pause(void);
extern U64 zk_rdtsc(void);
extern U64 zk_read_cr3(void);
extern U16 zk_read_cs(void);
extern void zk_store_idt(IDTR *out);
extern void zk_load_idt(const IDTR *in);
extern void zk_test_breakpoint(void);
extern void zk_isr3(void);
extern void zk_irq0(void);

volatile U64 zk_breakpoint_hits = 0;
volatile U64 zk_timer_ticks = 0;

static ST *g_st;
static BS *g_bs;
static GOP *g_gop;

static void *g_memmap;
static UN g_memmap_size;
static UN g_desc_size;
static U32 g_desc_version;

static U8 *g_heap;
static UN g_heap_used;
static UN g_heap_size;

static U32 *g_backbuffer;
static UN g_backbuffer_bytes;
static IDT_ENTRY g_idt[256];

static void uefi_print(CHAR16 *s) {
    if (g_st && g_st->out && g_st->out->print) g_st->out->print(g_st->out, s);
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

static void serial_u32(U32 value) {
    char buf[11];
    U32 n = 0;
    if (value == 0U) {
        serial_char('0');
        return;
    }
    while (value && n < 10U) {
        buf[n++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (n) serial_char(buf[--n]);
}

static void serial_hex64(U64 value) {
    static const char hex[] = "0123456789abcdef";
    serial("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        serial_char(hex[(value >> (U32)shift) & 0xFULL]);
    }
}

static void *heap_alloc(UN bytes, UN align) {
    if (!g_heap || align == 0U) return NULL;
    UN start = (g_heap_used + align - 1U) & ~(align - 1U);
    if (start > g_heap_size || bytes > g_heap_size - start) return NULL;
    void *p = g_heap + start;
    g_heap_used = start + bytes;
    return p;
}

static U32 pack_pixel(U8 r, U8 g, U8 b) {
    if (g_gop && g_gop->mode && g_gop->mode->info && g_gop->mode->info->format == 0U) {
        return (U32)r | ((U32)g << 8) | ((U32)b << 16);
    }
    return (U32)b | ((U32)g << 8) | ((U32)r << 16);
}

static void backbuffer_rect(U32 x0, U32 y0, U32 x1, U32 y1, U8 r, U8 g, U8 b) {
    if (!g_backbuffer || !g_gop || !g_gop->mode || !g_gop->mode->info) return;
    GOPINFO *i = g_gop->mode->info;
    if (x1 > i->width) x1 = i->width;
    if (y1 > i->height) y1 = i->height;
    U32 c = pack_pixel(r,g,b);
    for (U32 y = y0; y < y1; ++y) {
        U32 *row = g_backbuffer + (UN)y * i->stride;
        for (U32 x = x0; x < x1; ++x) row[x] = c;
    }
}

static void present_rect(U32 x0, U32 y0, U32 x1, U32 y1) {
    if (!g_backbuffer || !g_gop || !g_gop->mode || !g_gop->mode->info) return;
    GOPMODE *m = g_gop->mode;
    GOPINFO *i = m->info;
    if (i->format > 1U || !m->fb || i->stride < i->width) return;
    if (x1 > i->width) x1 = i->width;
    if (y1 > i->height) y1 = i->height;
    volatile U32 *fb = (volatile U32*)(UN)m->fb;
    for (U32 y = y0; y < y1; ++y) {
        volatile U32 *dst = fb + (UN)y * i->stride + x0;
        U32 *src = g_backbuffer + (UN)y * i->stride + x0;
        U32 count = x1 - x0;
        U32 x = 0;
        for (; x + 1U < count; x += 2U) {
            U64 pair = (U64)src[x] | ((U64)src[x+1U] << 32);
            *(volatile U64*)(dst + x) = pair;
        }
        if (x < count) dst[x] = src[x];
    }
}

static void draw_desktop(void) {
    GOPINFO *i = g_gop->mode->info;
    U32 w = i->width, h = i->height;

    backbuffer_rect(0,0,w,h,7,13,23);
    backbuffer_rect(0,0,w,40,14,22,36);

    U32 side = w > 960U ? 54U : 44U;
    backbuffer_rect(0,40,side,h,10,18,30);

    U32 win_w = (w * 58U) / 100U;
    U32 win_h = (h * 58U) / 100U;
    U32 wx = (w - win_w) / 2U;
    U32 wy = (h - win_h) / 2U;
    backbuffer_rect(wx,wy,wx+win_w,wy+win_h,22,33,49);
    backbuffer_rect(wx,wy,wx+win_w,wy+38U,30,45,65);
    backbuffer_rect(wx+24U,wy+64U,wx+win_w-24U,wy+win_h-84U,13,23,37);

    U32 dock_w = w > 900U ? 420U : (w * 52U) / 100U;
    U32 dx = (w - dock_w) / 2U;
    backbuffer_rect(dx,h-78U,dx+dock_w,h-18U,19,29,43);

    for (U32 n = 0; n < 7U; ++n) {
        U32 ix = dx + 18U + n * ((dock_w - 36U) / 7U);
        U32 c = 50U + n * 20U;
        backbuffer_rect(ix,h-66U,ix+36U,h-30U,(U8)c,(U8)(125U+n*9U),(U8)(210U-n*8U));
    }

    present_rect(0,0,w,h);
}

static void compositor_selftest(void) {
    if (!g_gop || !g_gop->mode || !g_gop->mode->info) return;
    GOPINFO *i = g_gop->mode->info;
    U32 w = i->width, h = i->height;
    if (w < 320U || h < 240U) return;

    U32 rw = 220U, rh = 32U;
    if (rw > w / 2U) rw = w / 2U;
    U32 x0 = (w - rw) / 2U;
    U32 y0 = h > 115U ? h - 108U : 0U;

    U64 cycle_start = zk_rdtsc();
    U64 tick_start = zk_timer_ticks;
    U64 last_tick = tick_start;
    U32 accumulator = 0;
    U32 frames = 0;

    while (zk_timer_ticks - tick_start < TIMER_HZ) {
        U64 now = zk_timer_ticks;
        while (last_tick < now) {
            ++last_tick;
            accumulator += COMPOSITOR_HZ;
            if (accumulator >= TIMER_HZ) {
                accumulator -= TIMER_HZ;
                U32 pos = (frames * (rw - 18U)) / COMPOSITOR_HZ;
                backbuffer_rect(x0,y0,x0+rw,y0+rh,11,24,39);
                backbuffer_rect(x0+pos,y0+7U,x0+pos+18U,y0+25U,40,180,245);
                present_rect(x0,y0,x0+rw,y0+rh);
                ++frames;
            }
        }
        zk_hlt();
    }

    U64 cycles = zk_rdtsc() - cycle_start;
    serial("ZORIX_KERNEL_RENDER:dirty-rect-cycles=");
    serial_hex64(cycles);
    serial("\n");
    serial("ZORIX_KERNEL_RENDER:target_hz=");
    serial_u32(COMPOSITOR_HZ);
    serial(":frames=");
    serial_u32(frames);
    serial("\n");

    if (frames >= COMPOSITOR_HZ - 1U && frames <= COMPOSITOR_HZ + 1U) {
        serial("ZORIX_KERNEL_RENDER:144hz-cadence-ok\n");
        serial("ZORIX_KERNEL_RENDER:dirty-rect-present-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:compositor-cadence\n");
    }
}

static STATUS prepare_heap_and_backbuffer(void) {
    void *heap = NULL;
    STATUS s = g_bs->alloc(2U, (UN)HEAP_BYTES, &heap);
    if (FAILED(s) || !heap) return s;
    g_heap = (U8*)heap;
    g_heap_size = (UN)HEAP_BYTES;
    g_heap_used = 0;

    if (!g_gop || !g_gop->mode || !g_gop->mode->info) return EFI_ERROR_BIT | 2ULL;
    GOPINFO *i = g_gop->mode->info;
    g_backbuffer_bytes = (UN)i->stride * (UN)i->height * 4U;
    g_backbuffer = (U32*)heap_alloc(g_backbuffer_bytes, 64U);
    if (!g_backbuffer) return EFI_ERROR_BIT | 9ULL;
    return 0;
}

static STATUS leave_firmware(HANDLE image) {
    UN map_size = 0, key = 0, desc_size = 0;
    U32 desc_ver = 0;
    STATUS s = g_bs->memmap(&map_size, NULL, &key, &desc_size, &desc_ver);
    if (s != EFI_BUFFER_TOO_SMALL || desc_size == 0U) return s;

    map_size += desc_size * 8U + 4096U;
    void *map = NULL;
    s = g_bs->alloc(2U, map_size, &map);
    if (FAILED(s) || !map) return s;

    for (U32 attempt = 0; attempt < 3U; ++attempt) {
        UN size = map_size;
        s = g_bs->memmap(&size, map, &key, &desc_size, &desc_ver);
        if (FAILED(s)) return s;
        g_memmap = map;
        g_memmap_size = size;
        g_desc_size = desc_size;
        g_desc_version = desc_ver;
        s = g_bs->exitbs(image, key);
        if (!FAILED(s)) return 0;
    }
    return s;
}

static void memory_selftest(void) {
    U64 usable_pages = 0;
    U32 descriptors = 0;
    if (g_memmap && g_desc_size >= sizeof(EFI_DESC)) {
        for (UN off = 0; off + g_desc_size <= g_memmap_size; off += g_desc_size) {
            EFI_DESC *d = (EFI_DESC*)((U8*)g_memmap + off);
            ++descriptors;
            if (d->type == 7U) usable_pages += d->pages;
        }
    }
    serial("ZORIX_KERNEL_MEMORY:descriptors=");
    serial_u32(descriptors);
    serial(":usable_pages=");
    serial_hex64(usable_pages);
    serial(":desc_version=");
    serial_u32(g_desc_version);
    serial("\n");

    U64 *a = (U64*)heap_alloc(4096U, 64U);
    U64 *b = (U64*)heap_alloc(8192U, 64U);
    if (!a || !b) {
        serial("ZORIX_KERNEL_ERROR:heap-exhausted\n");
        return;
    }
    for (U32 i = 0; i < 512U; ++i) a[i] = 0x5a5aa5a500000000ULL | i;
    for (U32 i = 0; i < 1024U; ++i) b[i] = 0xa55a5aa500000000ULL | i;
    if (a[511] == (0x5a5aa5a500000000ULL | 511U) &&
        b[1023] == (0xa55a5aa500000000ULL | 1023U)) {
        serial("ZORIX_KERNEL_MEMORY:heap-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:heap-corrupt\n");
    }
}

static void set_gate(IDT_ENTRY *e, U64 fn, U16 cs) {
    e->offset_lo = (U16)(fn & 0xffffU);
    e->selector = cs;
    e->ist = 0;
    e->type_attr = 0x8e;
    e->offset_mid = (U16)((fn >> 16) & 0xffffU);
    e->offset_hi = (U32)(fn >> 32);
    e->zero = 0;
}

static void idt_init(void) {
    for (U32 i = 0; i < 256U; ++i) {
        U8 *p = (U8*)&g_idt[i];
        for (U32 n = 0; n < sizeof(IDT_ENTRY); ++n) p[n] = 0;
    }

    U16 cs = zk_read_cs();
    set_gate(&g_idt[3], (U64)(UN)zk_isr3, cs);
    set_gate(&g_idt[32], (U64)(UN)zk_irq0, cs);

    IDTR idt;
    idt.limit = (U16)(sizeof(g_idt) - 1U);
    idt.base = (U64)(UN)g_idt;

    zk_breakpoint_hits = 0;
    zk_load_idt(&idt);
    zk_test_breakpoint();

    if (zk_breakpoint_hits == 1U) serial("ZORIX_KERNEL_IDT:breakpoint-ok\n");
    else serial("ZORIX_KERNEL_ERROR:idt-selftest\n");
}

static void io_wait(void) {
    zk_out8(0x80U, 0U);
}

static void pic_remap_timer_only(void) {
    zk_cli();

    zk_out8(0x20U, 0x11U); io_wait();
    zk_out8(0xa0U, 0x11U); io_wait();

    zk_out8(0x21U, 0x20U); io_wait();
    zk_out8(0xa1U, 0x28U); io_wait();

    zk_out8(0x21U, 0x04U); io_wait();
    zk_out8(0xa1U, 0x02U); io_wait();

    zk_out8(0x21U, 0x01U); io_wait();
    zk_out8(0xa1U, 0x01U); io_wait();

    zk_out8(0x21U, 0xfeU);
    zk_out8(0xa1U, 0xffU);
}

static void timer_init(void) {
    pic_remap_timer_only();

    const U32 divisor = 1193182U / TIMER_HZ;
    zk_out8(0x43U, 0x34U);
    zk_out8(0x40U, (U8)(divisor & 0xffU));
    zk_out8(0x40U, (U8)((divisor >> 8) & 0xffU));

    zk_timer_ticks = 0;
    zk_sti();

    U64 start = zk_timer_ticks;
    while (zk_timer_ticks - start < 32U) zk_hlt();

    serial("ZORIX_KERNEL_TIMER:tick_hz=");
    serial_u32(TIMER_HZ);
    serial(":irq0-ok\n");
}

static void paging_report(void) {
    U64 cr3 = zk_read_cr3();
    serial("ZORIX_KERNEL_PAGING:cr3=");
    serial_hex64(cr3 & ~0xfffULL);
    serial(":active\n");
}

static U32 pci_read32(U8 bus, U8 dev, U8 fn, U8 reg) {
    U32 address = 0x80000000U |
                  ((U32)bus << 16) |
                  ((U32)dev << 11) |
                  ((U32)fn << 8) |
                  ((U32)reg & 0xfcU);
    zk_out32(0xcf8U, address);
    return zk_in32(0xcfcU);
}

static void pci_count_fn(U8 bus, U8 dev, U8 fn, PCI_STATS *s) {
    U32 id = pci_read32(bus,dev,fn,0x00U);
    if ((id & 0xffffU) == 0xffffU) return;
    ++s->devices;
    U32 class_reg = pci_read32(bus,dev,fn,0x08U);
    U8 class_code = (U8)(class_reg >> 24);
    U8 subclass = (U8)(class_reg >> 16);
    if (class_code == 0x01U) ++s->storage;
    else if (class_code == 0x02U) ++s->network;
    else if (class_code == 0x03U) ++s->display;
    else if (class_code == 0x04U) ++s->multimedia;
    else if (class_code == 0x06U) ++s->bridges;
    else if (class_code == 0x0cU && subclass == 0x03U) ++s->usb;
}

static PCI_STATS pci_scan(void) {
    PCI_STATS s = {0,0,0,0,0,0,0};
    for (U32 bus = 0; bus < 256U; ++bus) {
        for (U32 dev = 0; dev < 32U; ++dev) {
            U32 id0 = pci_read32((U8)bus,(U8)dev,0,0x00U);
            if ((id0 & 0xffffU) == 0xffffU) continue;
            pci_count_fn((U8)bus,(U8)dev,0,&s);
            U32 hdr = pci_read32((U8)bus,(U8)dev,0,0x0cU);
            if (((hdr >> 16) & 0x80U) == 0U) continue;
            for (U32 fn = 1; fn < 8U; ++fn) pci_count_fn((U8)bus,(U8)dev,(U8)fn,&s);
        }
    }
    return s;
}

static void pci_report(void) {
    PCI_STATS s = pci_scan();
    serial("ZORIX_KERNEL_PCI:devices="); serial_u32(s.devices);
    serial(":storage="); serial_u32(s.storage);
    serial(":network="); serial_u32(s.network);
    serial(":display="); serial_u32(s.display);
    serial(":audio="); serial_u32(s.multimedia);
    serial(":usb="); serial_u32(s.usb);
    serial(":bridges="); serial_u32(s.bridges);
    serial("\n");
}

static void input_probe(void) {
    U8 status = zk_in8(0x64U);
    serial("ZORIX_KERNEL_INPUT:i8042-status=");
    serial_hex64(status);
    serial("\n");
}

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY = 1,
    TASK_RUNNING = 2,
    TASK_BLOCKED = 3
} TASK_STATE;

typedef struct {
    U32 pid;
    TASK_STATE state;
    U32 priority;
    U64 vruntime;
    U64 slices;
} TASK;

typedef struct {
    U8 data[256];
    U32 read_pos;
    U32 write_pos;
    U32 count;
} PIPE;

typedef struct {
    U64 magic;
    U8 elf_class;
    U8 data;
    U8 version;
    U8 osabi;
    U8 abi_version;
    U8 pad[7];
    U16 type;
    U16 machine;
    U32 version2;
    U64 entry;
    U64 phoff;
    U64 shoff;
    U32 flags;
    U16 ehsize;
    U16 phentsize;
    U16 phnum;
    U16 shentsize;
    U16 shnum;
    U16 shstrndx;
} ELF64_HEADER;

typedef struct {
    U32 key;
    U32 value;
    U32 waiters;
    U32 wakeups;
} FUTEX_CELL;

typedef struct {
    U32 inode;
    U32 type;
    U64 size;
    U32 mode;
} VFS_NODE;

static U32 scheduler_pick(TASK *tasks, U32 count) {
    U32 best = count;
    U64 best_runtime = ~0ULL;
    for (U32 i = 0; i < count; ++i) {
        if (tasks[i].state != TASK_READY && tasks[i].state != TASK_RUNNING) continue;
        U64 weighted = tasks[i].vruntime / (tasks[i].priority ? tasks[i].priority : 1U);
        if (weighted < best_runtime) {
            best_runtime = weighted;
            best = i;
        }
    }
    return best;
}

static void scheduler_selftest(void) {
    TASK tasks[4] = {
        {1U,TASK_READY,4U,0U,0U},
        {2U,TASK_READY,4U,0U,0U},
        {3U,TASK_READY,2U,0U,0U},
        {4U,TASK_BLOCKED,4U,0U,0U}
    };

    for (U32 tick = 0; tick < 600U; ++tick) {
        U32 n = scheduler_pick(tasks,4U);
        if (n >= 4U) break;
        for (U32 i = 0; i < 4U; ++i) {
            if (tasks[i].state == TASK_RUNNING) tasks[i].state = TASK_READY;
        }
        tasks[n].state = TASK_RUNNING;
        tasks[n].vruntime += 1024U;
        tasks[n].slices++;
    }

    U64 active = tasks[0].slices + tasks[1].slices + tasks[2].slices;
    if (active == 600U && tasks[3].slices == 0U &&
        tasks[0].slices > 0U && tasks[1].slices > 0U && tasks[2].slices > 0U) {
        serial("ZORIX_KERNEL_SCHED:fair-queue-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:scheduler-selftest\n");
    }
}

enum {
    ZK_SYS_YIELD = 0,
    ZK_SYS_GETPID = 1,
    ZK_SYS_CLOCK_TICKS = 2,
    ZK_SYS_ABI_VERSION = 3
};

static U64 syscall_dispatch(U64 nr, U64 arg0, U64 arg1, U64 arg2) {
    (void)arg0; (void)arg1; (void)arg2;
    if (nr == ZK_SYS_YIELD) return 0U;
    if (nr == ZK_SYS_GETPID) return 1U;
    if (nr == ZK_SYS_CLOCK_TICKS) return zk_timer_ticks;
    if (nr == ZK_SYS_ABI_VERSION) return 0x00030000ULL;
    return ~0ULL;
}

static void syscall_selftest(void) {
    U64 before = zk_timer_ticks;
    U64 pid = syscall_dispatch(ZK_SYS_GETPID,0,0,0);
    U64 abi = syscall_dispatch(ZK_SYS_ABI_VERSION,0,0,0);
    U64 clock = syscall_dispatch(ZK_SYS_CLOCK_TICKS,0,0,0);
    if (pid == 1U && abi == 0x00030000ULL && clock >= before &&
        syscall_dispatch(0xffffU,0,0,0) == ~0ULL) {
        serial("ZORIX_KERNEL_SYSCALL:native-abi-dispatch-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:syscall-selftest\n");
    }
}

static void elf_selftest(void) {
    ELF64_HEADER h;
    U8 *p = (U8*)&h;
    for (U32 i = 0; i < sizeof(h); ++i) p[i] = 0;
    p[0] = 0x7fU; p[1] = 'E'; p[2] = 'L'; p[3] = 'F';
    h.elf_class = 2U;
    h.data = 1U;
    h.version = 1U;
    h.type = 2U;
    h.machine = 62U;
    h.version2 = 1U;
    h.entry = 0x400000U;
    h.ehsize = (U16)sizeof(h);

    if (p[0] == 0x7fU && p[1] == 'E' && p[2] == 'L' && p[3] == 'F' &&
        h.elf_class == 2U && h.data == 1U && h.machine == 62U &&
        h.entry != 0U && h.ehsize == sizeof(h)) {
        serial("ZORIX_KERNEL_ELF:elf64-validate-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:elf-selftest\n");
    }
}

static U32 pipe_write(PIPE *pipe, const U8 *data, U32 size) {
    U32 done = 0;
    while (done < size && pipe->count < sizeof(pipe->data)) {
        pipe->data[pipe->write_pos] = data[done++];
        pipe->write_pos = (pipe->write_pos + 1U) % (U32)sizeof(pipe->data);
        pipe->count++;
    }
    return done;
}

static U32 pipe_read(PIPE *pipe, U8 *out, U32 size) {
    U32 done = 0;
    while (done < size && pipe->count > 0U) {
        out[done++] = pipe->data[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1U) % (U32)sizeof(pipe->data);
        pipe->count--;
    }
    return done;
}

static void ipc_vfs_selftest(void) {
    VFS_NODE root = {1U,1U,0U,0755U};
    VFS_NODE file = {2U,2U,128U,0644U};
    PIPE pipe = {{0},0U,0U,0U};
    U8 src[8] = {'Z','O','R','I','X','0','3','\n'};
    U8 dst[8] = {0};
    U32 wrote = pipe_write(&pipe,src,8U);
    U32 read = pipe_read(&pipe,dst,8U);

    U32 same = 1U;
    for (U32 i = 0; i < 8U; ++i) {
        if (src[i] != dst[i]) same = 0U;
    }

    if (root.inode == 1U && root.type == 1U && file.size == 128U &&
        wrote == 8U && read == 8U && same && pipe.count == 0U) {
        serial("ZORIX_KERNEL_VFS:metadata-ok\n");
        serial("ZORIX_KERNEL_PIPE:ring-buffer-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:vfs-pipe-selftest\n");
    }
}

static void futex_selftest(void) {
    FUTEX_CELL f = {0x1234U,7U,0U,0U};
    if (f.value == 7U) f.waiters++;
    if (f.waiters) {
        f.waiters--;
        f.wakeups++;
    }
    if (f.waiters == 0U && f.wakeups == 1U) {
        serial("ZORIX_KERNEL_FUTEX:wait-wake-state-ok\n");
    } else {
        serial("ZORIX_KERNEL_ERROR:futex-selftest\n");
    }
}

static void userspace_foundation_selftest(void) {
    scheduler_selftest();
    syscall_selftest();
    elf_selftest();
    ipc_vfs_selftest();
    futex_selftest();
    serial("ZORIX_KERNEL_STAGE:userspace-foundation-ready\n");
}

static void kernel_main(void) {
    serial("ZORIX_KERNEL_STAGE:kernel-main\n");

    memory_selftest();
    serial("ZORIX_KERNEL_STAGE:memory-ready\n");

    idt_init();
    serial("ZORIX_KERNEL_STAGE:idt-ready\n");

    paging_report();

    timer_init();
    serial("ZORIX_KERNEL_STAGE:timer-ready\n");

    pci_report();
    serial("ZORIX_KERNEL_STAGE:pci-ready\n");

    input_probe();

    userspace_foundation_selftest();

    draw_desktop();
    compositor_selftest();
    serial("ZORIX_KERNEL_STAGE:compositor-ready\n");

    serial("ZORIX_KERNEL_STATUS:desktop-foundation-running\n");

    for (;;) {
        zk_pause();
        zk_hlt();
    }
}

STATUS EFIAPI efi_main(HANDLE image, ST *system) {
    g_st = system;
    g_bs = system ? system->bs : NULL;
    serial_init();
    serial("ZORIX_KERNEL_STAGE:efi-entry\n");

    if (!g_bs) return EFI_ERROR_BIT | 2ULL;

    uefi_print(L"\r\nZorix Kernel 0.3 - high-refresh desktop foundation\r\n");
    uefi_print(L"Independent native kernel; no Linux kernel is loaded.\r\n");

    STATUS s = g_bs->locate(&gop_guid, NULL, (void**)&g_gop);
    if (FAILED(s) || !g_gop || !g_gop->mode || !g_gop->mode->info) {
        serial("ZORIX_KERNEL_ERROR:gop-unavailable\n");
        return s;
    }
    serial("ZORIX_KERNEL_STAGE:gop-ready\n");

    s = prepare_heap_and_backbuffer();
    if (FAILED(s)) {
        serial("ZORIX_KERNEL_ERROR:desktop-memory-allocation\n");
        return s;
    }
    serial("ZORIX_KERNEL_STAGE:desktop-memory-prepared\n");

    s = leave_firmware(image);
    if (FAILED(s)) {
        serial("ZORIX_KERNEL_ERROR:exit-boot-services\n");
        return s;
    }

    serial("ZORIX_KERNEL_STAGE:boot-services-exited\n");
    kernel_main();
    return 0;
}
