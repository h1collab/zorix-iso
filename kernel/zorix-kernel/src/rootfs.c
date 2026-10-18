/* SPDX-License-Identifier: MIT
 * Zorix Package Archive (ZPK) read-only native root filesystem.
 */
typedef unsigned char U8;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef struct __attribute__((packed)) {
    U8 magic[8];      /* "ZORIXPK1" */
    U32 version;      /* 1 */
    U32 count;
} ZPK_HDR;

typedef struct __attribute__((packed)) {
    char path[120];
    U64 offset;
    U64 size;
    U32 type;
    U32 flags;
} ZPK_ENTRY;

static const U8 *g_zpk;
static U64 g_zpk_bytes;
static const ZPK_ENTRY *g_entries;
static U32 g_count;

static U32 zpk_streq(const char *a,const char *b){
    if(!a||!b)return 0U;
    for(U32 i=0;i<240U;++i){
        if(a[i]!=b[i])return 0U;
        if(a[i]==0)return 1U;
    }
    return 0U;
}

U32 zk_rootfs_mount(const void *blob,U64 bytes){
    g_zpk=(const U8*)0;g_zpk_bytes=0;g_entries=(const ZPK_ENTRY*)0;g_count=0;
    if(!blob||bytes<sizeof(ZPK_HDR))return 0U;
    const ZPK_HDR*h=(const ZPK_HDR*)blob;
    static const U8 magic[8]={'Z','O','R','I','X','P','K','1'};
    for(U32 i=0;i<8U;++i)if(h->magic[i]!=magic[i])return 0U;
    if(h->version!=1U||h->count>8192U)return 0U;
    U64 index_end=sizeof(ZPK_HDR)+(U64)h->count*sizeof(ZPK_ENTRY);
    if(index_end>bytes)return 0U;
    const ZPK_ENTRY*e=(const ZPK_ENTRY*)((const U8*)blob+sizeof(ZPK_HDR));
    for(U32 i=0;i<h->count;++i){
        if(e[i].offset<index_end||e[i].offset>bytes||e[i].size>bytes-e[i].offset)return 0U;
        if(e[i].path[0]!='/')return 0U;
    }
    g_zpk=(const U8*)blob;g_zpk_bytes=bytes;g_entries=e;g_count=h->count;
    return 1U;
}

U32 zk_rootfs_count(void){return g_count;}

const char *zk_rootfs_path(U32 index){
    if(index>=g_count)return (const char*)0;
    return g_entries[index].path;
}

const void *zk_rootfs_data(U32 index,U64 *size){
    if(index>=g_count||!g_zpk)return (const void*)0;
    if(size)*size=g_entries[index].size;
    return g_zpk+g_entries[index].offset;
}

const void *zk_rootfs_find(const char *path,U64 *size){
    if(!path||!g_zpk)return (const void*)0;
    for(U32 i=0;i<g_count;++i){
        if(zpk_streq(g_entries[i].path,path))return zk_rootfs_data(i,size);
    }
    return (const void*)0;
}
