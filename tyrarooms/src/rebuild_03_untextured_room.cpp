#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <math.h>

/*
 * BACKROOMS REBUILD 03 - UNTEXTURED ROOM
 * A single static Backrooms-style room made only from flat-color 3D quads.
 * No textures, menu, player model, lights, collision, or gameplay systems.
 */

struct Vec3 { float x, y, z; };

static void project(const GSGLOBAL *gs, const Vec3 &p,
                    float &sx, float &sy, int &sz) {
    const float cameraX = 0.0f;
    const float cameraY = 1.55f;
    const float cameraZ = -2.5f;
    const float depth = p.z - cameraZ;
    const float side = p.x - cameraX;
    const float vertical = p.y - cameraY;
    const float focal = (float)gs->Width * 0.78f;

    sx = (float)gs->Width * 0.5f + side * focal / depth;
    sy = (float)gs->Height * 0.5f - vertical * focal / depth;
    sz = (int)(depth * 1800.0f);
    if (sz < 1) sz = 1;
}

static void quad(GSGLOBAL *gs, const Vec3 &a, const Vec3 &b,
                 const Vec3 &c, const Vec3 &d, u64 color) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    project(gs,a,x1,y1,z1);
    project(gs,b,x2,y2,z2);
    project(gs,c,x3,y3,z3);
    project(gs,d,x4,y4,z4);
    gsKit_prim_quad_3d(gs,x1,y1,z1,x2,y2,z2,
                       x3,y3,z3,x4,y4,z4,color);
}

static void drawRoom(GSGLOBAL *gs) {
    const float left=-3.8f, right=3.8f;
    const float front=0.0f, back=10.0f, floorY=0.0f, ceilingY=3.2f;

    const u64 wall = GS_SETREG_RGBAQ(198,177,91,0x80,0);
    const u64 wallSide = GS_SETREG_RGBAQ(184,163,82,0x80,0);
    const u64 carpet = GS_SETREG_RGBAQ(77,70,49,0x80,0);
    const u64 ceiling = GS_SETREG_RGBAQ(173,169,149,0x80,0);

    /* Far wall first, then side walls, ceiling and floor. */
    quad(gs, Vec3{left,floorY,back}, Vec3{right,floorY,back},
         Vec3{right,ceilingY,back}, Vec3{left,ceilingY,back}, wall);

    quad(gs, Vec3{left,floorY,front}, Vec3{left,floorY,back},
         Vec3{left,ceilingY,back}, Vec3{left,ceilingY,front}, wallSide);

    quad(gs, Vec3{right,floorY,back}, Vec3{right,floorY,front},
         Vec3{right,ceilingY,front}, Vec3{right,ceilingY,back}, wallSide);

    quad(gs, Vec3{left,ceilingY,front}, Vec3{left,ceilingY,back},
         Vec3{right,ceilingY,back}, Vec3{right,ceilingY,front}, ceiling);

    quad(gs, Vec3{left,floorY,back}, Vec3{left,floorY,front},
         Vec3{right,floorY,front}, Vec3{right,floorY,back}, carpet);
}

int main(int, char **) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) for (;;) { }

    gs->Mode = GS_MODE_NTSC;
    gs->Width = 640;
    gs->Height = 448;
    gs->PSM = GS_PSM_CT32;
    gs->PSMZ = GS_PSMZ_16;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->PrimAlphaEnable = GS_SETTING_OFF;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_vram_clear(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);

    for (;;) {
        gsKit_clear(gs, GS_SETREG_RGBAQ(24,32,48,0x80,0));
        drawRoom(gs);
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
