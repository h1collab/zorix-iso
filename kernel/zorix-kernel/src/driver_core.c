/* SPDX-License-Identifier: MIT
 * Zorix native driver command-path foundations.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef struct __attribute__((packed)) {
    U8 fis_type;
    U8 pmport_c;
    U8 command;
    U8 featurel;
    U8 lba0,lba1,lba2;
    U8 device;
    U8 lba3,lba4,lba5;
    U8 featureh;
    U8 countl,counth;
    U8 icc;
    U8 control;
    U8 reserved[4];
} ZK_AHCI_FIS_H2D;

typedef struct {
    U8 opcode;
    U8 flags;
    U16 cid;
    U32 nsid;
    U64 reserved;
    U64 mptr;
    U64 prp1;
    U64 prp2;
    U32 cdw10;
    U32 cdw11;
    U32 cdw12;
    U32 cdw13;
    U32 cdw14;
    U32 cdw15;
} ZK_NVME_CMD;

typedef struct {
    U64 parameter;
    U32 status;
    U32 control;
} ZK_XHCI_TRB;

typedef struct __attribute__((packed)) {
    U8 flags;
    U8 gso_type;
    U16 hdr_len;
    U16 gso_size;
    U16 csum_start;
    U16 csum_offset;
} ZK_VIRTIO_NET_HDR;

typedef struct __attribute__((packed)) {
    U8 dst[6];
    U8 src[6];
    U16 ethertype;
} ZK_ETH_HDR;

static U16 be16(U16 v) { return (U16)((v<<8)|(v>>8)); }

static void ahci_build_read(ZK_AHCI_FIS_H2D *f,U64 lba,U16 sectors) {
    U8 *p=(U8*)f;
    for(U32 i=0;i<sizeof(*f);++i) p[i]=0;
    f->fis_type=0x27U;
    f->pmport_c=0x80U;
    f->command=0x25U; /* READ DMA EXT */
    f->device=0x40U;
    f->lba0=(U8)lba; f->lba1=(U8)(lba>>8); f->lba2=(U8)(lba>>16);
    f->lba3=(U8)(lba>>24); f->lba4=(U8)(lba>>32); f->lba5=(U8)(lba>>40);
    f->countl=(U8)sectors; f->counth=(U8)(sectors>>8);
}

static void nvme_build_read(ZK_NVME_CMD *c,U16 cid,U32 nsid,U64 prp,U64 lba,U16 blocks) {
    U8 *p=(U8*)c;
    for(U32 i=0;i<sizeof(*c);++i) p[i]=0;
    c->opcode=0x02U;
    c->cid=cid;
    c->nsid=nsid;
    c->prp1=prp;
    c->cdw10=(U32)lba;
    c->cdw11=(U32)(lba>>32);
    c->cdw12=(U32)(blocks-1U);
}

static ZK_XHCI_TRB xhci_normal_trb(U64 buffer,U32 length,U32 cycle) {
    ZK_XHCI_TRB t;
    t.parameter=buffer;
    t.status=length&0x1ffffU;
    t.control=(1U<<10)|(cycle&1U); /* Normal TRB */
    return t;
}

static U32 hda_verb(U8 codec,U8 node,U16 verb,U8 payload) {
    return ((U32)(codec&0x0fU)<<28)|((U32)node<<20)|((U32)(verb&0x0fffU)<<8)|payload;
}

static U32 virtio_net_frame(U8 *out,U32 cap,const U8 dst[6],const U8 src[6],U16 type,const U8 *payload,U32 len) {
    U32 need=(U32)sizeof(ZK_VIRTIO_NET_HDR)+(U32)sizeof(ZK_ETH_HDR)+len;
    if(!out||!dst||!src||(!payload&&len)||cap<need) return 0U;
    for(U32 i=0;i<sizeof(ZK_VIRTIO_NET_HDR);++i) out[i]=0;
    ZK_ETH_HDR *e=(ZK_ETH_HDR*)(out+sizeof(ZK_VIRTIO_NET_HDR));
    for(U32 i=0;i<6U;++i){ e->dst[i]=dst[i]; e->src[i]=src[i]; }
    e->ethertype=be16(type);
    for(U32 i=0;i<len;++i) out[sizeof(ZK_VIRTIO_NET_HDR)+sizeof(ZK_ETH_HDR)+i]=payload[i];
    return need;
}

static U16 wifi_data_frame_control(U32 to_ds,U32 from_ds,U32 protected_frame) {
    U16 fc=0x0008U; /* data */
    if(to_ds) fc|=0x0100U;
    if(from_ds) fc|=0x0200U;
    if(protected_frame) fc|=0x4000U;
    return fc;
}

static U32 bt_hci_reset(U8 out[4]) {
    if(!out) return 0U;
    out[0]=0x01U; /* HCI command packet */
    out[1]=0x03U; out[2]=0x0cU; /* OGF Controller/Baseband, OCF Reset */
    out[3]=0x00U;
    return 4U;
}

U32 zk_driver_core_selftest(void) {
    U32 mask=0U;

    ZK_AHCI_FIS_H2D fis;
    ahci_build_read(&fis,0x1122334455ULL,8U);
    if(fis.fis_type==0x27U&&fis.command==0x25U&&fis.lba0==0x55U&&fis.lba4==0x11U&&fis.countl==8U) mask|=1U;

    ZK_NVME_CMD nv;
    nvme_build_read(&nv,7U,1U,0x100000ULL,0x12345678ULL,16U);
    if(nv.opcode==0x02U&&nv.cid==7U&&nv.nsid==1U&&nv.prp1==0x100000ULL&&nv.cdw10==0x12345678U&&nv.cdw12==15U) mask|=2U;

    ZK_XHCI_TRB trb=xhci_normal_trb(0x200000ULL,4096U,1U);
    if(trb.parameter==0x200000ULL&&(trb.status&0x1ffffU)==4096U&&((trb.control>>10)&0x3fU)==1U&&(trb.control&1U)) mask|=4U;

    U32 verb=hda_verb(0U,2U,0xf00U,0x04U);
    if(((verb>>20)&0xffU)==2U&&((verb>>8)&0xfffU)==0xf00U&&(verb&0xffU)==0x04U) mask|=8U;

    U8 frame[128];
    const U8 dst[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    const U8 src[6]={0x52,0x54,0x00,0x12,0x34,0x56};
    const U8 payload[4]={'Z','N','E','T'};
    U32 frame_len=virtio_net_frame(frame,sizeof(frame),dst,src,0x0800U,payload,sizeof(payload));
    if(frame_len==sizeof(ZK_VIRTIO_NET_HDR)+sizeof(ZK_ETH_HDR)+4U) mask|=16U;

    U16 wifi=wifi_data_frame_control(1U,0U,1U);
    if((wifi&0x000cU)==0x0008U&&(wifi&0x0100U)&&(wifi&0x4000U)) mask|=32U;

    U8 hci[4];
    if(bt_hci_reset(hci)==4U&&hci[0]==0x01U&&hci[1]==0x03U&&hci[2]==0x0cU&&hci[3]==0U) mask|=64U;

    return mask;
}
