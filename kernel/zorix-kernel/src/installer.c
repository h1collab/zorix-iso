/* SPDX-License-Identifier: MIT
 * Zorix Native Installer core.
 *
 * Produces and validates a GPT/ESP/root layout plan. Disk I/O backends are
 * supplied by storage drivers; this module owns partition metadata semantics.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef struct __attribute__((packed)) {
    U8 boot;
    U8 chs_first[3];
    U8 type;
    U8 chs_last[3];
    U32 lba_first;
    U32 sectors;
} MBR_PART;

typedef struct __attribute__((packed)) {
    U8 boot_code[440];
    U32 disk_sig;
    U16 reserved;
    MBR_PART part[4];
    U16 signature;
} MBR;

typedef struct __attribute__((packed)) {
    U64 signature;
    U32 revision;
    U32 header_size;
    U32 header_crc32;
    U32 reserved;
    U64 current_lba;
    U64 backup_lba;
    U64 first_usable_lba;
    U64 last_usable_lba;
    U8 disk_guid[16];
    U64 entries_lba;
    U32 entry_count;
    U32 entry_size;
    U32 entries_crc32;
} GPT_HEADER;

typedef struct __attribute__((packed)) {
    U8 type_guid[16];
    U8 unique_guid[16];
    U64 first_lba;
    U64 last_lba;
    U64 attributes;
    U16 name[36];
} GPT_ENTRY;

typedef struct {
    MBR protective_mbr;
    GPT_HEADER primary;
    GPT_ENTRY entries[2];
    U64 esp_first;
    U64 esp_last;
    U64 root_first;
    U64 root_last;
    U32 valid;
} ZK_INSTALL_PLAN;

static void zero_bytes(void *p,U32 n) {
    U8 *b=(U8*)p;
    for(U32 i=0;i<n;++i) b[i]=0;
}

static U32 crc32_bytes(const void *ptr,U32 n) {
    const U8 *p=(const U8*)ptr;
    U32 crc=0xffffffffU;
    for(U32 i=0;i<n;++i) {
        crc^=p[i];
        for(U32 bit=0;bit<8U;++bit) {
            U32 mask=(U32)-(int)(crc&1U);
            crc=(crc>>1)^(0xedb88320U&mask);
        }
    }
    return ~crc;
}

static void guid_copy(U8 dst[16],const U8 src[16]) {
    for(U32 i=0;i<16U;++i) dst[i]=src[i];
}

static void name_ascii(U16 dst[36],const char *src) {
    U32 i=0;
    while(i<35U&&src[i]) { dst[i]=(U16)(U8)src[i]; ++i; }
    while(i<36U) dst[i++]=0;
}

U32 zk_installer_plan_gpt(ZK_INSTALL_PLAN *p,U64 disk_sectors) {
    static const U8 esp_type[16]={0x28,0x73,0x2a,0xc1,0x1f,0xf8,0xd2,0x11,0xba,0x4b,0x00,0xa0,0xc9,0x3e,0xc9,0x3b};
    static const U8 root_type[16]={0xaf,0x3d,0xc6,0x0f,0x83,0x84,0x72,0x47,0x8e,0x79,0x3d,0x69,0xd8,0x47,0x7d,0xe4};
    static const U8 disk_guid[16]={0x5a,0x4f,0x52,0x49,0x58,0x4f,0x53,0x35,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    static const U8 esp_guid[16]={0x5a,0x4f,0x52,0x49,0x58,0x45,0x53,0x50,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x08};
    static const U8 root_guid[16]={0x5a,0x4f,0x52,0x49,0x58,0x52,0x4f,0x4f,0x54,0x01,0x02,0x03,0x04,0x05,0x06,0x09};

    if(!p||disk_sectors<262144ULL) return 0U;
    zero_bytes(p,sizeof(*p));

    p->protective_mbr.part[0].type=0xeeU;
    p->protective_mbr.part[0].lba_first=1U;
    p->protective_mbr.part[0].sectors=(disk_sectors-1ULL>0xffffffffULL)?0xffffffffU:(U32)(disk_sectors-1ULL);
    p->protective_mbr.signature=0xaa55U;

    const U64 first_usable=2048ULL;
    const U64 backup_lba=disk_sectors-1ULL;
    const U64 last_usable=backup_lba-2048ULL;
    const U64 esp_sectors=131072ULL; /* 64 MiB at 512-byte sectors */
    const U64 esp_first=first_usable;
    const U64 esp_last=esp_first+esp_sectors-1ULL;
    const U64 root_first=(esp_last+2048ULL)&~2047ULL;
    if(root_first>=last_usable) return 0U;

    p->esp_first=esp_first; p->esp_last=esp_last;
    p->root_first=root_first; p->root_last=last_usable;

    guid_copy(p->entries[0].type_guid,esp_type);
    guid_copy(p->entries[0].unique_guid,esp_guid);
    p->entries[0].first_lba=esp_first;
    p->entries[0].last_lba=esp_last;
    name_ascii(p->entries[0].name,"Zorix EFI");

    guid_copy(p->entries[1].type_guid,root_type);
    guid_copy(p->entries[1].unique_guid,root_guid);
    p->entries[1].first_lba=root_first;
    p->entries[1].last_lba=last_usable;
    name_ascii(p->entries[1].name,"Zorix System");

    p->primary.signature=0x5452415020494645ULL; /* EFI PART */
    p->primary.revision=0x00010000U;
    p->primary.header_size=92U;
    p->primary.current_lba=1ULL;
    p->primary.backup_lba=backup_lba;
    p->primary.first_usable_lba=first_usable;
    p->primary.last_usable_lba=last_usable;
    guid_copy(p->primary.disk_guid,disk_guid);
    p->primary.entries_lba=2ULL;
    p->primary.entry_count=2U;
    p->primary.entry_size=sizeof(GPT_ENTRY);
    p->primary.entries_crc32=crc32_bytes(p->entries,sizeof(p->entries));
    p->primary.header_crc32=0U;
    p->primary.header_crc32=crc32_bytes(&p->primary,p->primary.header_size);

    p->valid=1U;
    return 1U;
}

U32 zk_installer_selftest(void) {
    ZK_INSTALL_PLAN p;
    if(!zk_installer_plan_gpt(&p,2097152ULL)) return 0U; /* 1 GiB */
    U32 mask=0U;
    if(p.protective_mbr.signature==0xaa55U && p.protective_mbr.part[0].type==0xeeU) mask|=1U;
    if(p.primary.signature==0x5452415020494645ULL && p.primary.header_crc32!=0U && p.primary.entries_crc32!=0U) mask|=2U;
    if(p.esp_first==2048ULL && p.esp_last>p.esp_first && p.root_first>p.esp_last && p.root_last>p.root_first) mask|=4U;
    if(p.primary.backup_lba==2097151ULL && p.primary.last_usable_lba<p.primary.backup_lba) mask|=8U;
    return mask;
}
