/* SPDX-License-Identifier: MIT
 * ZorixFS minimal native filesystem core.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

#define ZFS_BLOCK 512U
#define ZFS_MAX_FILES 16U

typedef struct __attribute__((packed)) {
    U8 magic[8];
    U32 version;
    U32 block_size;
    U32 total_blocks;
    U32 inode_count;
    U32 next_free_block;
    U32 checksum;
} ZFS_SUPER;

typedef struct __attribute__((packed)) {
    char name[32];
    U32 first_block;
    U32 block_count;
    U32 size;
    U32 mode;
    U32 used;
} ZFS_INODE;

typedef struct {
    U8 *disk;
    U32 blocks;
    ZFS_SUPER *super;
    ZFS_INODE *inodes;
} ZFS;

static void zfs_zero(void *p,U32 n){U8*b=(U8*)p;for(U32 i=0;i<n;++i)b[i]=0;}
static void zfs_copy(void*d,const void*s,U32 n){U8*dd=(U8*)d;const U8*ss=(const U8*)s;for(U32 i=0;i<n;++i)dd[i]=ss[i];}
static U32 zfs_streq(const char*a,const char*b){U32 i=0;while(a[i]&&b[i]){if(a[i]!=b[i])return 0;i++;}return a[i]==b[i];}
static void zfs_name(char*d,const char*s){U32 i=0;while(i<31U&&s[i]){d[i]=s[i];i++;}d[i]=0;}
static U32 zfs_sum(const U8*p,U32 n){U32 s=0x5a6f7269U;for(U32 i=0;i<n;++i)s=(s<<5)^(s>>27)^p[i];return s;}

U32 zk_zfs_format(U8 *disk,U32 blocks){
    if(!disk||blocks<64U) return 0U;
    zfs_zero(disk,blocks*ZFS_BLOCK);
    ZFS_SUPER *s=(ZFS_SUPER*)disk;
    const U8 magic[8]={'Z','O','R','I','X','F','S','1'};
    for(U32 i=0;i<8U;++i)s->magic[i]=magic[i];
    s->version=1U;s->block_size=ZFS_BLOCK;s->total_blocks=blocks;s->inode_count=ZFS_MAX_FILES;
    U32 inode_bytes=(U32)sizeof(ZFS_INODE)*ZFS_MAX_FILES;
    U32 inode_blocks=(inode_bytes+ZFS_BLOCK-1U)/ZFS_BLOCK;
    s->next_free_block=1U+inode_blocks;
    s->checksum=0U;
    s->checksum=zfs_sum((const U8*)s,(U32)sizeof(*s));
    return 1U;
}

U32 zk_zfs_mount(ZFS *fs,U8 *disk,U32 blocks){
    if(!fs||!disk) return 0U;
    ZFS_SUPER *s=(ZFS_SUPER*)disk;
    const U8 magic[8]={'Z','O','R','I','X','F','S','1'};
    for(U32 i=0;i<8U;++i)if(s->magic[i]!=magic[i])return 0U;
    if(s->version!=1U||s->block_size!=ZFS_BLOCK||s->total_blocks!=blocks||s->inode_count!=ZFS_MAX_FILES)return 0U;
    U32 saved=s->checksum;s->checksum=0U;U32 calc=zfs_sum((const U8*)s,(U32)sizeof(*s));s->checksum=saved;
    if(saved!=calc)return 0U;
    fs->disk=disk;fs->blocks=blocks;fs->super=s;fs->inodes=(ZFS_INODE*)(disk+ZFS_BLOCK);
    return 1U;
}

ZFS_INODE *zk_zfs_find(ZFS *fs,const char *name){
    if(!fs||!name)return (ZFS_INODE*)0;
    for(U32 i=0;i<ZFS_MAX_FILES;++i)if(fs->inodes[i].used&&zfs_streq(fs->inodes[i].name,name))return &fs->inodes[i];
    return (ZFS_INODE*)0;
}

ZFS_INODE *zk_zfs_create(ZFS *fs,const char *name,U32 capacity,U32 mode){
    if(!fs||!name||capacity==0U||zk_zfs_find(fs,name))return (ZFS_INODE*)0;
    U32 need=(capacity+ZFS_BLOCK-1U)/ZFS_BLOCK;
    if(fs->super->next_free_block+need>fs->blocks)return (ZFS_INODE*)0;
    for(U32 i=0;i<ZFS_MAX_FILES;++i){
        if(fs->inodes[i].used)continue;
        ZFS_INODE *ino=&fs->inodes[i];
        zfs_zero(ino,sizeof(*ino));zfs_name(ino->name,name);ino->used=1U;ino->mode=mode;
        ino->first_block=fs->super->next_free_block;ino->block_count=need;
        fs->super->next_free_block+=need;
        return ino;
    }
    return (ZFS_INODE*)0;
}

U32 zk_zfs_write(ZFS *fs,ZFS_INODE *ino,const void *data,U32 size){
    if(!fs||!ino||!ino->used||!data||size>ino->block_count*ZFS_BLOCK)return 0U;
    zfs_copy(fs->disk+ino->first_block*ZFS_BLOCK,data,size);ino->size=size;return size;
}

U32 zk_zfs_read(ZFS *fs,const ZFS_INODE *ino,void *out,U32 cap){
    if(!fs||!ino||!ino->used||!out)return 0U;
    U32 n=ino->size<cap?ino->size:cap;zfs_copy(out,fs->disk+ino->first_block*ZFS_BLOCK,n);return n;
}

U32 zk_zfs_selftest(void){
    static U8 disk[128U*ZFS_BLOCK];
    ZFS fs;U8 out[16];const U8 in[10]={'Z','o','r','i','x','F','S','!','\n',0};
    if(!zk_zfs_format(disk,128U)||!zk_zfs_mount(&fs,disk,128U))return 0U;
    ZFS_INODE *f=zk_zfs_create(&fs,"/system/config",1024U,0644U);
    if(!f||zk_zfs_write(&fs,f,in,sizeof(in))!=sizeof(in)||zk_zfs_read(&fs,f,out,sizeof(out))!=sizeof(in))return 0U;
    for(U32 i=0;i<sizeof(in);++i)if(in[i]!=out[i])return 0U;
    return 1U;
}
