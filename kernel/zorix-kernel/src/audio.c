/* SPDX-License-Identifier: MIT
 * Zorix native audio engine foundation.
 */
typedef unsigned int U32;
typedef int S32;
typedef short S16;

#define AUDIO_FRAMES 1024U

typedef struct {
    S16 l;
    S16 r;
} ZK_AUDIO_FRAME;

typedef struct {
    ZK_AUDIO_FRAME ring[AUDIO_FRAMES];
    U32 read_pos;
    U32 write_pos;
    U32 count;
    U32 sample_rate;
} ZK_AUDIO_QUEUE;

static S16 sat16(S32 v){if(v>32767)return 32767;if(v<-32768)return-32768;return(S16)v;}

void zk_audio_init(ZK_AUDIO_QUEUE*q,U32 rate){if(!q)return;q->read_pos=0;q->write_pos=0;q->count=0;q->sample_rate=rate;}

U32 zk_audio_push(ZK_AUDIO_QUEUE*q,const ZK_AUDIO_FRAME*in,U32 frames){
    if(!q||!in)return 0U;U32 n=0;
    while(n<frames&&q->count<AUDIO_FRAMES){q->ring[q->write_pos]=in[n++];q->write_pos=(q->write_pos+1U)%AUDIO_FRAMES;q->count++;}
    return n;
}

U32 zk_audio_pop(ZK_AUDIO_QUEUE*q,ZK_AUDIO_FRAME*out,U32 frames){
    if(!q||!out)return 0U;U32 n=0;
    while(n<frames&&q->count){out[n++]=q->ring[q->read_pos];q->read_pos=(q->read_pos+1U)%AUDIO_FRAMES;q->count--;}
    return n;
}

void zk_audio_mix(ZK_AUDIO_FRAME*dst,const ZK_AUDIO_FRAME*src,U32 frames,U32 gain_q8){
    if(!dst||!src)return;
    for(U32 i=0;i<frames;++i){
        dst[i].l=sat16((S32)dst[i].l+((S32)src[i].l*(S32)gain_q8>>8));
        dst[i].r=sat16((S32)dst[i].r+((S32)src[i].r*(S32)gain_q8>>8));
    }
}

U32 zk_audio_selftest(void){
    static ZK_AUDIO_QUEUE q;zk_audio_init(&q,48000U);
    ZK_AUDIO_FRAME a[4]={{1000,-1000},{2000,-2000},{3000,-3000},{32000,32000}};
    ZK_AUDIO_FRAME b[4]={{500,500},{500,500},{500,500},{2000,2000}};
    zk_audio_mix(a,b,4U,256U);
    if(a[0].l!=1500||a[0].r!=-500||a[3].l!=32767||a[3].r!=32767)return 0U;
    if(zk_audio_push(&q,a,4U)!=4U||q.count!=4U)return 0U;
    ZK_AUDIO_FRAME out[4];if(zk_audio_pop(&q,out,4U)!=4U||q.count!=0U)return 0U;
    if(out[2].l!=3500||out[2].r!=-2500||q.sample_rate!=48000U)return 0U;
    return 1U;
}
