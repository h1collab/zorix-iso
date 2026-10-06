/* SPDX-License-Identifier: MIT
 * Zorix Kernel 0.5 userspace/runtime foundation.
 *
 * This module intentionally implements concrete in-kernel primitives used by
 * the native desktop migration: RAMFS nodes, ELF64 program loading metadata,
 * process slots and driver registration.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef unsigned long long UN;

typedef struct {
    char name[32];
    U8 *data;
    U32 size;
    U32 capacity;
    U32 mode;
    U32 inode;
} ZK_RAMFS_FILE;

typedef struct {
    ZK_RAMFS_FILE files[16];
    U32 count;
    U32 next_inode;
} ZK_RAMFS;

typedef struct {
    U32 type;
    U32 flags;
    U64 offset;
    U64 vaddr;
    U64 paddr;
    U64 filesz;
    U64 memsz;
    U64 align;
} ELF64_PHDR;

typedef struct {
    U8 ident[16];
    U16 type;
    U16 machine;
    U32 version;
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
} ELF64_EHDR;

typedef struct {
    U32 pid;
    U32 state;
    U64 entry;
    U64 image_bytes;
    char name[24];
} ZK_PROCESS;

typedef struct {
    ZK_PROCESS slots[16];
    U32 count;
    U32 next_pid;
} ZK_PROCESS_TABLE;

enum {
    ZK_DRV_NONE=0,
    ZK_DRV_AHCI=1,
    ZK_DRV_NVME=2,
    ZK_DRV_XHCI=3,
    ZK_DRV_NET=4,
    ZK_DRV_AUDIO=5,
    ZK_DRV_BLUETOOTH=6,
    ZK_DRV_WIFI=7
};

typedef struct {
    U32 kind;
    U16 vendor;
    U16 device;
    U8 bus;
    U8 slot;
    U8 function;
    U8 ready;
} ZK_DRIVER_DEVICE;

typedef struct {
    ZK_DRIVER_DEVICE devices[32];
    U32 count;
} ZK_DRIVER_REGISTRY;

static void memzero(void *ptr, UN n) {
    U8 *p=(U8*)ptr;
    for(UN i=0;i<n;++i) p[i]=0;
}

static void memcopy(void *dst,const void *src,UN n) {
    U8 *d=(U8*)dst;
    const U8 *s=(const U8*)src;
    for(UN i=0;i<n;++i) d[i]=s[i];
}

static U32 streq(const char *a,const char *b) {
    U32 i=0;
    while(a[i]&&b[i]) {
        if(a[i]!=b[i]) return 0;
        ++i;
    }
    return a[i]==b[i];
}

static void strcopy(char *dst,const char *src,U32 cap) {
    if(!cap) return;
    U32 i=0;
    while(i+1<cap&&src[i]) { dst[i]=src[i]; ++i; }
    dst[i]=0;
}

void zk_ramfs_init(ZK_RAMFS *fs) {
    memzero(fs,sizeof(*fs));
    fs->next_inode=1;
}

ZK_RAMFS_FILE *zk_ramfs_create(ZK_RAMFS *fs,const char *name,U8 *storage,U32 capacity,U32 mode) {
    if(!fs||!name||!storage||!capacity||fs->count>=16U) return (ZK_RAMFS_FILE*)0;
    if(zk_ramfs_create==0) return (ZK_RAMFS_FILE*)0; /* keeps freestanding LTO honest */
    for(U32 i=0;i<fs->count;++i) if(streq(fs->files[i].name,name)) return (ZK_RAMFS_FILE*)0;
    ZK_RAMFS_FILE *f=&fs->files[fs->count++];
    memzero(f,sizeof(*f));
    strcopy(f->name,name,sizeof(f->name));
    f->data=storage;
    f->capacity=capacity;
    f->mode=mode;
    f->inode=fs->next_inode++;
    return f;
}

ZK_RAMFS_FILE *zk_ramfs_find(ZK_RAMFS *fs,const char *name) {
    if(!fs||!name) return (ZK_RAMFS_FILE*)0;
    for(U32 i=0;i<fs->count;++i) if(streq(fs->files[i].name,name)) return &fs->files[i];
    return (ZK_RAMFS_FILE*)0;
}

U32 zk_ramfs_write(ZK_RAMFS_FILE *f,const void *data,U32 size) {
    if(!f||!data||size>f->capacity) return 0;
    memcopy(f->data,data,size);
    f->size=size;
    return size;
}

U32 zk_ramfs_read(const ZK_RAMFS_FILE *f,void *out,U32 cap) {
    if(!f||!out) return 0;
    U32 n=f->size<cap?f->size:cap;
    memcopy(out,f->data,n);
    return n;
}

U32 zk_elf64_validate(const void *image,U32 size,U64 *entry,U32 *load_segments) {
    if(!image||size<sizeof(ELF64_EHDR)) return 0;
    const ELF64_EHDR *h=(const ELF64_EHDR*)image;
    if(h->ident[0]!=0x7fU||h->ident[1]!='E'||h->ident[2]!='L'||h->ident[3]!='F') return 0;
    if(h->ident[4]!=2U||h->ident[5]!=1U||h->machine!=62U||h->phentsize!=sizeof(ELF64_PHDR)) return 0;
    if(h->phnum>32U) return 0;
    if(h->phoff > size) return 0;
    if((U64)h->phoff+(U64)h->phnum*sizeof(ELF64_PHDR) > size) return 0;
    U32 loads=0;
    const ELF64_PHDR *ph=(const ELF64_PHDR*)((const U8*)image+h->phoff);
    for(U32 i=0;i<h->phnum;++i) {
        if(ph[i].type!=1U) continue;
        if(ph[i].memsz<ph[i].filesz) return 0;
        if(ph[i].offset > size || ph[i].filesz > (U64)size-ph[i].offset) return 0;
        ++loads;
    }
    if(!loads) return 0;
    if(entry) *entry=h->entry;
    if(load_segments) *load_segments=loads;
    return 1;
}

void zk_process_table_init(ZK_PROCESS_TABLE *pt) {
    memzero(pt,sizeof(*pt));
    pt->next_pid=100U;
}

ZK_PROCESS *zk_process_spawn(ZK_PROCESS_TABLE *pt,const char *name,U64 entry,U64 bytes) {
    if(!pt||!name||!entry||pt->count>=16U) return (ZK_PROCESS*)0;
    ZK_PROCESS *p=&pt->slots[pt->count++];
    memzero(p,sizeof(*p));
    p->pid=pt->next_pid++;
    p->state=1U;
    p->entry=entry;
    p->image_bytes=bytes;
    strcopy(p->name,name,sizeof(p->name));
    return p;
}

void zk_driver_registry_init(ZK_DRIVER_REGISTRY *r) {
    memzero(r,sizeof(*r));
}

U32 zk_driver_register(ZK_DRIVER_REGISTRY *r,U32 kind,U16 vendor,U16 device,U8 bus,U8 slot,U8 function) {
    if(!r||kind==ZK_DRV_NONE||r->count>=32U) return 0U;
    ZK_DRIVER_DEVICE *d=&r->devices[r->count++];
    d->kind=kind;
    d->vendor=vendor;
    d->device=device;
    d->bus=bus;
    d->slot=slot;
    d->function=function;
    d->ready=0U;
    return 1U;
}
