#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <math.h>

/*
 * BACKROOMS REBUILD 06 - FIRST PERSON / LONG LIT CORRIDOR
 * World geometry is built from closed planes and ordered opaque surfaces.
 * No texture dependency and no Z-buffer dependency.
 */

struct Vec3 { float x,y,z; };

static GSGLOBAL *gs=0;
static unsigned char padBuffer[256] __attribute__((aligned(64)));

static float playerX=0.0f;
static float playerZ=1.0f;
static float yaw=0.0f;
static float pitch=0.0f;
static float animTime=0.0f;
static float walkPhase=0.0f;
static float moveAmount=0.0f;

static const float ROOM_LEFT=-4.0f;
static const float ROOM_RIGHT=4.0f;
static const float ROOM_HEIGHT=3.35f;
static const float ROOM_END=82.0f;
static const float FOCAL=430.0f;

static u64 color(int r,int g,int b) {
    return GS_SETREG_RGBAQ(r,g,b,0x80,0);
}

static bool projectWorld(const Vec3 &p,float &sx,float &sy,int &sz) {
    const float cameraBob=(moveAmount>0.03f)?fabsf(sinf(walkPhase*2.0f))*0.035f:0.0f;
    const float cameraY=1.62f+cameraBob;
    const float dx=p.x-playerX;
    const float dz=p.z-playerZ;
    const float side=dx*cosf(yaw)-dz*sinf(yaw);
    const float depth=dx*sinf(yaw)+dz*cosf(yaw);
    const float vertical=(p.y-cameraY)-depth*tanf(pitch);
    if(depth<0.16f) return false;
    sx=320.0f+side*FOCAL/depth;
    sy=224.0f-vertical*FOCAL/depth;
    sz=(int)(depth*1400.0f);
    if(sz<1) sz=1;
    if(sz>65535) sz=65535;
    return true;
}

static bool projectView(float x,float y,float depth,
                        float &sx,float &sy,int &sz) {
    if(depth<0.16f) return false;
    sx=320.0f+x*FOCAL/depth;
    sy=224.0f-y*FOCAL/depth;
    sz=(int)(depth*1400.0f);
    if(sz<1) sz=1;
    if(sz>65535) sz=65535;
    return true;
}

static void quadWorld(const Vec3 &a,const Vec3 &b,
                      const Vec3 &c,const Vec3 &d,u64 col) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    if(!projectWorld(a,x1,y1,z1)||!projectWorld(b,x2,y2,z2)||
       !projectWorld(c,x3,y3,z3)||!projectWorld(d,x4,y4,z4)) return;
    gsKit_prim_quad_3d(gs,x1,y1,z1,x2,y2,z2,
                       x3,y3,z3,x4,y4,z4,col);
}

static void quadView(float ax,float ay,float az,
                     float bx,float by,float bz,
                     float cx,float cy,float cz,
                     float dx,float dy,float dz,u64 col) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    if(!projectView(ax,ay,az,x1,y1,z1)||
       !projectView(bx,by,bz,x2,y2,z2)||
       !projectView(cx,cy,cz,x3,y3,z3)||
       !projectView(dx,dy,dz,x4,y4,z4)) return;
    gsKit_prim_quad_3d(gs,x1,y1,z1,x2,y2,z2,
                       x3,y3,z3,x4,y4,z4,col);
}

/* Draw only the camera-visible faces of a small first-person hand block. */
static void viewBox(float x,float y,float depth,
                    float w,float h,float d,
                    u64 front,u64 outer,u64 upper,int sideSign) {
    const float x0=x-w*0.5f,x1=x+w*0.5f;
    const float y0=y-h*0.5f,y1=y+h*0.5f;
    const float z0=depth-d*0.5f,z1=depth+d*0.5f;

    quadView(x0,y1,z0,x1,y1,z0,x1,y1,z1,x0,y1,z1,upper);
    if(sideSign<0)
        quadView(x0,y0,z0,x0,y0,z1,x0,y1,z1,x0,y1,z0,outer);
    else
        quadView(x1,y0,z1,x1,y0,z0,x1,y1,z0,x1,y1,z1,outer);
    quadView(x0,y0,z0,x1,y0,z0,x1,y1,z0,x0,y1,z0,front);
}

static int lampBoost(float z) {
    int nearest=(int)((z-6.0f)/12.0f+0.5f);
    const float lampZ=6.0f+(float)nearest*12.0f;
    const float distance=fabsf(z-lampZ);
    if(distance>=6.0f) return 0;
    return (int)((6.0f-distance)*1.7f);
}

static void drawEndWall() {
    quadWorld(Vec3{ROOM_LEFT,0.0f,ROOM_END},
              Vec3{ROOM_RIGHT,0.0f,ROOM_END},
              Vec3{ROOM_RIGHT,ROOM_HEIGHT,ROOM_END},
              Vec3{ROOM_LEFT,ROOM_HEIGHT,ROOM_END},color(153,132,76));

    /* A dark doorway at the far end gives the corridor a destination. */
    quadWorld(Vec3{-0.95f,0.12f,ROOM_END-0.015f},
              Vec3{ 0.95f,0.12f,ROOM_END-0.015f},
              Vec3{ 0.95f,2.85f,ROOM_END-0.015f},
              Vec3{-0.95f,2.85f,ROOM_END-0.015f},color(47,40,29));
    quadWorld(Vec3{-1.08f,2.82f,ROOM_END-0.025f},
              Vec3{ 1.08f,2.82f,ROOM_END-0.025f},
              Vec3{ 1.08f,2.96f,ROOM_END-0.025f},
              Vec3{-1.08f,2.96f,ROOM_END-0.025f},color(119,101,59));
}

static void drawSideWalls(float start) {
    for(float z0=start;z0<ROOM_END;z0+=8.0f) {
        float z1=z0+8.0f;
        if(z1>ROOM_END) z1=ROOM_END;
        const float mid=(z0+z1)*0.5f;
        const int boost=lampBoost(mid);

        quadWorld(Vec3{ROOM_LEFT,0.0f,z0},
                  Vec3{ROOM_LEFT,0.0f,z1},
                  Vec3{ROOM_LEFT,ROOM_HEIGHT,z1},
                  Vec3{ROOM_LEFT,ROOM_HEIGHT,z0},
                  color(184+boost,163+boost,97+boost/2));
        quadWorld(Vec3{ROOM_RIGHT,0.0f,z1},
                  Vec3{ROOM_RIGHT,0.0f,z0},
                  Vec3{ROOM_RIGHT,ROOM_HEIGHT,z0},
                  Vec3{ROOM_RIGHT,ROOM_HEIGHT,z1},
                  color(174+boost,153+boost,90+boost/2));
    }

    /* Narrow seams at real wall joints, not broad decorative stripes. */
    for(int i=1;i<11;i++) {
        const float z=(float)i*8.0f;
        if(z<start || z>ROOM_END) continue;
        quadWorld(Vec3{ROOM_LEFT+0.012f,0.0f,z-0.018f},
                  Vec3{ROOM_LEFT+0.012f,ROOM_HEIGHT,z-0.018f},
                  Vec3{ROOM_LEFT+0.012f,ROOM_HEIGHT,z+0.018f},
                  Vec3{ROOM_LEFT+0.012f,0.0f,z+0.018f},color(159,139,82));
        quadWorld(Vec3{ROOM_RIGHT-0.012f,0.0f,z+0.018f},
                  Vec3{ROOM_RIGHT-0.012f,ROOM_HEIGHT,z+0.018f},
                  Vec3{ROOM_RIGHT-0.012f,ROOM_HEIGHT,z-0.018f},
                  Vec3{ROOM_RIGHT-0.012f,0.0f,z-0.018f},color(151,131,76));
    }

    /* Continuous baseboards and upper wall trim. */
    quadWorld(Vec3{ROOM_LEFT+0.01f,0.12f,start},
              Vec3{ROOM_LEFT+0.01f,0.12f,ROOM_END},
              Vec3{ROOM_LEFT+0.01f,0.25f,ROOM_END},
              Vec3{ROOM_LEFT+0.01f,0.25f,start},color(112,94,54));
    quadWorld(Vec3{ROOM_RIGHT-0.01f,0.12f,ROOM_END},
              Vec3{ROOM_RIGHT-0.01f,0.12f,start},
              Vec3{ROOM_RIGHT-0.01f,0.25f,start},
              Vec3{ROOM_RIGHT-0.01f,0.25f,ROOM_END},color(104,87,51));
    quadWorld(Vec3{ROOM_LEFT+0.01f,3.10f,start},
              Vec3{ROOM_LEFT+0.01f,3.10f,ROOM_END},
              Vec3{ROOM_LEFT+0.01f,3.22f,ROOM_END},
              Vec3{ROOM_LEFT+0.01f,3.22f,start},color(142,128,83));
    quadWorld(Vec3{ROOM_RIGHT-0.01f,3.10f,ROOM_END},
              Vec3{ROOM_RIGHT-0.01f,3.10f,start},
              Vec3{ROOM_RIGHT-0.01f,3.22f,start},
              Vec3{ROOM_RIGHT-0.01f,3.22f,ROOM_END},color(134,120,77));
}

static void drawFloor(float start) {
    for(float z0=start;z0<ROOM_END;z0+=6.0f) {
        float z1=z0+6.0f;
        if(z1>ROOM_END) z1=ROOM_END;
        const int boost=lampBoost((z0+z1)*0.5f)/2;
        quadWorld(Vec3{ROOM_LEFT,0.0f,z1},
                  Vec3{ROOM_RIGHT,0.0f,z1},
                  Vec3{ROOM_RIGHT,0.0f,z0},
                  Vec3{ROOM_LEFT,0.0f,z0},
                  color(83+boost,71+boost,50+boost/2));
    }

    /* Very subtle carpet joins every few metres. */
    for(float z=start+3.0f;z<ROOM_END;z+=3.0f) {
        quadWorld(Vec3{ROOM_LEFT,0.012f,z-0.012f},
                  Vec3{ROOM_RIGHT,0.012f,z-0.012f},
                  Vec3{ROOM_RIGHT,0.012f,z+0.012f},
                  Vec3{ROOM_LEFT,0.012f,z+0.012f},color(78,67,48));
    }
}

static void drawCeiling(float start) {
    for(float z0=start;z0<ROOM_END;z0+=8.0f) {
        float z1=z0+8.0f;
        if(z1>ROOM_END) z1=ROOM_END;
        quadWorld(Vec3{ROOM_LEFT,ROOM_HEIGHT,z0},
                  Vec3{ROOM_LEFT,ROOM_HEIGHT,z1},
                  Vec3{ROOM_RIGHT,ROOM_HEIGHT,z1},
                  Vec3{ROOM_RIGHT,ROOM_HEIGHT,z0},color(166,162,145));
    }

    for(int i=1;i<11;i++) {
        const float z=(float)i*8.0f;
        if(z<start || z>ROOM_END) continue;
        quadWorld(Vec3{ROOM_LEFT,3.29f,z-0.045f},
                  Vec3{ROOM_RIGHT,3.29f,z-0.045f},
                  Vec3{ROOM_RIGHT,3.35f,z+0.045f},
                  Vec3{ROOM_LEFT,3.35f,z+0.045f},color(143,140,125));
    }
}

static void drawDoor(int side,float z) {
    const float x=(side<0)?ROOM_LEFT+0.025f:ROOM_RIGHT-0.025f;
    const float w=1.45f;
    /* Dark recessed door panel. */
    quadWorld(Vec3{x,0.20f,z},Vec3{x,0.20f,z+w},
              Vec3{x,2.78f,z+w},Vec3{x,2.78f,z},color(55,47,34));
    /* Three raised frame pieces and a small handle. */
    quadWorld(Vec3{x,0.16f,z-0.09f},Vec3{x,0.16f,z+0.01f},
              Vec3{x,2.88f,z+0.01f},Vec3{x,2.88f,z-0.09f},color(132,112,65));
    quadWorld(Vec3{x,0.16f,z+w-0.01f},Vec3{x,0.16f,z+w+0.09f},
              Vec3{x,2.88f,z+w+0.09f},Vec3{x,2.88f,z+w-0.01f},color(132,112,65));
    quadWorld(Vec3{x,2.78f,z-0.09f},Vec3{x,2.78f,z+w+0.09f},
              Vec3{x,2.90f,z+w+0.09f},Vec3{x,2.90f,z-0.09f},color(132,112,65));
    quadWorld(Vec3{x-0.008f,1.12f,z+w-0.20f},Vec3{x-0.008f,1.12f,z+w-0.12f},
              Vec3{x-0.008f,1.28f,z+w-0.12f},Vec3{x-0.008f,1.28f,z+w-0.20f},color(205,178,104));
}

static void drawLamp(float z) {
    /* Grey housing and a bright inset fluorescent panel. */
    quadWorld(Vec3{-0.98f,3.285f,z-0.88f},
              Vec3{ 0.98f,3.285f,z-0.88f},
              Vec3{ 0.98f,3.285f,z+0.88f},
              Vec3{-0.98f,3.285f,z+0.88f},color(111,109,99));
    quadWorld(Vec3{-0.79f,3.265f,z-0.70f},
              Vec3{ 0.79f,3.265f,z-0.70f},
              Vec3{ 0.79f,3.265f,z+0.70f},
              Vec3{-0.79f,3.265f,z+0.70f},color(250,244,214));

    /* A soft-looking warm patch painted into the carpet below the lamp. */
    quadWorld(Vec3{-3.0f,0.018f,z-3.2f},
              Vec3{ 3.0f,0.018f,z-3.2f},
              Vec3{ 2.35f,0.018f,z+3.2f},
              Vec3{-2.35f,0.018f,z+3.2f},color(100,88,61));
}

static void drawRoom() {
    const float start=playerZ+0.20f;
    drawEndWall();
    drawSideWalls(start);
    drawFloor(start);
    drawCeiling(start);

    /* Doors are sparse landmarks so the hall reads as one large space. */
    drawDoor(-1,18.0f);
    drawDoor( 1,43.0f);
    drawDoor(-1,62.0f);

    for(int i=0;i<7;i++) {
        const float z=6.0f+(float)i*12.0f;
        if(z>start-1.0f) drawLamp(z);
    }
}

/* Tapered, closed-looking sleeves are drawn in camera space.
 * The near face and outer side meet at shared vertices: no detached quads. */
static void drawArm(int side,float swing,float bob) {
    const float shoulderX=(float)side*0.84f+(float)side*swing*0.18f;
    const float shoulderY=-0.91f+bob;
    const float shoulderZ=1.98f;
    const float wristX=(float)side*0.48f+(float)side*swing*0.42f;
    const float wristY=-0.34f+bob+swing*0.08f;
    const float wristZ=0.88f;

    float dx=wristX-shoulderX,dy=wristY-shoulderY;
    float len=sqrtf(dx*dx+dy*dy);
    if(len<0.01f) len=0.01f;
    const float nx=-dy/len,ny=dx/len;
    const float outer=(side<0)?1.0f:-1.0f;
    const float sh=0.205f,wh=0.145f,thick=0.19f;

    const float smx=shoulderX-nx*sh,smy=shoulderY-ny*sh;
    const float spx=shoulderX+nx*sh,spy=shoulderY+ny*sh;
    const float wmx=wristX-nx*wh,wmy=wristY-ny*wh;
    const float wpx=wristX+nx*wh,wpy=wristY+ny*wh;

    /* Front face. */
    quadView(smx,smy,shoulderZ-thick*0.5f,
             spx,spy,shoulderZ-thick*0.5f,
             wpx,wpy,wristZ-thick*0.5f,
             wmx,wmy,wristZ-thick*0.5f,color(57,70,88));

    /* Outer thickness face. */
    if(side<0)
        quadView(spx,spy,shoulderZ-thick*0.5f,
                 spx,spy,shoulderZ+thick*0.5f,
                 wpx,wpy,wristZ+thick*0.5f,
                 wpx,wpy,wristZ-thick*0.5f,color(36,47,63));
    else
        quadView(smx,smy,shoulderZ-thick*0.5f,
                 smx,smy,shoulderZ+thick*0.5f,
                 wmx,wmy,wristZ+thick*0.5f,
                 wmx,wmy,wristZ-thick*0.5f,color(36,47,63));

    /* Wrist cuff. */
    quadView(wmx,wmy,wristZ-thick*0.5f,
             wpx,wpy,wristZ-thick*0.5f,
             wpx,wpy,wristZ+thick*0.5f,
             wmx,wmy,wristZ+thick*0.5f,color(91,103,119));
}

static void drawHand(int side,float sway,float bob) {
    const float hx=(float)side*0.46f+(float)side*sway*0.35f;
    const float hy=-0.34f+bob;
    const float hd=0.73f;

    viewBox(hx,hy,hd,0.235f,0.18f,0.23f,
            color(207,165,124),color(164,124,91),color(225,185,145),side);

    /* Four short fingers and a thumb, all separate solid 3D pieces. */
    for(int i=0;i<4;i++) {
        const float fx=hx-0.075f+(float)i*0.050f;
        viewBox(fx,hy-0.115f,0.67f,0.038f,0.085f,0.095f,
                color(207,165,124),color(164,124,91),color(225,185,145),side);
    }
    viewBox(hx-(float)side*0.145f,hy-0.035f,0.72f,
            0.075f,0.13f,0.12f,
            color(207,165,124),color(164,124,91),color(225,185,145),side);
}

static void drawFirstPersonArms() {
    const float moving=(moveAmount>0.035f)?1.0f:0.0f;
    const float wave=sinf(walkPhase);
    const float sway=wave*0.075f*moving;
    const float bob=(moving>0.0f)?fabsf(sinf(walkPhase*2.0f))*0.035f:
                    sinf(animTime*1.6f)*0.012f;

    /* Farther sleeves first; hands and fingers are layered in front. */
    drawArm(-1,sway,bob);
    drawArm( 1,-sway,bob);
    drawHand(-1,sway,bob);
    drawHand( 1,-sway,bob);
}

static float axisValue(unsigned char raw,bool invert) {
    float v=((float)raw-128.0f)/127.0f;
    if(invert) v=-v;
    if(v>-0.18f && v<0.18f) return 0.0f;
    return v>0.0f?(v-0.18f)/0.82f:(v+0.18f)/0.82f;
}

static void updatePad() {
    if(padGetState(0,0)!=PAD_STATE_STABLE) { moveAmount=0.0f; return; }
    padButtonStatus p;
    if(padRead(0,0,&p)==0) { moveAmount=0.0f; return; }

    float forward=axisValue(p.ljoy_v,true);
    float strafe=axisValue(p.ljoy_h,false);
    if(forward==0.0f) {
        if(!(p.btns&PAD_UP)) forward=1.0f;
        else if(!(p.btns&PAD_DOWN)) forward=-1.0f;
    }
    if(strafe==0.0f) {
        if(!(p.btns&PAD_RIGHT)) strafe=1.0f;
        else if(!(p.btns&PAD_LEFT)) strafe=-1.0f;
    }

    float mag=sqrtf(forward*forward+strafe*strafe);
    moveAmount=mag;
    if(mag>1.0f) { forward/=mag; strafe/=mag; mag=1.0f; }

    yaw+=axisValue(p.rjoy_h,false)*0.035f;
    pitch+=axisValue(p.rjoy_v,true)*0.018f;
    if(pitch>0.32f) pitch=0.32f;
    if(pitch< -0.32f) pitch=-0.32f;

    const float speed=0.072f;
    playerX+=(sinf(yaw)*forward+cosf(yaw)*strafe)*speed;
    playerZ+=(cosf(yaw)*forward-sinf(yaw)*strafe)*speed;
    if(playerX< -2.8f) playerX=-2.8f;
    if(playerX>  2.8f) playerX= 2.8f;
    if(playerZ<0.0f) playerZ=0.0f;
    if(playerZ>ROOM_END-2.0f) playerZ=ROOM_END-2.0f;

    if(mag>0.035f) walkPhase+=0.19f+mag*0.13f;
}

int main(int,char**) {
    gs=gsKit_init_global();
    if(!gs) for(;;) {}
    gs->Mode=GS_MODE_NTSC;
    gs->Width=640;
    gs->Height=448;
    gs->PSM=GS_PSM_CT32;
    gs->PSMZ=GS_PSMZ_16;
    gs->ZBuffering=GS_SETTING_OFF;
    gs->PrimAlphaEnable=GS_SETTING_OFF;

    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_init_screen(gs);
    gsKit_vram_clear(gs);
    gsKit_mode_switch(gs,GS_ONESHOT);

    padInit(0);
    if(padPortOpen(0,0,padBuffer)) {
        for(int i=0;i<120 && padGetState(0,0)!=PAD_STATE_STABLE;++i) {}
        if(padGetState(0,0)==PAD_STATE_STABLE)
            padSetMainMode(0,0,PAD_MMODE_DUALSHOCK,PAD_MMODE_LOCK);
    }

    for(;;) {
        updatePad();
        gsKit_clear(gs,color(28,26,20));
        drawRoom();
        drawFirstPersonArms();
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
        animTime+=1.0f/60.0f;
    }
    return 0;
}
