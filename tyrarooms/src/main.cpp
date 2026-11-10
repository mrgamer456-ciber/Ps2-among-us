#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <malloc.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <gsToolkit.h>

static const int TEX_SIZE = 64;
static const int MAP_W = 13;
static const int MAP_H = 13;
static const float CELL = 3.5f;
static const float ROOM_HEIGHT = 3.2f;

static const char *g_map[MAP_H] = {
    "######.######",
    "######.######",
    "##.....######",
    "##.##########",
    "##.##########",
    "##.##########",
    "##...........",
    "##########.##",
    "####.#####.##",
    "####.#####.##",
    "####.......##",
    "########.####",
    "########.####"
};

struct Vec3 { float x, y, z; };
struct Camera { float x, y, z, yaw; };

static GSGLOBAL *g_gs = 0;
static GSTEXTURE g_wall, g_floor, g_ceiling, g_suit;
static GSFONTM *g_loadingFont = 0;
static Camera g_camera = { 0.0f, 1.55f, -1.35f, 0.0f };
static float g_animTime = 0.0f;
static unsigned char g_padBuffer[256] __attribute__((aligned(64)));

static uint32_t packColor(unsigned char r, unsigned char g, unsigned char b) {
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0x80000000u;
}

static unsigned int noise(unsigned int x, unsigned int y, unsigned int seed) {
    unsigned int n = x * 374761393u + y * 668265263u + seed * 1442695041u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return (n ^ (n >> 16)) & 31u;
}

static bool makeTexture(GSTEXTURE *tex, int kind) {
    memset(tex, 0, sizeof(GSTEXTURE));
    tex->Width = TEX_SIZE;
    tex->Height = TEX_SIZE;
    tex->TBW = 1;
    tex->PSM = GS_PSM_CT32;
    tex->Filter = GS_FILTER_LINEAR;
    tex->Mem = (u32 *)memalign(128, TEX_SIZE * TEX_SIZE * sizeof(u32));
    if (!tex->Mem) return false;

    for (unsigned int y = 0; y < TEX_SIZE; ++y) {
        for (unsigned int x = 0; x < TEX_SIZE; ++x) {
            unsigned int n = noise(x, y, (unsigned int)(kind + 7));
            unsigned char r = 0, g = 0, b = 0;
            if (kind == 0) {
                int seam = (x % 32 == 0) ? 9 : 0;
                r = (unsigned char)(190 + n / 3 - seam);
                g = (unsigned char)(174 + n / 3 - seam);
                b = (unsigned char)(91 + n / 4 - seam);
                if ((y % 32) == 0) { r -= 4; g -= 4; }
            } else if (kind == 1) {
                int fleck = (n > 24) ? 9 : 0;
                r = (unsigned char)(65 + n / 2 + fleck);
                g = (unsigned char)(61 + n / 2 + fleck);
                b = (unsigned char)(43 + n / 3);
                if (((x * 3 + y * 5) % 29) == 0) { r -= 8; g -= 8; b -= 5; }
            } else if (kind == 2) {
                /* Plain ceiling material. Fluorescent fixtures are real 3D meshes below. */
                r = (unsigned char)(174 + n / 3);
                g = (unsigned char)(169 + n / 3);
                b = (unsigned char)(148 + n / 3);
                if ((x % 32) < 2 || (y % 32) < 2) { r -= 5; g -= 5; b -= 4; }
            } else {
                int seam = ((x % 32) < 2 || (y % 32) < 2) ? 10 : 0;
                r = (unsigned char)(205 + n / 4 - seam);
                g = (unsigned char)(174 + n / 4 - seam);
                b = (unsigned char)(53 + n / 5 - seam / 2);
                if (x > 42 && x < 59 && y > 36 && y < 57) {
                    r = (unsigned char)(190 + n / 4);
                    g = (unsigned char)(158 + n / 4);
                    b = (unsigned char)(43 + n / 5);
                }
            }
            tex->Mem[y * TEX_SIZE + x] = packColor(r, g, b);
        }
    }
    tex->Vram = gsKit_vram_alloc(g_gs,
        gsKit_texture_size(tex->Width, tex->Height, tex->PSM),
        GSKIT_ALLOC_USERBUFFER);
    if (tex->Vram == GSKIT_ALLOC_ERROR) {
        free(tex->Mem);
        tex->Mem = 0;
        return false;
    }
    gsKit_texture_upload(g_gs, tex);
    return true;
}

static bool projectVertex(const Vec3 &p, float &sx, float &sy, int &sz) {
    const float dx = p.x - g_camera.x;
    const float dz = p.z - g_camera.z;
    const float c = cosf(g_camera.yaw);
    const float s = sinf(g_camera.yaw);
    const float side = dx * c - dz * s;
    const float depth = dx * s + dz * c;
    if (depth < 0.18f) return false;
    const float focal = (float)g_gs->Width * 0.78f;
    sx = (float)g_gs->Width * 0.5f + (side * focal / depth);
    sy = (float)g_gs->Height * 0.5f - ((p.y - g_camera.y) * focal / depth);
    sz = (int)(depth * 1800.0f);
    if (sz < 1) sz = 1;
    return true;
}

static void drawQuad(const GSTEXTURE *tex, const Vec3 &a, const Vec3 &b,
                     const Vec3 &c, const Vec3 &d, float tileU, float tileV) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    if (!projectVertex(a,x1,y1,z1) || !projectVertex(b,x2,y2,z2) ||
        !projectVertex(c,x3,y3,z3) || !projectVertex(d,x4,y4,z4)) return;
    gsKit_prim_quad_texture_3d(g_gs, (GSTEXTURE *)tex,
        x1,y1,z1,0.0f,0.0f, x2,y2,z2,tileU,0.0f,
        x3,y3,z3,tileU,tileV, x4,y4,z4,0.0f,tileV,
        GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
}

static void drawSolidQuad(const Vec3 &a, const Vec3 &b, const Vec3 &c,
                          const Vec3 &d, u64 color) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    if (!projectVertex(a,x1,y1,z1) || !projectVertex(b,x2,y2,z2) ||
        !projectVertex(c,x3,y3,z3) || !projectVertex(d,x4,y4,z4)) return;
    gsKit_prim_quad_3d(g_gs,x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4,color);
}

static void drawSolidBox(const Vec3 &p, float w, float h, float d, u64 color) {
    const float x0=p.x-w*0.5f, x1=p.x+w*0.5f;
    const float y0=p.y-h*0.5f, y1=p.y+h*0.5f;
    const float z0=p.z-d*0.5f, z1=p.z+d*0.5f;
    drawSolidQuad(Vec3{x0,y0,z0},Vec3{x1,y0,z0},Vec3{x1,y1,z0},Vec3{x0,y1,z0},color);
    drawSolidQuad(Vec3{x1,y0,z1},Vec3{x0,y0,z1},Vec3{x0,y1,z1},Vec3{x1,y1,z1},color);
    drawSolidQuad(Vec3{x0,y0,z1},Vec3{x0,y0,z0},Vec3{x0,y1,z0},Vec3{x0,y1,z1},color);
    drawSolidQuad(Vec3{x1,y0,z0},Vec3{x1,y0,z1},Vec3{x1,y1,z1},Vec3{x1,y1,z0},color);
    drawSolidQuad(Vec3{x0,y1,z0},Vec3{x1,y1,z0},Vec3{x1,y1,z1},Vec3{x0,y1,z1},color);
    drawSolidQuad(Vec3{x0,y0,z1},Vec3{x1,y0,z1},Vec3{x1,y0,z0},Vec3{x0,y0,z0},color);
}

static bool isOpen(int row, int col) {
    if (row < 0 || row >= MAP_H || col < 0 || col >= MAP_W) return false;
    return g_map[row][col] == '.';
}

static float cellX(int col) { return ((float)col - 6.0f) * CELL; }
static float cellZ(int row) { return (float)row * CELL + CELL * 0.5f; }

static void drawWallSegment(int row, int col, int side) {
    /* Leave the south wall open at the final tile: this is the actual exit. */
    if (row == 12 && col == 8 && side == 1) return;
    const float cx=cellX(col), cz=cellZ(row), h=ROOM_HEIGHT;
    const float half=CELL*0.5f;
    if (side==0) {
        drawQuad(&g_wall,Vec3{cx-half,0,cz-half},Vec3{cx-half,h,cz-half},
                 Vec3{cx+half,h,cz-half},Vec3{cx+half,0,cz-half},2,2);
    } else if (side==1) {
        drawQuad(&g_wall,Vec3{cx+half,0,cz+half},Vec3{cx+half,h,cz+half},
                 Vec3{cx-half,h,cz+half},Vec3{cx-half,0,cz+half},2,2);
    } else if (side==2) {
        drawQuad(&g_wall,Vec3{cx-half,0,cz+half},Vec3{cx-half,h,cz+half},
                 Vec3{cx-half,h,cz-half},Vec3{cx-half,0,cz-half},2,2);
    } else {
        drawQuad(&g_wall,Vec3{cx+half,0,cz-half},Vec3{cx+half,h,cz-half},
                 Vec3{cx+half,h,cz+half},Vec3{cx+half,0,cz+half},2,2);
    }
}

/* Fluorescent lamp: housing and luminous diffuser are separate 3D box meshes. */
static void drawFluorescentLamp(float x, float z) {
    drawSolidBox(Vec3{x,3.105f,z},1.55f,0.13f,0.62f,
                 GS_SETREG_RGBAQ(74,72,65,0x80,0));
    drawSolidBox(Vec3{x,3.025f,z},1.30f,0.035f,0.43f,
                 GS_SETREG_RGBAQ(248,248,224,0x80,0));
    drawSolidBox(Vec3{x,3.145f,z},1.40f,0.025f,0.50f,
                 GS_SETREG_RGBAQ(115,112,100,0x80,0));
}

static void drawFloorArrow(float x, float z, int direction) {
    const float y=0.035f, shaft=0.52f, half=0.075f, head=0.34f;
    /* Direction: 0 north(-Z), 1 east(+X), 2 south(+Z), 3 west(-X). */
    float dx=0.0f, dz=1.0f;
    if(direction==0) dz=-1.0f;
    if(direction==1) { dx=1.0f; dz=0.0f; }
    if(direction==3) { dx=-1.0f; dz=0.0f; }
    const float px=-dz, pz=dx;
    const float bx=x-dx*0.05f, bz=z-dz*0.05f;
    const float tx=x+dx*shaft, tz=z+dz*shaft;
    const u64 black=GS_SETREG_RGBAQ(8,8,7,0x80,0);
    drawSolidQuad(Vec3{bx+px*half,y,bz+pz*half},
                  Vec3{bx-px*half,y,bz-pz*half},
                  Vec3{tx-px*half,y,tz-pz*half},
                  Vec3{tx+px*half,y,tz+pz*half},black);
    const float hx=x+dx*(shaft+head);
    const float hz=z+dz*(shaft+head);
    drawSolidQuad(Vec3{tx+px*half,y,tz+pz*half},
                  Vec3{tx-px*half,y,tz-pz*half},
                  Vec3{hx,y,hz},
                  Vec3{hx,y,hz},black);
    /* Two thick diagonal arrowhead wings, also real 3D geometry. */
    const float wx=px*0.23f, wz=pz*0.23f;
    drawSolidQuad(Vec3{tx+px*half,y,tz+pz*half},
                  Vec3{tx+px*half+dx*0.12f,y,tz+pz*half+dz*0.12f},
                  Vec3{hx,y,hz},
                  Vec3{hx-wx*0.22f,y,hz-wz*0.22f},black);
    drawSolidQuad(Vec3{tx-px*half,y,tz-pz*half},
                  Vec3{tx-px*half+dx*0.12f,y,tz-pz*half+dz*0.12f},
                  Vec3{hx,y,hz},
                  Vec3{hx+wx*0.22f,y,hz+wz*0.22f},black);
}

static void renderMaze() {
    for (int r=0;r<MAP_H;++r) {
        for (int c=0;c<MAP_W;++c) {
            if (!isOpen(r,c)) continue;
            const float x=cellX(c), z=cellZ(r), q=CELL*0.5f;
            drawQuad(&g_floor,Vec3{x-q,0,z-q},Vec3{x+q,0,z-q},
                     Vec3{x+q,0,z+q},Vec3{x-q,0,z+q},3,3);
            drawQuad(&g_ceiling,Vec3{x-q,ROOM_HEIGHT,z+q},
                     Vec3{x+q,ROOM_HEIGHT,z+q},Vec3{x+q,ROOM_HEIGHT,z-q},
                     Vec3{x-q,ROOM_HEIGHT,z-q},2,2);

            if (!isOpen(r-1,c)) drawWallSegment(r,c,0);
            if (!isOpen(r+1,c)) drawWallSegment(r,c,1);
            if (!isOpen(r,c-1)) drawWallSegment(r,c,2);
            if (!isOpen(r,c+1)) drawWallSegment(r,c,3);

            if (((r*3+c*5)%4)==0) drawFluorescentLamp(x,z);
        }
    }

    /* Black floor arrows at the junctions, pointing along the route to the exit. */
    drawFloorArrow(cellX(6),cellZ(1),2);
    drawFloorArrow(cellX(4),cellZ(2),3);
    drawFloorArrow(cellX(2),cellZ(4),2);
    drawFloorArrow(cellX(5),cellZ(6),1);
    drawFloorArrow(cellX(10),cellZ(8),2);
    drawFloorArrow(cellX(8),cellZ(10),3);
    drawFloorArrow(cellX(8),cellZ(11),2);

    /* 3D exit frame and green marker beyond the last corridor tile. */
    const float exitZ = cellZ(12) + CELL * 0.5f;
    drawSolidBox(Vec3{cellX(8)-0.78f,1.50f,exitZ},0.16f,3.0f,0.16f,
                 GS_SETREG_RGBAQ(90,83,61,0x80,0));
    drawSolidBox(Vec3{cellX(8)+0.78f,1.50f,exitZ},0.16f,3.0f,0.16f,
                 GS_SETREG_RGBAQ(90,83,61,0x80,0));
    drawSolidBox(Vec3{cellX(8),2.94f,exitZ},1.70f,0.16f,0.16f,
                 GS_SETREG_RGBAQ(90,83,61,0x80,0));
    drawSolidBox(Vec3{cellX(8),2.66f,exitZ+0.02f},0.95f,0.34f,0.12f,
                 GS_SETREG_RGBAQ(35,130,54,0x80,0));
}


static void drawLoadingScreen(const char *status, int progress) {
    if (!g_loadingFont) return;
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;

    const float screenW = (float)g_gs->Width;
    const float screenH = (float)g_gs->Height;
    const float barLeft = screenW * 0.16f;
    const float barRight = screenW * 0.84f;
    const float barTop = screenH * 0.84f;
    const float barBottom = barTop + 15.0f;

    gsKit_clear(g_gs, GS_SETREG_RGBAQ(12,11,9,0x80,0));
    gsKit_prim_sprite(g_gs, 0, 0, screenW, screenH, 1,
                      GS_SETREG_RGBAQ(12,11,9,0x80,0));

    /* The status line is centered in the upper-middle of the screen. */
    gsKit_fontm_print_scaled(g_gs, g_loadingFont,
        screenW * 0.5f - 132.0f, screenH * 0.34f, 2, 0.72f,
        GS_SETREG_RGBAQ(235,222,175,0x80,0), "BACKROOMS - LEVEL 0");
    gsKit_fontm_print_scaled(g_gs, g_loadingFont,
        screenW * 0.5f - 150.0f, screenH * 0.46f, 2, 0.62f,
        GS_SETREG_RGBAQ(238,236,220,0x80,0), status);

    /* Actual percentage-backed progress bar, updated after each load task. */
    gsKit_prim_sprite(g_gs, barLeft-3.0f, barTop-3.0f,
                      barRight+3.0f, barBottom+3.0f, 2,
                      GS_SETREG_RGBAQ(105,96,67,0x80,0));
    gsKit_prim_sprite(g_gs, barLeft, barTop, barRight, barBottom, 3,
                      GS_SETREG_RGBAQ(35,33,27,0x80,0));
    const float fillRight = barLeft + (barRight-barLeft) * ((float)progress/100.0f);
    if (progress > 0) {
        gsKit_prim_sprite(g_gs, barLeft+2.0f, barTop+2.0f,
                          fillRight-2.0f, barBottom-2.0f, 4,
                          GS_SETREG_RGBAQ(205,177,77,0x80,0));
    }

    char percent[12];
    sprintf(percent, "%d%%", progress);
    gsKit_fontm_print_scaled(g_gs, g_loadingFont,
        screenW * 0.5f - 12.0f, barBottom + 13.0f, 2, 0.55f,
        GS_SETREG_RGBAQ(230,224,202,0x80,0), percent);

    gsKit_queue_exec(g_gs);
    gsKit_sync_flip(g_gs);
}

static bool validateMazeData() {
    int openTiles = 0;
    for (int row=0; row<MAP_H; ++row) {
        for (int col=0; col<MAP_W; ++col) {
            if (isOpen(row,col)) ++openTiles;
        }
    }
    /* Spawn and exit must both be inside walkable cells. */
    return openTiles > 0 && isOpen(0,6) && isOpen(12,8);
}

static Vec3 rotateAroundX(const Vec3 &p,const Vec3 &pivot,float angle) {
    const float dy=p.y-pivot.y, dz=p.z-pivot.z;
    const float c=cosf(angle), sn=sinf(angle);
    return Vec3{p.x,pivot.y+dy*c-dz*sn,pivot.z+dy*sn+dz*c};
}

static void drawBoxPart(const Vec3 &pivot,const Vec3 &offset,
                        float width,float height,float depth,float swing,
                        const GSTEXTURE *texture) {
    const Vec3 center={pivot.x+offset.x,pivot.y+offset.y,pivot.z+offset.z};
    const float x0=-width*.5f,x1=width*.5f,y0=-height*.5f,y1=height*.5f;
    const float z0=-depth*.5f,z1=depth*.5f;
    Vec3 v[8]={
        {center.x+x0,center.y+y0,center.z+z0},{center.x+x1,center.y+y0,center.z+z0},
        {center.x+x1,center.y+y1,center.z+z0},{center.x+x0,center.y+y1,center.z+z0},
        {center.x+x0,center.y+y0,center.z+z1},{center.x+x1,center.y+y0,center.z+z1},
        {center.x+x1,center.y+y1,center.z+z1},{center.x+x0,center.y+y1,center.z+z1}
    };
    for(int i=0;i<8;++i) v[i]=rotateAroundX(v[i],pivot,swing);
    drawQuad(texture,v[0],v[1],v[2],v[3],1,1);
    drawQuad(texture,v[5],v[4],v[7],v[6],1,1);
    drawQuad(texture,v[4],v[0],v[3],v[7],1,1);
    drawQuad(texture,v[1],v[5],v[6],v[2],1,1);
    drawQuad(texture,v[3],v[2],v[6],v[7],1,1);
    drawQuad(texture,v[4],v[5],v[1],v[0],1,1);
}

static void drawYellowMaskedCharacter() {
    const float walk=sinf(g_animTime*6.0f);
    const float opposite=sinf(g_animTime*6.0f+3.14159265f);
    const float bob=fabsf(walk)*0.045f;
    const float z=4.7f, torsoY=1.48f+bob;
    drawBoxPart(Vec3{-0.19f,0.91f+bob,z},Vec3{0,-0.39f,0},.34f,.82f,.38f,walk*.48f,&g_suit);
    drawBoxPart(Vec3{0.19f,0.91f+bob,z},Vec3{0,-0.39f,0},.34f,.82f,.38f,opposite*.48f,&g_suit);
    drawBoxPart(Vec3{0,torsoY,z},Vec3{0,0,0},.86f,.98f,.48f,0,&g_suit);
    drawBoxPart(Vec3{-0.53f,1.86f+bob,z},Vec3{0,-.37f,0},.30f,.78f,.36f,opposite*.58f,&g_suit);
    drawBoxPart(Vec3{0.53f,1.86f+bob,z},Vec3{0,-.37f,0},.30f,.78f,.36f,walk*.58f,&g_suit);
    const Vec3 head={0,2.48f+bob,z};
    drawBoxPart(head,Vec3{0,0,0},.66f,.66f,.62f,sinf(g_animTime*3)*.035f,&g_suit);
    const float fz=z-.315f, fy=head.y-.04f;
    drawSolidQuad(Vec3{-.23f,fy-.14f,fz},Vec3{.23f,fy-.14f,fz},
                  Vec3{.23f,fy+.13f,fz},Vec3{-.23f,fy+.13f,fz},
                  GS_SETREG_RGBAQ(38,43,42,0x80,0));
    drawSolidQuad(Vec3{-.17f,fy+.015f,fz-.006f},Vec3{-.035f,fy+.015f,fz-.006f},
                  Vec3{-.035f,fy+.085f,fz-.006f},Vec3{-.17f,fy+.085f,fz-.006f},
                  GS_SETREG_RGBAQ(145,160,145,0x80,0));
    drawSolidQuad(Vec3{.035f,fy+.015f,fz-.006f},Vec3{.17f,fy+.015f,fz-.006f},
                  Vec3{.17f,fy+.085f,fz-.006f},Vec3{.035f,fy+.085f,fz-.006f},
                  GS_SETREG_RGBAQ(145,160,145,0x80,0));
}

static bool canStand(float x,float z) {
    /* A short entry apron connects the starting position to the first maze tile. */
    if (z>=-2.2f && z<0.0f && fabsf(x)<1.25f) return true;
    const int col=(int)floorf(x/CELL+6.5f);
    const int row=(int)floorf(z/CELL);
    if (!isOpen(row,col)) return false;
    /* Keep the camera slightly away from the wall edges. */
    const float localX=x-cellX(col), localZ=z-cellZ(row);
    return fabsf(localX)<CELL*.39f && fabsf(localZ)<CELL*.39f;
}

static void updateController() {
    if (padGetState(0,0)!=PAD_STATE_STABLE) return;
    padButtonStatus buttons;
    if (padRead(0,0,&buttons)==0) return;
    const unsigned short b=buttons.btns;
    const float turn=.035f, speed=.075f;
    if (!(b & PAD_L1)) g_camera.yaw-=turn;
    if (!(b & PAD_R1)) g_camera.yaw+=turn;
    float forward=0.0f, strafe=0.0f;
    if (!(b & PAD_UP)) forward+=speed;
    if (!(b & PAD_DOWN)) forward-=speed;
    if (!(b & PAD_RIGHT)) strafe+=speed;
    if (!(b & PAD_LEFT)) strafe-=speed;
    const float fx=sinf(g_camera.yaw), fz=cosf(g_camera.yaw);
    const float nx=g_camera.x+fx*forward+cosf(g_camera.yaw)*strafe;
    const float nz=g_camera.z+fz*forward-sinf(g_camera.yaw)*strafe;
    if (canStand(nx,g_camera.z)) g_camera.x=nx;
    if (canStand(g_camera.x,nz)) g_camera.z=nz;
}

static bool initGraphicsAndLoadLevel() {
    g_gs=gsKit_init_global();
    if (!g_gs) return false;
    g_gs->PSM=GS_PSM_CT24;
    g_gs->PSMZ=GS_PSMZ_24;
    g_gs->ZBuffering=GS_SETTING_ON;
    g_gs->PrimAlphaEnable=GS_SETTING_ON;
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_vram_clear(g_gs);
    gsKit_init_screen(g_gs);
    gsKit_set_test(g_gs,GS_ZTEST_ON);
    gsKit_set_clamp(g_gs,GS_CMODE_REPEAT);
    gsKit_mode_switch(g_gs,GS_ONESHOT);

    g_loadingFont = gsKit_init_fontm();
    if (!g_loadingFont) return false;
    gsKit_fontm_upload(g_gs,g_loadingFont);
    drawLoadingScreen("INICIANDO GRAFICOS",5);

    drawLoadingScreen("CRIANDO TEXTURA DAS PAREDES",10);
    if (!makeTexture(&g_wall,0)) return false;
    drawLoadingScreen("TEXTURA DAS PAREDES CARREGADA",25);

    drawLoadingScreen("CRIANDO TEXTURA DO CARPETE",30);
    if (!makeTexture(&g_floor,1)) return false;
    drawLoadingScreen("TEXTURA DO CARPETE CARREGADA",45);

    drawLoadingScreen("CRIANDO TEXTURA DO TETO",50);
    if (!makeTexture(&g_ceiling,2)) return false;
    drawLoadingScreen("TEXTURA DO TETO CARREGADA",65);

    drawLoadingScreen("CRIANDO TEXTURA DO PERSONAGEM",70);
    if (!makeTexture(&g_suit,3)) return false;
    drawLoadingScreen("TEXTURA DO PERSONAGEM CARREGADA",85);

    drawLoadingScreen("MONTANDO ROTAS DO LABIRINTO",90);
    if (!validateMazeData()) return false;
    padInit(0);
    padPortOpen(0,0,g_padBuffer);
    drawLoadingScreen("FINALIZANDO CONTROLES",96);
    drawLoadingScreen("CARREGAMENTO CONCLUIDO",100);
    return true;
}

int main(int,char **) {
    if (!initGraphicsAndLoadLevel()) {
        for (;;) { }
    }
    for (;;) {
        updateController();
        gsKit_clear(g_gs,GS_SETREG_RGBAQ(18,17,14,0x80,0));
        renderMaze();
        drawYellowMaskedCharacter();
        g_animTime+=1.0f/60.0f;
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
    }
    return 0;
}
