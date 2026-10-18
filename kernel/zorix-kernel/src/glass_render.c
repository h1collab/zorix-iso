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

extern void zr_font_text(U32*,U32,U32,U32,U32,const char*,U32,U32,U32,U32);

void zr_text(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
             const char *text,U32 x,U32 y,U32 color,U32 scale){
    zr_font_text(fb,stride,width,height,format,text,x,y,color,scale);
}

static void zr_dot(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
                   int cx,int cy,int radius,U32 color,U8 alpha){
    U8 r=(U8)(color>>16),g=(U8)(color>>8),b=(U8)color;
    int rr=radius*radius;
    for(int y=cy-radius;y<=cy+radius;++y){
        if(y<0||(U32)y>=height)continue;
        U32 *row=fb+(UN)(U32)y*stride;
        for(int x=cx-radius;x<=cx+radius;++x){
            if(x<0||(U32)x>=width)continue;
            int dx=x-cx,dy=y-cy;
            int d=dx*dx+dy*dy;
            if(d<=rr) row[x]=zr_blend(row[x],format,r,g,b,alpha);
            else if(d<=rr+radius+1) row[x]=zr_blend(row[x],format,r,g,b,(U8)(alpha/4U));
        }
    }
}

static void zr_line(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
                    int x0,int y0,int x1,int y1,int thick,U32 color,U8 alpha){
    int dx=x1-x0;if(dx<0)dx=-dx;
    int sx=x0<x1?1:-1;
    int dy=y1-y0;if(dy>0)dy=-dy;
    int sy=y0<y1?1:-1;
    int err=dx+dy;
    for(;;){
        zr_dot(fb,stride,width,height,format,x0,y0,thick,color,alpha);
        if(x0==x1&&y0==y1)break;
        int e2=2*err;
        if(e2>=dy){err+=dy;x0+=sx;}
        if(e2<=dx){err+=dx;y0+=sy;}
    }
}

void zr_symbol(U32 *fb,U32 stride,U32 width,U32 height,U32 format,
               U32 id,U32 x,U32 y,U32 size,U32 color,U8 alpha){
    if(!fb||size<12U)return;
    int cx=(int)(x+size/2U),cy=(int)(y+size/2U);
    int s=(int)size;
    int t=s/18;if(t<1)t=1;

    switch(id){
    case 0: /* files / folder */
        zr_line(fb,stride,width,height,format,(int)x+s/5,(int)y+s/3,(int)x+s*2/5,(int)y+s/3,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s*2/5,(int)y+s/3,(int)x+s/2,(int)y+s*2/5,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s/5,(int)y+s*2/5,(int)x+s*4/5,(int)y+s*2/5,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s/5,(int)y+s*2/5,(int)x+s/4,(int)y+s*3/4,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s/4,(int)y+s*3/4,(int)x+s*3/4,(int)y+s*3/4,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s*3/4,(int)y+s*3/4,(int)x+s*4/5,(int)y+s*2/5,t,color,alpha);
        break;
    case 1: /* settings gear */
        zr_dot(fb,stride,width,height,format,cx,cy,s/4,color,(U8)(alpha/2U));
        zr_dot(fb,stride,width,height,format,cx,cy,s/9,0x0b1727U,220U);
        for(int k=0;k<8;++k){
            static const int vx[8]={0,7,10,7,0,-7,-10,-7};
            static const int vy[8]={-10,-7,0,7,10,7,0,-7};
            zr_line(fb,stride,width,height,format,cx+vx[k]*s/48,cy+vy[k]*s/48,
                    cx+vx[k]*s/32,cy+vy[k]*s/32,t,color,alpha);
        }
        break;
    case 2: /* terminal */
        zr_line(fb,stride,width,height,format,(int)x+s/4,(int)y+s/3,(int)x+s*2/5,(int)y+s/2,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s*2/5,(int)y+s/2,(int)x+s/4,(int)y+s*2/3,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s/2,(int)y+s*2/3,(int)x+s*3/4,(int)y+s*2/3,t,color,alpha);
        break;
    case 3: /* search */
        zr_dot(fb,stride,width,height,format,cx-s/10,cy-s/10,s/5,color,(U8)(alpha/5U));
        zr_dot(fb,stride,width,height,format,cx-s/10,cy-s/10,s/7,0x0b1727U,220U);
        zr_line(fb,stride,width,height,format,cx+s/20,cy+s/20,cx+s/4,cy+s/4,t,color,alpha);
        break;
    case 4: /* network/wifi */
        zr_dot(fb,stride,width,height,format,cx,cy+s/5,t+1,color,alpha);
        zr_line(fb,stride,width,height,format,cx-s/5,cy,cx,cy-s/8,t,color,(U8)(alpha*3U/4U));
        zr_line(fb,stride,width,height,format,cx,cy-s/8,cx+s/5,cy,t,color,(U8)(alpha*3U/4U));
        zr_line(fb,stride,width,height,format,cx-s/3,cy-s/6,cx,cy-s/3,t,color,(U8)(alpha/2U));
        zr_line(fb,stride,width,height,format,cx,cy-s/3,cx+s/3,cy-s/6,t,color,(U8)(alpha/2U));
        break;
    case 5: /* volume */
        zr_line(fb,stride,width,height,format,(int)x+s/4,cy,(int)x+s*2/5,cy,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s*2/5,cy,cx,cy-s/6,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx,cy-s/6,cx,cy+s/6,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx,cy+s/6,(int)x+s*2/5,cy,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx+s/12,cy-s/7,cx+s/6,cy,t,color,(U8)(alpha*3U/4U));
        zr_line(fb,stride,width,height,format,cx+s/6,cy,cx+s/12,cy+s/7,t,color,(U8)(alpha*3U/4U));
        break;
    case 6: /* battery */
        zr_round(fb,stride,width,height,format,x+s/5,y+s/3,x+s*4/5,y+s*2/3,color,(U8)(alpha/5U),s/12);
        zr_round(fb,stride,width,height,format,x+s/4,y+s*2/5,x+s*7/10,y+s*3/5,color,alpha,s/16);
        zr_round(fb,stride,width,height,format,x+s*4/5,y+s*5/12,x+s*17/20,y+s*7/12,color,alpha,s/24);
        break;
    case 7: /* grid/apps */
        for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx)
            zr_round(fb,stride,width,height,format,x+s/4+(U32)xx*s/4,y+s/4+(U32)yy*s/4,
                     x+s/4+(U32)xx*s/4+s/7,y+s/4+(U32)yy*s/4+s/7,color,alpha,s/18);
        break;
    case 8: /* image/photos */
        zr_round(fb,stride,width,height,format,x+s/5,y+s/5,x+s*4/5,y+s*4/5,color,(U8)(alpha/6U),s/10);
        zr_dot(fb,stride,width,height,format,(int)x+s*2/3,(int)y+s/3,s/14,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s/4,(int)y+s*2/3,(int)x+s*2/5,(int)y+s/2,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s*2/5,(int)y+s/2,(int)x+s*3/4,(int)y+s*3/4,t,color,alpha);
        break;
    case 9: /* document */
        zr_round(fb,stride,width,height,format,x+s/4,y+s/6,x+s*3/4,y+s*5/6,color,(U8)(alpha/5U),s/12);
        zr_line(fb,stride,width,height,format,(int)x+s/3,(int)y+s*2/5,(int)x+s*2/3,(int)y+s*2/5,t,color,alpha);
        zr_line(fb,stride,width,height,format,(int)x+s/3,(int)y+s/2,(int)x+s*2/3,(int)y+s/2,t,color,(U8)(alpha*3U/4U));
        zr_line(fb,stride,width,height,format,(int)x+s/3,(int)y+s*3/5,(int)x+s*3/5,(int)y+s*3/5,t,color,(U8)(alpha/2U));
        break;
    case 10: /* home */
        zr_line(fb,stride,width,height,format,cx-s/3,cy,cx,cy-s/3,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx,cy-s/3,cx+s/3,cy,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx-s/4,cy-s/12,cx-s/4,cy+s/3,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx+s/4,cy-s/12,cx+s/4,cy+s/3,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx-s/4,cy+s/3,cx+s/4,cy+s/3,t,color,alpha);
        break;
    case 11: /* display */
        zr_round(fb,stride,width,height,format,x+s/6,y+s/5,x+s*5/6,y+s*2/3,color,(U8)(alpha/5U),s/12);
        zr_line(fb,stride,width,height,format,cx,cy+s/6,cx,cy+s/3,t,color,alpha);
        zr_line(fb,stride,width,height,format,cx-s/6,cy+s/3,cx+s/6,cy+s/3,t,color,alpha);
        break;
    default:
        zr_dot(fb,stride,width,height,format,cx,cy,s/5,color,alpha);
        break;
    }
}
