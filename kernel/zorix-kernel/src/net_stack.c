/* SPDX-License-Identifier: MIT
 * Zorix native Ethernet/ARP/IPv4/UDP foundation.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

typedef struct __attribute__((packed)){U8 dst[6],src[6];U16 type;} ETH;
typedef struct __attribute__((packed)){U16 htype,ptype;U8 hlen,plen;U16 oper;U8 sha[6];U8 spa[4];U8 tha[6];U8 tpa[4];} ARP;
typedef struct __attribute__((packed)){U8 ver_ihl,tos;U16 total,id,frag;U8 ttl,proto;U16 checksum;U8 src[4],dst[4];} IPV4;
typedef struct __attribute__((packed)){U16 src,dst,len,checksum;} UDP;

static U16 nbe16(U16 v){return(U16)((v<<8)|(v>>8));}
static U16 ip_checksum(const void *p,U32 bytes){
    const U8*b=(const U8*)p;U32 sum=0;
    for(U32 i=0;i+1<bytes;i+=2)sum+=((U32)b[i]<<8)|b[i+1];
    if(bytes&1U)sum+=(U32)b[bytes-1]<<8;
    while(sum>>16)sum=(sum&0xffffU)+(sum>>16);
    return(U16)~sum;
}
static void copy6(U8*d,const U8*s){for(U32 i=0;i<6U;++i)d[i]=s[i];}
static void copy4(U8*d,const U8*s){for(U32 i=0;i<4U;++i)d[i]=s[i];}

U32 zk_net_build_arp_request(U8*out,U32 cap,const U8 mac[6],const U8 ip[4],const U8 target[4]){
    if(!out||cap<sizeof(ETH)+sizeof(ARP))return 0U;
    ETH*e=(ETH*)out;for(U32 i=0;i<6U;++i)e->dst[i]=0xffU;copy6(e->src,mac);e->type=nbe16(0x0806U);
    ARP*a=(ARP*)(out+sizeof(ETH));a->htype=nbe16(1U);a->ptype=nbe16(0x0800U);a->hlen=6U;a->plen=4U;a->oper=nbe16(1U);
    copy6(a->sha,mac);copy4(a->spa,ip);for(U32 i=0;i<6U;++i)a->tha[i]=0;copy4(a->tpa,target);
    return sizeof(ETH)+sizeof(ARP);
}

U32 zk_net_build_udp(U8*out,U32 cap,const U8 dmac[6],const U8 smac[6],const U8 sip[4],const U8 dip[4],U16 sport,U16 dport,const U8*payload,U32 len){
    U32 need=sizeof(ETH)+sizeof(IPV4)+sizeof(UDP)+len;if(!out||!payload||cap<need)return 0U;
    ETH*e=(ETH*)out;copy6(e->dst,dmac);copy6(e->src,smac);e->type=nbe16(0x0800U);
    IPV4*ip=(IPV4*)(out+sizeof(ETH));U8*z=(U8*)ip;for(U32 i=0;i<sizeof(IPV4);++i)z[i]=0;
    ip->ver_ihl=0x45U;ip->total=nbe16((U16)(sizeof(IPV4)+sizeof(UDP)+len));ip->id=nbe16(1U);ip->ttl=64U;ip->proto=17U;copy4(ip->src,sip);copy4(ip->dst,dip);
    ip->checksum=ip_checksum(ip,sizeof(IPV4));
    UDP*u=(UDP*)((U8*)ip+sizeof(IPV4));u->src=nbe16(sport);u->dst=nbe16(dport);u->len=nbe16((U16)(sizeof(UDP)+len));u->checksum=0U;
    U8*dst=(U8*)u+sizeof(UDP);for(U32 i=0;i<len;++i)dst[i]=payload[i];return need;
}

U32 zk_net_parse_ipv4_udp(const U8*frame,U32 len,U16*dst_port,U32*payload_len){
    if(!frame||len<sizeof(ETH)+sizeof(IPV4)+sizeof(UDP))return 0U;
    const ETH*e=(const ETH*)frame;if(e->type!=nbe16(0x0800U))return 0U;
    const IPV4*ip=(const IPV4*)(frame+sizeof(ETH));if(ip->ver_ihl!=0x45U||ip->proto!=17U)return 0U;
    IPV4 tmp=*ip;U16 saved=tmp.checksum;tmp.checksum=0U;if(saved!=ip_checksum(&tmp,sizeof(tmp)))return 0U;
    const UDP*u=(const UDP*)((const U8*)ip+sizeof(IPV4));U16 ulen=nbe16(u->len);if(ulen<sizeof(UDP)||sizeof(ETH)+sizeof(IPV4)+ulen>len)return 0U;
    if(dst_port)*dst_port=nbe16(u->dst);if(payload_len)*payload_len=ulen-sizeof(UDP);return 1U;
}

U32 zk_net_selftest(void){
    U8 frame[256];const U8 mac[6]={0x52,0x54,0,0x12,0x34,0x56};const U8 dmac[6]={0x52,0x54,0,0xab,0xcd,0xef};
    const U8 ip[4]={10,0,2,15},gw[4]={10,0,2,2};const U8 p[5]={'H','E','L','L','O'};
    U32 mask=0;U32 n=zk_net_build_arp_request(frame,sizeof(frame),mac,ip,gw);if(n==sizeof(ETH)+sizeof(ARP)&&((ETH*)frame)->type==nbe16(0x0806U))mask|=1U;
    n=zk_net_build_udp(frame,sizeof(frame),dmac,mac,ip,gw,49152U,53U,p,sizeof(p));U16 port=0;U32 plen=0;
    if(n&&zk_net_parse_ipv4_udp(frame,n,&port,&plen)&&port==53U&&plen==5U)mask|=2U;
    return mask;
}
