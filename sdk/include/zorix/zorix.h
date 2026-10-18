#ifndef ZORIX_ZORIX_H
#define ZORIX_ZORIX_H

typedef unsigned char zk_u8;
typedef unsigned int zk_u32;
typedef unsigned long long zk_u64;

#define ZORIX_ABI_VERSION 0x00060000ULL

enum zorix_syscall {
    ZK_GETPID=0,
    ZK_ABI_VERSION=1,
    ZK_CLOCK_TICKS=2,
    ZK_EXIT=3,
    ZK_SCREEN_INFO=9,
    ZK_DRAW_RECT=10,
    ZK_PRESENT=11,
    ZK_SLEEP=13,
    ZK_POINTER=14,
    ZK_KEYBOARD=17,
    ZK_GRADIENT=19,
    ZK_GLASS=20,
    ZK_ROUNDED=21,
    ZK_TEXT=22,
    ZK_SYSTEM_INFO=24,
    ZK_SYMBOL=30
};

static __inline zk_u64 zk_syscall3(zk_u64 nr,zk_u64 a0,zk_u64 a1,zk_u64 a2) {
    register zk_u64 rax __asm__("rax")=nr;
    register zk_u64 rbx __asm__("rbx")=a0;
    register zk_u64 r10 __asm__("r10")=a1;
    register zk_u64 r11 __asm__("r11")=a2;
    __asm__ volatile("int $0x80"
        : "+a"(rax)
        : "b"(rbx), "r"(r10), "r"(r11)
        : "memory");
    return rax;
}

static __inline zk_u64 zk_xy(zk_u32 x,zk_u32 y) {
    return ((zk_u64)x<<32)|(zk_u64)y;
}

static __inline zk_u64 zk_getpid(void) { return zk_syscall3(ZK_GETPID,0,0,0); }
static __inline zk_u64 zk_abi_version(void) { return zk_syscall3(ZK_ABI_VERSION,0,0,0); }
static __inline zk_u64 zk_screen_info(void) { return zk_syscall3(ZK_SCREEN_INFO,0,0,0); }
static __inline void zk_sleep_ms(zk_u32 ms) { (void)zk_syscall3(ZK_SLEEP,ms,0,0); }

static __inline void zk_present(zk_u32 x0,zk_u32 y0,zk_u32 x1,zk_u32 y1) {
    (void)zk_syscall3(ZK_PRESENT,zk_xy(x0,y0),zk_xy(x1,y1),0);
}

static __inline void zk_round(zk_u32 x0,zk_u32 y0,zk_u32 x1,zk_u32 y1,
                              zk_u32 rgb,zk_u8 alpha,zk_u8 radius) {
    zk_u64 style=(zk_u64)(rgb&0xffffffU)|((zk_u64)alpha<<24)|((zk_u64)radius<<32);
    (void)zk_syscall3(ZK_ROUNDED,zk_xy(x0,y0),zk_xy(x1,y1),style);
}

static __inline void zk_glass(zk_u32 x0,zk_u32 y0,zk_u32 x1,zk_u32 y1,
                              zk_u32 tint,zk_u8 alpha,zk_u8 radius,
                              zk_u8 blur,zk_u8 border) {
    zk_u64 style=(zk_u64)(tint&0xffffffU)|((zk_u64)alpha<<24)|
                 ((zk_u64)radius<<32)|((zk_u64)blur<<40)|((zk_u64)border<<48);
    (void)zk_syscall3(ZK_GLASS,zk_xy(x0,y0),zk_xy(x1,y1),style);
}

static __inline void zk_text(const char *s,zk_u32 x,zk_u32 y,zk_u32 rgb,zk_u8 scale) {
    zk_u64 style=(zk_u64)(rgb&0xffffffU)|((zk_u64)scale<<24);
    (void)zk_syscall3(ZK_TEXT,(zk_u64)(unsigned long long)s,zk_xy(x,y),style);
}

static __inline void zk_symbol(zk_u32 id,zk_u32 x,zk_u32 y,zk_u8 size,zk_u32 rgb,zk_u8 alpha) {
    zk_u64 style=(zk_u64)(rgb&0xffffffU)|((zk_u64)alpha<<24)|((zk_u64)size<<32);
    (void)zk_syscall3(ZK_SYMBOL,id,zk_xy(x,y),style);
}

#endif
