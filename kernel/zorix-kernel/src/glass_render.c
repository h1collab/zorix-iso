/* SPDX-License-Identifier: MIT
 * Zorix Native Glass software compositor primitives.
 * Pixel-level rendering: alpha, rounded corners, gradients, backdrop sampling,
 * glass tint, shadow/highlight edges, and a tiny built-in bitmap font.
 */
typedef unsigned char U8;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef unsigned long long UN;

static U32 zr_pack(U8 r,U8 g,U8 b,U32 format){
    if(format==0U) return (U32)r|((U32)g<<8)|((U32)b<<16);
    return (U32)b|((U32)g<<8)|((U32)r<<16);
}

static void zr_unpack(U32 p,U32 format,U8 *r,U8 *g,U8 *b){
    if(format==0U){*r=(U8)(p&0xffU);*g=(U8)((p>>8)&0xffU);*b=(U8)((p>>16)&0xffU);}
    else {*b=(U8)(p&0xffU);*g=(U8)((p>>8)&0xffU);*r=(U8)((p>>16)&0xffU);}
}

static U32 zr_blend(U32 dst,U32 format,U8 r,U8 g,U8 b,U8 a){
    U8 dr,dg,db;
    zr_unpack(dst,format,&dr,&dg,&db);
    U32 inv=255U-(U32)a;
    return zr_pack(
        (U8)(((U32)dr*inv+(U32)r*a+127U)/255U),
        (U8)(((U32)dg*inv+(U32)g*a+127U)/255U),
        (U8)(((U32)db*inv+(U32)b*a+127U)/255U),
        format);
}

static U32 zr_inside(U32 x,U32 y,U32 x0,U32 y0,U32 x1,U32 y1,U32 radius){
    if(x<x0||y<y0||x>=x1||y>=y1)return 0U;
    U32 w=x1-x0,h=y1-y0;
    if(radius*2U>w)radius=w/2U;
    if(radius*2U>h)radius=h/2U;
    if(radius==0U)return 1U;
    if(x>=x0+radius&&x<x1-radius)return 1U;
    if(y>=y0+radius&&y<y1-radius)return 1U;
    int cx=(x<x0+radius)?(int)(x0+radius-1U):(int)(x1-radius);
    int cy=(y<y0+radius)?(int)(y0+radius-1U):(int)(y1-radius);
    int dx=(int)x-cx,dy=(int)y-cy;
    return (U32)(dx*dx+dy*dy)<=radius*radius;
}

void zr_gradient(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
                 U32 x0,U32 y0,U32 x1,U32 y1,U32 top,U32 bottom){
    if(!fb||x0>=width||y0>=height)return;
    if(x1>width)x1=width;if(y1>height)y1=height;
    if(x1<=x0||y1<=y0)return;
    int tr=(int)((top>>16)&0xffU),tg=(int)((top>>8)&0xffU),tb=(int)(top&0xffU);
    int br=(int)((bottom>>16)&0xffU),bg=(int)((bottom>>8)&0xffU),bb=(int)(bottom&0xffU);
    U32 h=y1-y0;
    for(U32 y=y0;y<y1;++y){
        U32 t=y-y0;
        U8 r=(U8)(tr+((br-tr)*(int)t)/(int)h);
        U8 g=(U8)(tg+((bg-tg)*(int)t)/(int)h);
        U8 b=(U8)(tb+((bb-tb)*(int)t)/(int)h);
        U32 c=zr_pack(r,g,b,format);
        U32 *row=fb+(UN)y*stride;
        for(U32 x=x0;x<x1;++x)row[x]=c;
    }
}

void zr_round(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
              U32 x0,U32 y0,U32 x1,U32 y1,U32 color,U8 alpha,U32 radius){
    if(!fb||x0>=width||y0>=height)return;
    if(x1>width)x1=width;if(y1>height)y1=height;
    U8 r=(U8)((color>>16)&0xffU),g=(U8)((color>>8)&0xffU),b=(U8)(color&0xffU);
    for(U32 y=y0;y<y1;++y){
        U32 *row=fb+(UN)y*stride;
        for(U32 x=x0;x<x1;++x)
            if(zr_inside(x,y,x0,y0,x1,y1,radius))row[x]=zr_blend(row[x],format,r,g,b,alpha);
    }
}

static U32 zr_sample(U32 *fb,U32 stride,U32 width,U32 height,U32 x,U32 y){
    if(x>=width)x=width-1U;if(y>=height)y=height-1U;
    return fb[(UN)y*stride+x];
}

void zr_glass(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
              U32 x0,U32 y0,U32 x1,U32 y1,U32 tint,U8 alpha,
              U32 radius,U32 blur,U8 border){
    if(!fb||x0>=width||y0>=height)return;
    if(x1>width)x1=width;if(y1>height)y1=height;
    if(x1<=x0||y1<=y0)return;
    if(blur>14U)blur=14U;
    U8 tr=(U8)((tint>>16)&0xffU),tg=(U8)((tint>>8)&0xffU),tb=(U8)(tint&0xffU);

    /* Soft drop shadow behind the rounded panel. */
    U32 sx0=x0>12U?x0-12U:0U,sy0=y0>8U?y0-8U:0U;
    U32 sx1=x1+12U<width?x1+12U:width,sy1=y1+16U<height?y1+16U:height;
    for(U32 y=sy0;y<sy1;++y){
        U32 *row=fb+(UN)y*stride;
        for(U32 x=sx0;x<sx1;++x){
            U32 shadow=zr_inside(x,y,x0+4U,y0+7U,x1+4U,y1+9U,radius+5U);
            U32 panel=zr_inside(x,y,x0,y0,x1,y1,radius);
            if(shadow&&!panel)row[x]=zr_blend(row[x],format,0U,4U,12U,48U);
        }
    }

    /* Approximate backdrop blur with nine taps, then translucent tint. */
    for(U32 y=y0;y<y1;++y){
        U32 *row=fb+(UN)y*stride;
        for(U32 x=x0;x<x1;++x){
            if(!zr_inside(x,y,x0,y0,x1,y1,radius))continue;
            U32 xm=x>blur?x-blur:0U,xp=x+blur<width?x+blur:width-1U;
            U32 ym=y>blur?y-blur:0U,yp=y+blur<height?y+blur:height-1U;
            U32 qx=x>blur/2U?x-blur/2U:0U,qxp=x+blur/2U<width?x+blur/2U:width-1U;
            U32 qy=y>blur/2U?y-blur/2U:0U,qyp=y+blur/2U<height?y+blur/2U:height-1U;
            U32 samples[9]={
                zr_sample(fb,stride,width,height,x,y),
                zr_sample(fb,stride,width,height,xm,y),
                zr_sample(fb,stride,width,height,xp,y),
                zr_sample(fb,stride,width,height,x,ym),
                zr_sample(fb,stride,width,height,x,yp),
                zr_sample(fb,stride,width,height,qx,qy),
                zr_sample(fb,stride,width,height,qxp,qy),
                zr_sample(fb,stride,width,height,qx,qyp),
                zr_sample(fb,stride,width,height,qxp,qyp)
            };
            U32 sr=0U,sg=0U,sb=0U;
            for(U32 n=0;n<9U;++n){U8 rr,gg,bb;zr_unpack(samples[n],format,&rr,&gg,&bb);sr+=rr;sg+=gg;sb+=bb;}
            U32 blurred=zr_pack((U8)(sr/9U),(U8)(sg/9U),(U8)(sb/9U),format);
            row[x]=zr_blend(blurred,format,tr,tg,tb,alpha);

            U32 inset=2U;
            U32 inner=0U;
            if(x1-x0>inset*2U&&y1-y0>inset*2U)
                inner=zr_inside(x,y,x0+inset,y0+inset,x1-inset,y1-inset,radius>inset?radius-inset:0U);
            if(!inner)row[x]=zr_blend(row[x],format,225U,245U,255U,border);
            else if(y<y0+4U)row[x]=zr_blend(row[x],format,255U,255U,255U,(U8)(border/2U));
        }
    }
}

static const U8 letters[26][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
 {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},
 {31,4,4,4,4,4,31},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
 {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
 {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static const U8 digits[10][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},
 {31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
};

static U8 zr_glyph(char ch,U32 row){
    if(ch>='A'&&ch<='Z')return letters[(U32)(ch-'A')][row];
    if(ch>='a'&&ch<='z')return letters[(U32)(ch-'a')][row];
    if(ch>='0'&&ch<='9')return digits[(U32)(ch-'0')][row];
    if(ch=='-')return row==3U?31U:0U;
    if(ch=='.')return row==6U?4U:0U;
    if(ch==':')return (row==2U||row==5U)?4U:0U;
    return 0U;
}

void zr_text(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
             const char *text,U32 x,U32 y,U32 color,U32 scale){
    if(!fb||!text)return;
    if(scale<1U)scale=1U;if(scale>4U)scale=4U;
    U8 r=(U8)((color>>16)&0xffU),g=(U8)((color>>8)&0xffU),b=(U8)(color&0xffU);
    U32 cursor=x;
    for(U32 ci=0;ci<64U;++ci){
        char ch=text[ci];if(ch==0)break;
        if(ch==' '){cursor+=4U*scale;continue;}
        for(U32 gy=0;gy<7U;++gy){
            U8 bits=zr_glyph(ch,gy);
            for(U32 gx=0;gx<5U;++gx){
                if(bits&(1U<<(4U-gx))){
                    U32 px=cursor+gx*scale,py=y+gy*scale;
                    if(px<width&&py<height){
                        for(U32 sy=0;sy<scale&&py+sy<height;++sy){
                            U32 *row=fb+(UN)(py+sy)*stride;
                            for(U32 sx=0;sx<scale&&px+sx<width;++sx)
                                row[px+sx]=zr_blend(row[px+sx],format,r,g,b,238U);
                        }
                    }
                }
            }
        }
        cursor+=6U*scale;
    }
}
