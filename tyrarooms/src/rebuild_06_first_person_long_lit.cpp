#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <math.h>

struct Vec3 { float x,y,z; };

static GSGLOBAL *gs=0;
static unsigned char padBuffer[256] __attribute__((aligned(64)));
static float px=0.0f, pz=0.5f, yaw=0.0f, pitch=0.0f;
static float walkTime=0.0f, moveAmount=0.0f;
static const float PI=3.1415926535f;

static u64 rgba(int r,int g,int b) { return GS_SETREG_RGBAQ(r,g,b,0x80,0); }

static bool project(const Vec3 &p,float &sx,float &sy,int &sz) {
    const float camY=1.62f + ((moveAmount>0.03f)?fabsf(sinf(walkTime*10.0f))*0.035f:0.0f);
    float dx=p.x-px, dz=p.z-pz;
    float side=dx*cosf(yaw)-dz*sinf(yaw);
    float depth=dx*sinf(yaw)+dz*cosf(yaw);
    if(depth<0.12f) return false;
    float vertical=(p.y-camY) - depth*tanf(pitch);
    float focal=(float)gs->Width*0.86f;
    sx=(float)gs->Width*0.5f + side*focal/depth;
    sy=(float)gs->Height*0.51f - vertical*focal/depth;
    sz=(int)(depth*1500.0f);
    if(sz<1) sz=1;
    return true;
}

static void quad3(const Vec3 &a,const Vec3 &b,const Vec3 &c,const Vec3 &d,u64 color) {
    float x1,y1,x2,y2,x3,y3,x4,y4; int z1,z2,z3,z4;
    if(!project(a,x1,y1,z1)||!project(b,x2,y2,z2)||
       !project(c,x3,y3,z3)||!project(d,x4,y4,z4)) return;
    gsKit_prim_quad_3d(gs,x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4,color);
}

static void quad2(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,u64 color) {
    gsKit_prim_quad_3d(gs,x1,y1,8,x2,y2,8,x3,y3,8,x4,y4,8,color);
}

static void drawRoom() {
    const float L=-4.0f,R=4.0f,H=3.35f;
    const float start=pz+0.18f, end=82.0f;
    const u64 wall1=rgba(193,171,91), wall2=rgba(178,157,80), wall3=rgba(205,184,105);
    const u64 floor1=rgba(104,91,60), floor2=rgba(91,80,53);
    const u64 ceil1=rgba(165,161,143), ceil2=rgba(151,148,132);
    const u64 trim=rgba(116,99,55), dark=rgba(49,43,31);
    const u64 lamp=rgba(255,247,203), glow=rgba(220,207,150);
    
    /* Far wall closes a long, continuous corridor. */
    quad3(Vec3{L,0,end},Vec3{R,0,end},Vec3{R,H,end},Vec3{L,H,end},wall2);

    /* Ceiling is one continuous plane with repeating structural seams. */
    for(int i=0;i<40;i++) {
        float z0=start+(end-start)*(float)i/40.0f;
        float z1=start+(end-start)*(float)(i+1)/40.0f;
        u64 c=(i&1)?ceil1:ceil2;
        quad3(Vec3{L,H,z0},Vec3{R,H,z0},Vec3{R,H,z1},Vec3{L,H,z1},c);
        /* Ceiling fluorescent fixtures: bright rectangular light panels. */
        if((i%2)==0) {
            float mid=(z0+z1)*0.5f;
            quad3(Vec3{-0.72f,H-0.025f,mid-0.52f},Vec3{0.72f,H-0.025f,mid-0.52f},
                  Vec3{0.72f,H-0.025f,mid+0.52f},Vec3{-0.72f,H-0.025f,mid+0.52f},lamp);
            quad3(Vec3{-0.83f,H-0.045f,mid-0.58f},Vec3{0.83f,H-0.045f,mid-0.58f},
                  Vec3{0.83f,H-0.045f,mid+0.58f},Vec3{-0.83f,H-0.045f,mid+0.58f},glow);
        }
    }

    /* Side walls are divided into tall, solid panels; no open gaps. */
    for(int i=0;i<40;i++) {
        float z0=start+(end-start)*(float)i/40.0f;
        float z1=start+(end-start)*(float)(i+1)/40.0f;
        u64 c=(i%3==0)?wall3:((i&1)?wall1:wall2);
        quad3(Vec3{L,0,z0},Vec3{L,0,z1},Vec3{L,H,z1},Vec3{L,H,z0},c);
        quad3(Vec3{R,0,z1},Vec3{R,0,z0},Vec3{R,H,z0},Vec3{R,H,z1},(i&1)?wall2:wall3);
        /* Dark vertical seams and continuous lower/upper trim. */
        quad3(Vec3{L+0.015f,0.0f,z0},Vec3{L+0.015f,0.0f,z0+0.025f},Vec3{L+0.015f,H,z0+0.025f},Vec3{L+0.015f,H,z0},trim);
        quad3(Vec3{R-0.015f,0.0f,z0+0.025f},Vec3{R-0.015f,0.0f,z0},Vec3{R-0.015f,H,z0},Vec3{R-0.015f,H,z0+0.025f},trim);
    }
    quad3(Vec3{L,0.10f,start},Vec3{L,0.10f,end},Vec3{L,0.24f,end},Vec3{L,0.24f,start},trim);
    quad3(Vec3{R,0.10f,end},Vec3{R,0.10f,start},Vec3{R,0.24f,start},Vec3{R,0.24f,end},trim);
    quad3(Vec3{L,3.08f,start},Vec3{L,3.08f,end},Vec3{L,3.20f,end},Vec3{L,3.20f,start},trim);
    quad3(Vec3{R,3.08f,end},Vec3{R,3.08f,start},Vec3{R,3.20f,start},Vec3{R,3.20f,end},trim);

    /* Floor tiles and warm light pools underneath the fluorescent panels. */
    for(int iz=0;iz<80;iz++) {
        float z0=start+(end-start)*(float)iz/80.0f;
        float z1=start+(end-start)*(float)(iz+1)/80.0f;
        for(int ix=0;ix<8;ix++) {
            float x0=L+(R-L)*(float)ix/8.0f;
            float x1=L+(R-L)*(float)(ix+1)/8.0f;
            u64 c=((ix+iz)&1)?floor1:floor2;
            quad3(Vec3{x0,0.012f,z1},Vec3{x1,0.012f,z1},Vec3{x1,0.012f,z0},Vec3{x0,0.012f,z0},c);
        }
        if((iz%4)==1) {
            float mid=(z0+z1)*0.5f;
            quad3(Vec3{-1.25f,0.018f,mid-0.30f},Vec3{1.25f,0.018f,mid-0.30f},
                  Vec3{1.25f,0.018f,mid+0.30f},Vec3{-1.25f,0.018f,mid+0.30f},rgba(126,111,69));
        }
    }
    /* A few wall-mounted fluorescent boxes add depth cues and ambience. */
    for(int i=2;i<38;i+=5) {
        float z=start+(end-start)*(float)i/40.0f;
        quad3(Vec3{L+0.025f,1.72f,z},Vec3{L+0.025f,2.25f,z},Vec3{L+0.025f,2.25f,z+0.12f},Vec3{L+0.025f,1.72f,z+0.12f},lamp);
        quad3(Vec3{R-0.025f,2.25f,z+0.12f},Vec3{R-0.025f,1.72f,z+0.12f},Vec3{R-0.025f,1.72f,z},Vec3{R-0.025f,2.25f,z},lamp);
    }
    /* Back wall seam and dark skirting give the corridor a clear endpoint. */
    quad3(Vec3{-0.06f,0.0f,end-0.02f},Vec3{0.06f,0.0f,end-0.02f},Vec3{0.06f,H,end-0.02f},Vec3{-0.06f,H,end-0.02f},dark);
}

static void drawFirstPersonArms() {
    float bob=(moveAmount>0.03f)?sinf(walkTime*10.0f):sinf(walkTime*2.0f)*0.12f;
    float swing=(moveAmount>0.03f)?sinf(walkTime*10.0f)*12.0f:0.0f;
    float baseY=(float)gs->Height*0.83f + bob*7.0f;
    float leftX=(float)gs->Width*0.29f + swing;
    float rightX=(float)gs->Width*0.71f - swing;
    /* Sleeves / forearms are layered trapezoids, anchored to the screen bottom. */
    quad2(leftX-44,baseY+80,leftX+10,baseY+80,leftX+27,baseY-26,leftX-10,baseY-42,rgba(55,66,79));
    quad2(rightX-10,baseY-42,rightX+27,baseY-26,rightX+10,baseY+80,rightX-44,baseY+80,rgba(55,66,79));
    quad2(leftX-10,baseY-42,leftX+27,baseY-26,leftX+20,baseY+20,leftX-20,baseY+30,rgba(188,145,105));
    quad2(rightX-20,baseY+30,rightX+20,baseY+20,rightX+27,baseY-26,rightX-10,baseY-42,rgba(188,145,105));
    /* Simple clenched hands, kept deliberately low so the corridor stays visible. */
    quad2(leftX-22,baseY-25,leftX+18,baseY-25,leftX+22,baseY+3,leftX-18,baseY+7,rgba(207,165,124));
    quad2(rightX-18,baseY+7,rightX+22,baseY+3,rightX+18,baseY-25,rightX-22,baseY-25,rgba(207,165,124));
}

static float axis(unsigned char raw,bool invert) {
    float v=((float)raw-128.0f)/127.0f;
    if(invert) v=-v;
    if(v>-0.18f && v<0.18f) return 0.0f;
    return v>0.0f?(v-0.18f)/0.82f:(v+0.18f)/0.82f;
}

static void updatePad() {
    if(padGetState(0,0)!=PAD_STATE_STABLE) { moveAmount=0; return; }
    padButtonStatus p;
    if(padRead(0,0,&p)==0) { moveAmount=0; return; }
    float forward=axis(p.ljoy_v,true), strafe=axis(p.ljoy_h,false);
    if(forward==0) { if(!(p.btns&PAD_UP)) forward=1; else if(!(p.btns&PAD_DOWN)) forward=-1; }
    if(strafe==0) { if(!(p.btns&PAD_RIGHT)) strafe=1; else if(!(p.btns&PAD_LEFT)) strafe=-1; }
    float mag=sqrtf(forward*forward+strafe*strafe);
    moveAmount=mag;
    if(mag>1) { forward/=mag; strafe/=mag; }
    yaw += axis(p.rjoy_h,false)*0.035f;
    pitch += axis(p.rjoy_v,true)*0.018f;
    if(pitch>0.35f) pitch=0.35f;
    if(pitch< -0.35f) pitch=-0.35f;
    float speed=0.075f;
    px += (sinf(yaw)*forward+cosf(yaw)*strafe)*speed;
    pz += (cosf(yaw)*forward-sinf(yaw)*strafe)*speed;
    if(px< -2.8f) px=-2.8f; if(px>2.8f) px=2.8f;
    if(pz<0.0f) pz=0.0f; if(pz>76.0f) pz=76.0f;
}

int main(int,char**) {
    gs=gsKit_init_global();
    if(!gs) for(;;) {}
    gs->Mode=GS_MODE_NTSC; gs->Width=640; gs->Height=448;
    gs->PSM=GS_PSM_CT32; gs->PSMZ=GS_PSMZ_16;
    gs->ZBuffering=GS_SETTING_OFF; gs->PrimAlphaEnable=GS_SETTING_OFF;
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_init_screen(gs); gsKit_vram_clear(gs); gsKit_mode_switch(gs,GS_ONESHOT);
    padInit(0);
    if(padPortOpen(0,0,padBuffer)) {
        for(int i=0;i<120 && padGetState(0,0)!=PAD_STATE_STABLE;++i) {}
        if(padGetState(0,0)==PAD_STATE_STABLE) padSetMainMode(0,0,PAD_MMODE_DUALSHOCK,PAD_MMODE_LOCK);
    }
    for(;;) {
        updatePad();
        gsKit_clear(gs,rgba(30,28,22));
        drawRoom();
        drawFirstPersonArms();
        gsKit_queue_exec(gs); gsKit_sync_flip(gs);
        walkTime+=1.0f/60.0f;
    }
    return 0;
}
