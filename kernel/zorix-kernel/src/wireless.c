/* SPDX-License-Identifier: MIT
 * Zorix native Wi-Fi and Bluetooth protocol foundations.
 */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

typedef struct __attribute__((packed)){
    U16 frame_control;
    U16 duration;
    U8 addr1[6],addr2[6],addr3[6];
    U16 seq;
} WIFI_HDR;

typedef struct {
    U8 state;
    U16 last_opcode;
    U8 last_status;
    U32 events;
} BT_HCI_STATE;

static U16 le16(U16 v){return v;}

U32 zk_wifi_parse_data(const U8*frame,U32 len,U32*to_ds,U32*from_ds,U32*protected_frame){
    if(!frame||len<sizeof(WIFI_HDR))return 0U;
    const WIFI_HDR*h=(const WIFI_HDR*)frame;U16 fc=le16(h->frame_control);
    if((fc&0x000cU)!=0x0008U)return 0U;
    if(to_ds)*to_ds=(fc&0x0100U)?1U:0U;if(from_ds)*from_ds=(fc&0x0200U)?1U:0U;if(protected_frame)*protected_frame=(fc&0x4000U)?1U:0U;return 1U;
}

U32 zk_bt_hci_command(U8*out,U32 cap,U16 opcode,const U8*params,U8 plen){
    if(!out||cap<(U32)plen+4U)return 0U;out[0]=0x01U;out[1]=(U8)opcode;out[2]=(U8)(opcode>>8);out[3]=plen;
    for(U32 i=0;i<plen;++i)out[4U+i]=params?params[i]:0U;return(U32)plen+4U;
}

U32 zk_bt_hci_event(BT_HCI_STATE*s,const U8*packet,U32 len){
    if(!s||!packet||len<7U||packet[0]!=0x04U||packet[1]!=0x0eU)return 0U; /* Command Complete */
    U8 plen=packet[2];if((U32)plen+3U>len||plen<4U)return 0U;
    s->last_opcode=(U16)packet[4]|((U16)packet[5]<<8);s->last_status=packet[6];s->events++;s->state=(s->last_status==0U)?1U:2U;return 1U;
}

U32 zk_wireless_selftest(void){
    U32 mask=0U;U8 wifi[sizeof(WIFI_HDR)];for(U32 i=0;i<sizeof(wifi);++i)wifi[i]=0;
    WIFI_HDR*h=(WIFI_HDR*)wifi;h->frame_control=0x4108U;U32 t=0,f=0,p=0;
    if(zk_wifi_parse_data(wifi,sizeof(wifi),&t,&f,&p)&&t==1U&&f==0U&&p==1U)mask|=1U;
    U8 cmd[8];if(zk_bt_hci_command(cmd,sizeof(cmd),0x0c03U,(const U8*)0,0U)==4U&&cmd[0]==1U&&cmd[1]==0x03U&&cmd[2]==0x0cU)mask|=2U;
    BT_HCI_STATE s={0,0,0,0};U8 evt[7]={0x04,0x0e,0x04,0x01,0x03,0x0c,0x00};
    if(zk_bt_hci_event(&s,evt,sizeof(evt))&&s.state==1U&&s.last_opcode==0x0c03U&&s.events==1U)mask|=4U;
    return mask;
}
