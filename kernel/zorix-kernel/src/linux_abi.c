/* SPDX-License-Identifier: MIT
 * Zorix Linux ABI compatibility subset.
 *
 * This is a translation layer foundation, not a claim of complete Linux ABI
 * compatibility. It provides concrete syscall semantics used by the native
 * compatibility tests and selected simple ELF programs.
 */
typedef unsigned char U8;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef long long S64;

enum {
    LX_SYS_READ=0,
    LX_SYS_WRITE=1,
    LX_SYS_MMAP=9,
    LX_SYS_MPROTECT=10,
    LX_SYS_MUNMAP=11,
    LX_SYS_GETPID=39,
    LX_SYS_EXIT=60,
    LX_SYS_FUTEX=202,
    LX_SYS_CLOCK_GETTIME=228
};

typedef struct {
    S64 tv_sec;
    S64 tv_nsec;
} LX_TIMESPEC;

typedef struct {
    U64 base;
    U64 length;
    U32 prot;
    U32 flags;
    U32 active;
} LX_MAPPING;

typedef struct {
    LX_MAPPING maps[16];
    U32 map_count;
    U64 next_base;
    U32 pid;
    U64 ticks;
    U32 futex_wakeups;
    U32 writes;
} LX_ABI_STATE;

static U64 align4096(U64 n) {
    return (n+4095ULL)&~4095ULL;
}

void zk_linux_abi_init(LX_ABI_STATE *s,U32 pid,U64 ticks) {
    for(U32 i=0;i<sizeof(*s);++i) ((U8*)s)[i]=0;
    s->pid=pid;
    s->ticks=ticks;
    s->next_base=0x0000000040000000ULL;
}

S64 zk_linux_abi_dispatch(LX_ABI_STATE *s,U64 nr,U64 a0,U64 a1,U64 a2,U64 a3) {
    if(!s) return -22;

    if(nr==LX_SYS_GETPID) return (S64)s->pid;

    if(nr==LX_SYS_WRITE) {
        /* Compatibility core accepts stdout/stderr writes after pointer/length
         * validation; actual console routing is wired at a higher layer. */
        if((a0!=1ULL&&a0!=2ULL)||a1==0ULL||a2==0ULL) return -9;
        s->writes++;
        return (S64)a2;
    }

    if(nr==LX_SYS_CLOCK_GETTIME) {
        if(a1==0ULL) return -14;
        LX_TIMESPEC *ts=(LX_TIMESPEC*)(unsigned long long)a1;
        ts->tv_sec=(S64)(s->ticks/1000ULL);
        ts->tv_nsec=(S64)((s->ticks%1000ULL)*1000000ULL);
        return 0;
    }

    if(nr==LX_SYS_MMAP) {
        U64 len=align4096(a1);
        if(len==0ULL||s->map_count>=16U) return -12;
        LX_MAPPING *m=&s->maps[s->map_count++];
        m->base=a0?a0:s->next_base;
        m->length=len;
        m->prot=(U32)a2;
        m->flags=(U32)a3;
        m->active=1U;
        if(!a0) s->next_base+=len;
        return (S64)m->base;
    }

    if(nr==LX_SYS_MUNMAP) {
        for(U32 i=0;i<s->map_count;++i) {
            if(s->maps[i].active&&s->maps[i].base==a0) {
                s->maps[i].active=0U;
                return 0;
            }
        }
        return -22;
    }

    if(nr==LX_SYS_MPROTECT) {
        for(U32 i=0;i<s->map_count;++i) {
            if(s->maps[i].active&&s->maps[i].base==a0&&a1<=s->maps[i].length) {
                s->maps[i].prot=(U32)a2;
                return 0;
            }
        }
        return -22;
    }

    if(nr==LX_SYS_FUTEX) {
        U32 op=(U32)(a1&0x7fULL);
        if(a0==0ULL) return -14;
        if(op==1U) { /* FUTEX_WAKE */
            s->futex_wakeups+=(U32)a2;
            return (S64)a2;
        }
        if(op==0U) { /* FUTEX_WAIT: nonblocking compatibility foundation */
            U32 *word=(U32*)(unsigned long long)a0;
            return (*word==(U32)a2)?-11:0;
        }
        return -38;
    }

    if(nr==LX_SYS_EXIT) return (S64)a0;
    return -38;
}

U32 zk_linux_abi_selftest(void) {
    LX_ABI_STATE s;
    zk_linux_abi_init(&s,4242U,12345ULL);

    U32 mask=0U;
    if(zk_linux_abi_dispatch(&s,LX_SYS_GETPID,0,0,0,0)==4242) mask|=1U;

    char msg[4]={'Z','K','\n',0};
    if(zk_linux_abi_dispatch(&s,LX_SYS_WRITE,1,(U64)(unsigned long long)msg,3,0)==3 && s.writes==1U) mask|=2U;

    LX_TIMESPEC ts={0,0};
    if(zk_linux_abi_dispatch(&s,LX_SYS_CLOCK_GETTIME,1,(U64)(unsigned long long)&ts,0,0)==0 &&
       ts.tv_sec==12 && ts.tv_nsec==345000000LL) mask|=4U;

    S64 base=zk_linux_abi_dispatch(&s,LX_SYS_MMAP,0,8192,3,0x22);
    if(base>0 && zk_linux_abi_dispatch(&s,LX_SYS_MPROTECT,(U64)base,4096,1,0)==0 &&
       zk_linux_abi_dispatch(&s,LX_SYS_MUNMAP,(U64)base,8192,0,0)==0) mask|=8U;

    U32 futex=7U;
    if(zk_linux_abi_dispatch(&s,LX_SYS_FUTEX,(U64)(unsigned long long)&futex,1,2,0)==2 &&
       s.futex_wakeups==2U) mask|=16U;

    return mask;
}
