#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <math.h>
#include <string.h>

/*
 * BACKROOMS REBUILD 04 - UNTEXTURED CONTROLLABLE CHARACTER
 * Flat-color room + simple third-person character.
 * No textures, menu, imported model, or gameplay assets.
 */

struct Vec3 { float x, y, z; };

static GSGLOBAL *g_gs = 0;
static unsigned char g_padBuffer[256] __attribute__((aligned(64)));
static float g_playerX = 0.0f;
static float g_playerZ = 3.0f;
static float g_animTime = 0.0f;
static float g_yaw = 0.0f;
static float g_move = 0.0f;

static const float PI = 3.1415926535f;

static bool project(const Vec3 &p, float &sx, float &sy, int &sz) {
    /* Third-person camera follows behind the player, looking along +Z. */
    const float camX = g_playerX - sinf(g_yaw) * 6.0f;
    const float camY = 2.8f;
    const float camZ = g_playerZ - cosf(g_yaw) * 6.0f;
    const float dx = p.x - camX;
    const float dz = p.z - camZ;
    const float side = dx * cosf(g_yaw) - dz * sinf(g_yaw);
    const float depth = dx * sinf(g_yaw) + dz * cosf(g_yaw);
    const float vertical = p.y - camY;
    if (depth < 0.2f) return false;
    const float focal = (float)g_gs->Width * 0.78f;
    sx = (float)g_gs->Width * 0.5f + side * focal / depth;
    sy = (float)g_gs->Height * 0.5f - vertical * focal / depth;
    sz = (int)(depth * 1800.0f);
    if (sz < 1) sz = 1;
    return true;
}

static void quad(const Vec3 &a, const Vec3 &b, const Vec3 &c,
                 const Vec3 &d, u64 color) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    if (!project(a,x1,y1,z1) || !project(b,x2,y2,z2) ||
        !project(c,x3,y3,z3) || !project(d,x4,y4,z4)) return;
    gsKit_prim_quad_3d(g_gs,x1,y1,z1,x2,y2,z2,
                       x3,y3,z3,x4,y4,z4,color);
}

static void box(float x, float y, float z, float w, float h, float d, u64 color) {
    const float x0=x-w*0.5f, x1=x+w*0.5f;
    const float y0=y-h*0.5f, y1=y+h*0.5f;
    const float z0=z-d*0.5f, z1=z+d*0.5f;
    quad(Vec3{x0,y0,z0},Vec3{x1,y0,z0},Vec3{x1,y1,z0},Vec3{x0,y1,z0},color);
    quad(Vec3{x1,y0,z1},Vec3{x0,y0,z1},Vec3{x0,y1,z1},Vec3{x1,y1,z1},color);
    quad(Vec3{x0,y0,z1},Vec3{x0,y0,z0},Vec3{x0,y1,z0},Vec3{x0,y1,z1},color);
    quad(Vec3{x1,y0,z0},Vec3{x1,y0,z1},Vec3{x1,y1,z1},Vec3{x1,y1,z0},color);
    quad(Vec3{x0,y1,z0},Vec3{x1,y1,z0},Vec3{x1,y1,z1},Vec3{x0,y1,z1},color);
    quad(Vec3{x0,y0,z1},Vec3{x1,y0,z1},Vec3{x1,y0,z0},Vec3{x0,y0,z0},color);
}

static void drawRoom() {
    const float L=-3.8f, R=3.8f, F=-1.0f, B=14.0f, H=3.2f;
    const u64 wallA=GS_SETREG_RGBAQ(198,177,91,0x80,0);
    const u64 wallB=GS_SETREG_RGBAQ(184,163,82,0x80,0);
    const u64 wallC=GS_SETREG_RGBAQ(205,185,103,0x80,0);
    const u64 trim=GS_SETREG_RGBAQ(127,111,61,0x80,0);
    const u64 floorA=GS_SETREG_RGBAQ(91,81,55,0x80,0);
    const u64 floorB=GS_SETREG_RGBAQ(76,68,47,0x80,0);
    const u64 ceilingA=GS_SETREG_RGBAQ(174,169,148,0x80,0);
    const u64 ceilingB=GS_SETREG_RGBAQ(157,153,136,0x80,0);
    const u64 dark=GS_SETREG_RGBAQ(43,39,29,0x80,0);

    /* Back wall first: segmented panels make its depth and scale readable. */
    for(int i=0;i<4;i++) {
        float x0=L+(R-L)*(float)i/4.0f;
        float x1=L+(R-L)*(float)(i+1)/4.0f;
        u64 c=(i&1)?wallA:wallC;
        quad(Vec3{x0,0,B},Vec3{x1,0,B},Vec3{x1,H,B},Vec3{x0,H,B},c);
    }

    /* Long side walls: separate vertical panels converge toward the far wall. */
    for(int i=0;i<6;i++) {
        float z0=F+(B-F)*(float)i/6.0f;
        float z1=F+(B-F)*(float)(i+1)/6.0f;
        u64 c=(i&1)?wallA:wallB;
        quad(Vec3{L,0,z0},Vec3{L,0,z1},Vec3{L,H,z1},Vec3{L,H,z0},c);
        quad(Vec3{R,0,z1},Vec3{R,0,z0},Vec3{R,H,z0},Vec3{R,H,z1},(i&1)?wallB:wallC);
    }

    /* Strong baseboards and top rails emphasize the room's 3D corners. */
    quad(Vec3{L,0.12f,F},Vec3{L,0.12f,B},Vec3{L,0.25f,B},Vec3{L,0.25f,F},trim);
    quad(Vec3{R,0.12f,B},Vec3{R,0.12f,F},Vec3{R,0.25f,F},Vec3{R,0.25f,B},trim);
    quad(Vec3{L,2.95f,F},Vec3{L,2.95f,B},Vec3{L,3.08f,B},Vec3{L,3.08f,F},trim);
    quad(Vec3{R,2.95f,B},Vec3{R,2.95f,F},Vec3{R,3.08f,F},Vec3{R,3.08f,B},trim);

    /* Ceiling panels and cross beams add visible perspective overhead. */
    for(int i=0;i<7;i++) {
        float z0=F+(B-F)*(float)i/7.0f;
        float z1=F+(B-F)*(float)(i+1)/7.0f;
        u64 c=(i&1)?ceilingA:ceilingB;
        quad(Vec3{L,H,z0},Vec3{R,H,z0},Vec3{R,H,z1},Vec3{L,H,z1},c);
    }
    for(int i=1;i<7;i++) {
        float z=F+(B-F)*(float)i/7.0f;
        quad(Vec3{L,3.08f,z-0.035f},Vec3{R,3.08f,z-0.035f},
             Vec3{R,3.16f,z+0.035f},Vec3{L,3.16f,z+0.035f},trim);
    }

    /* Floor is tiled in perspective instead of being one flat polygon. */
    for(int iz=0;iz<15;iz++) {
        float z0=F+(B-F)*(float)iz/15.0f;
        float z1=F+(B-F)*(float)(iz+1)/15.0f;
        for(int ix=0;ix<4;ix++) {
            float x0=L+(R-L)*(float)ix/4.0f;
            float x1=L+(R-L)*(float)(ix+1)/4.0f;
            u64 c=((ix+iz)&1)?floorA:floorB;
            quad(Vec3{x0,0.015f,z1},Vec3{x1,0.015f,z1},
                 Vec3{x1,0.015f,z0},Vec3{x0,0.015f,z0},c);
        }
    }

    /* Far-end dark seams give the eye a clear vanishing point. */
    quad(Vec3{-0.10f,0.0f,B-0.015f},Vec3{0.10f,0.0f,B-0.015f},
         Vec3{0.10f,H,B-0.015f},Vec3{-0.10f,H,B-0.015f},dark);
}

static void drawCharacter() {
    const float phase = g_animTime * (g_move > 0.05f ? 10.0f : 2.0f);
    const float swing = (g_move > 0.05f) ? sinf(phase)*0.22f : sinf(phase)*0.025f;
    const float bob = (g_move > 0.05f) ? fabsf(sinf(phase))*0.045f : 0.0f;
    const float x=g_playerX, z=g_playerZ, y=0.92f+bob;
    const u64 skin=GS_SETREG_RGBAQ(205,165,125,0x80,0);
    const u64 shirt=GS_SETREG_RGBAQ(65,95,145,0x80,0);
    const u64 pants=GS_SETREG_RGBAQ(45,48,58,0x80,0);
    const u64 shoes=GS_SETREG_RGBAQ(28,25,24,0x80,0);

    /* Head and torso */
    box(x,y+0.73f,z,0.38f,0.40f,0.34f,skin);
    box(x,y+0.27f,z,0.52f,0.62f,0.30f,shirt);

    /* Swinging arms and legs: intentionally simple block animation. */
    box(x-0.34f,y+0.28f+swing,z,0.17f,0.52f,0.18f,skin);
    box(x+0.34f,y+0.28f-swing,z,0.17f,0.52f,0.18f,skin);
    box(x-0.15f,y-0.20f-swing,z,0.20f,0.48f,0.22f,pants);
    box(x+0.15f,y-0.20f+swing,z,0.20f,0.48f,0.22f,pants);
    box(x-0.15f,y-0.46f-swing,z+0.035f,0.22f,0.12f,0.32f,shoes);
    box(x+0.15f,y-0.46f+swing,z+0.035f,0.22f,0.12f,0.32f,shoes);
}

static float axis(unsigned char raw, bool invert) {
    float v=((float)raw-128.0f)/127.0f;
    if(invert) v=-v;
    if(v>-0.20f && v<0.20f) return 0.0f;
    if(v>0.0f) return (v-0.20f)/0.80f;
    return (v+0.20f)/0.80f;
}

static void updateController() {
    if (padGetState(0,0)!=PAD_STATE_STABLE) { g_move=0.0f; return; }
    padButtonStatus p;
    if (padRead(0,0,&p)==0) { g_move=0.0f; return; }

    float forward=axis(p.ljoy_v,true);
    float strafe=axis(p.ljoy_h,false);
    if(forward==0.0f) {
        if(!(p.btns & PAD_UP)) forward=1.0f;
        else if(!(p.btns & PAD_DOWN)) forward=-1.0f;
    }
    if(strafe==0.0f) {
        if(!(p.btns & PAD_RIGHT)) strafe=1.0f;
        else if(!(p.btns & PAD_LEFT)) strafe=-1.0f;
    }
    const float mag=sqrtf(forward*forward+strafe*strafe);
    g_move=mag;
    if(mag>1.0f) { forward/=mag; strafe/=mag; }

    /* D-pad/left stick moves; right stick rotates the character. */
    const float turn=axis(p.rjoy_h,false);
    g_yaw += turn*0.045f;
    const float speed=0.055f;
    const float dx=sinf(g_yaw)*forward + cosf(g_yaw)*strafe;
    const float dz=cosf(g_yaw)*forward - sinf(g_yaw)*strafe;
    g_playerX += dx*speed;
    g_playerZ += dz*speed;

    if(g_playerX < -3.1f) g_playerX=-3.1f;
    if(g_playerX > 3.1f) g_playerX=3.1f;
    if(g_playerZ < 0.0f) g_playerZ=0.0f;
    if(g_playerZ > 12.0f) g_playerZ=12.0f;
}

int main(int, char **) {
    g_gs=gsKit_init_global();
    if(!g_gs) for(;;) {}
    g_gs->Mode=GS_MODE_NTSC;
    g_gs->Width=640;
    g_gs->Height=448;
    g_gs->PSM=GS_PSM_CT32;
    g_gs->PSMZ=GS_PSMZ_16;
    g_gs->ZBuffering=GS_SETTING_OFF;
    g_gs->PrimAlphaEnable=GS_SETTING_OFF;

    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_init_screen(g_gs);
    gsKit_vram_clear(g_gs);
    gsKit_mode_switch(g_gs,GS_ONESHOT);

    padInit(0);
    if(padPortOpen(0,0,g_padBuffer)) {
        for(int i=0;i<120 && padGetState(0,0)!=PAD_STATE_STABLE;++i) {}
        if(padGetState(0,0)==PAD_STATE_STABLE)
            padSetMainMode(0,0,PAD_MMODE_DUALSHOCK,PAD_MMODE_LOCK);
    }

    for(;;) {
        updateController();
        gsKit_clear(g_gs,GS_SETREG_RGBAQ(24,32,48,0x80,0));
        drawRoom();
        drawCharacter();
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
        g_animTime += 1.0f/60.0f;
    }
    return 0;
}
