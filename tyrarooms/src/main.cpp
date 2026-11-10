#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <gsToolkit.h>
#include "minecraft_skin.h"
#include "menu_background.h"

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
static float g_cameraPitch = 0.0f;
static float g_verticalVelocity = 0.0f;
static float g_jumpHeight = 0.0f;
static float g_moveAmount = 0.0f;
static bool g_grounded = true;
static bool g_running = false;
static bool g_crouching = false;
static bool g_previousL3 = false;
static bool g_previousJump = false;
static bool g_previousCircle = false;
static bool g_playerDead = false;
static float g_health = 100.0f;
static float g_stamina = 100.0f;
static bool g_waterCollected = false;
static float g_waterMessageTimer = 0.0f;
static const float g_waterX = 0.0f;
static const float g_waterZ = 22.75f;
static float g_deathTimer = 0.0f;
static float g_deathTilt = 0.0f;
static float g_deathLimbFlop = 0.0f;
static float g_deathX = 0.0f, g_deathZ = 0.0f, g_deathYaw = 0.0f;

static GSGLOBAL *g_gs = 0;
static GSTEXTURE g_wall, g_floor, g_ceiling, g_suit, g_menuBackground;
static int g_difficulty = 1; // 0=facil, 1=normal, 2=dificil
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

static uint32_t wallpaperHash(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2147483647u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static float wallpaperNoise(float x, float y, int period, int seed) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - (float)x0, fy = y - (float)y0;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    auto value = [&](int ix, int iy) {
        int px = ((ix % period) + period) % period;
        int py = ((iy % period) + period) % period;
        return (float)(wallpaperHash(px, py, seed) & 0xFFFF) / 65535.0f;
    };
    float a = value(x0, y0), b = value(x0 + 1, y0);
    float c = value(x0, y0 + 1), d = value(x0 + 1, y0 + 1);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

static unsigned char clampTextureChannel(float v) {
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}

static bool makeTexture(GSTEXTURE *tex, int kind) {
    const int texSize = (kind == 0) ? 128 : TEX_SIZE;
    memset(tex, 0, sizeof(GSTEXTURE));
    tex->Width = texSize;
    tex->Height = texSize;
    tex->TBW = (texSize + 63) / 64;
    tex->PSM = GS_PSM_CT32;
    tex->Filter = GS_FILTER_LINEAR;
    tex->Mem = (u32 *)memalign(128, texSize * texSize * sizeof(u32));
    if (!tex->Mem) return false;

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            unsigned int n = noise((unsigned int)x, (unsigned int)y, (unsigned int)(kind + 7));
            unsigned char r = 0, g = 0, b = 0;
            if (kind == 0) {
                /* Exact Claude-era v0.3 Backrooms wallpaper from Backrooms-in-ps2/textures.cpp. */
                const int cell = texSize / 4;
                float fr = 205.0f, fg = 186.0f, fb = 84.0f;
                float k = 1.0f;
                int sx = x % cell;
                if (sx < 4) k *= 0.86f;
                else if (sx == 4) k *= 1.05f;
                int row = y / cell, oy = y % cell;
                int ox = (sx + (row & 1) * (cell / 2)) % cell;
                int d = abs(ox - cell / 2) + abs(oy - cell / 2);
                if (d < 7) k *= 0.90f;
                else if (d == 7) k *= 0.96f;
                float stain = wallpaperNoise((float)x * 8.0f / (float)texSize,
                                             (float)y * 8.0f / (float)texSize, 8, 3);
                k *= 0.86f + 0.14f * stain;
                float grain = ((float)(wallpaperHash(x, y, 7) & 255) / 255.0f - 0.5f) * 10.0f;
                if (y >= texSize - 10) {
                    fr = 96.0f; fg = 74.0f; fb = 42.0f;
                    k = (y == texSize - 10) ? 1.2f : 0.92f;
                } else if (y < 4) {
                    k *= 0.80f;
                }
                r = clampTextureChannel(fr * k + grain);
                g = clampTextureChannel(fg * k + grain);
                b = clampTextureChannel(fb * k + grain);
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
                /* Fallback only; the Minecraft skin atlas is copied below. */
                r = 0; g = 0; b = 0;
            }
            tex->Mem[y * texSize + x] = packColor(r, g, b);
        }
    }
    if (kind == 3) {
        for (int y=0; y<TEX_SIZE; ++y)
            for (int x=0; x<TEX_SIZE; ++x)
                tex->Mem[y*TEX_SIZE+x] = minecraftSkinPixels[y*TEX_SIZE+x];
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
    const float cp = cosf(g_cameraPitch), sp = sinf(g_cameraPitch);
    const float vertical = p.y - g_camera.y;
    const float viewY = vertical * cp - depth * sp;
    const float viewDepth = depth * cp + vertical * sp;
    if (viewDepth < 0.18f) return false;
    const float focal = (float)g_gs->Width * 0.78f;
    sx = (float)g_gs->Width * 0.5f + (side * focal / viewDepth);
    sy = (float)g_gs->Height * 0.5f - (viewY * focal / viewDepth);
    sz = (int)(viewDepth * 1800.0f);
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

static void drawQuadUV(const GSTEXTURE *tex, const Vec3 &a, const Vec3 &b,
                         const Vec3 &c, const Vec3 &d,
                         float u0, float v0, float u1, float v1) {
    float x1,y1,x2,y2,x3,y3,x4,y4;
    int z1,z2,z3,z4;
    if (!projectVertex(a,x1,y1,z1) || !projectVertex(b,x2,y2,z2) ||
        !projectVertex(c,x3,y3,z3) || !projectVertex(d,x4,y4,z4)) return;
    gsKit_prim_quad_texture_3d(g_gs, (GSTEXTURE *)tex,
        x1,y1,z1,u0,v0, x2,y2,z2,u1,v0,
        x3,y3,z3,u1,v1, x4,y4,z4,u0,v1,
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

/* Low-poly water bottle assembled from solid 3D geometry, not a billboard texture. */
static void drawWaterBottle() {
    if (g_waterCollected) return;
    const float x=g_waterX, z=g_waterZ;
    drawSolidBox(Vec3{x,0.245f,z},0.31f,0.48f,0.27f,
                 GS_SETREG_RGBAQ(65,177,205,0x80,0));
    drawSolidBox(Vec3{x,0.255f,z-0.006f},0.235f,0.34f,0.205f,
                 GS_SETREG_RGBAQ(35,126,190,0x80,0));
    drawSolidBox(Vec3{x,0.515f,z},0.255f,0.13f,0.235f,
                 GS_SETREG_RGBAQ(100,205,220,0x80,0));
    drawSolidBox(Vec3{x,0.625f,z},0.145f,0.12f,0.145f,
                 GS_SETREG_RGBAQ(112,215,225,0x80,0));
    drawSolidBox(Vec3{x,0.705f,z},0.17f,0.075f,0.17f,
                 GS_SETREG_RGBAQ(30,91,155,0x80,0));
    drawSolidBox(Vec3{x-0.105f,0.27f,z-0.139f},0.025f,0.30f,0.012f,
                 GS_SETREG_RGBAQ(185,235,240,0x80,0));
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

    drawWaterBottle();

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



/* Unified 5x7 pixel font used by every on-screen text element. */
static void drawPixelText(float x, float y, const char *text, float scale, u64 color) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-!:%.";
    static const unsigned char glyphs[][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14},
        {0,0,0,31,0,0,0},{4,4,4,4,4,0,4},{0,4,4,0,4,4,0},
        {17,10,4,4,4,10,17},{0,0,0,0,0,0,31}
    };
    if (!text || scale <= 0.0f) return;
    for (const char *p=text; *p; ++p) {
        char ch=*p;
        if (ch>='a' && ch<='z') ch=(char)(ch-'a'+'A');
        const char *found=strchr(alphabet,ch);
        if (found) {
            const int index=(int)(found-alphabet);
            for (int row=0; row<7; ++row) {
                for (int col=0; col<5; ++col) {
                    if (glyphs[index][row] & (1u << (4-col))) {
                        const float px=x+(float)col*scale;
                        const float py=y+(float)row*scale;
                        gsKit_prim_sprite(g_gs,px,py,px+scale,py+scale,1,color);
                    }
                }
            }
        }
        x += 6.0f*scale;
    }
}

static void drawPixelTextCentered(float centerX, float y, const char *text,
                                  float scale, u64 color) {
    const float width=text ? (float)strlen(text)*6.0f*scale : 0.0f;
    drawPixelText(centerX-width*0.5f,y,text,scale,color);
}


static bool makeMenuBackgroundTexture() {
    memset(&g_menuBackground, 0, sizeof(GSTEXTURE));
    g_menuBackground.Width = 512;
    g_menuBackground.Height = 512;
    g_menuBackground.TBW = 8;
    g_menuBackground.PSM = GS_PSM_CT32;
    g_menuBackground.Filter = GS_FILTER_LINEAR;
    g_menuBackground.Mem = (u32 *)memalign(128, 512 * 512 * sizeof(u32));
    if (!g_menuBackground.Mem) return false;
    for (int i = 0; i < 512 * 512; ++i)
        g_menuBackground.Mem[i] = 0x80000000u;
    for (int y = 0; y < 384; ++y)
        for (int x = 0; x < 512; ++x)
            g_menuBackground.Mem[y * 512 + x] = menuBackgroundPixels[y * 512 + x];
    g_menuBackground.Vram = gsKit_vram_alloc(g_gs,
        gsKit_texture_size(g_menuBackground.Width, g_menuBackground.Height,
                           g_menuBackground.PSM), GSKIT_ALLOC_USERBUFFER);
    if (g_menuBackground.Vram == GSKIT_ALLOC_ERROR) {
        free(g_menuBackground.Mem);
        g_menuBackground.Mem = 0;
        return false;
    }
    gsKit_texture_upload(g_gs, &g_menuBackground);
    return true;
}

static void drawMenuPanel(float left, float top, float right, float bottom,
                          bool selected) {
    const u64 border = selected
        ? GS_SETREG_RGBAQ(224, 194, 120, 0x80, 0)
        : GS_SETREG_RGBAQ(130, 119, 91, 0x60, 0);
    const u64 fill = GS_SETREG_RGBAQ(13, 12, 9, 0x70, 0);
    gsKit_prim_sprite(g_gs, left, top, right, bottom, 2, border);
    gsKit_prim_sprite(g_gs, left + 3, top + 3, right - 3, bottom - 3, 3, fill);
}

static void drawMainMenuScreen(int page, int selected) {
    const float sw = (float)g_gs->Width;
    const float sh = (float)g_gs->Height;
    gsKit_clear(g_gs, GS_SETREG_RGBAQ(5, 5, 4, 0x80, 0));
    gsKit_prim_sprite_texture(g_gs, &g_menuBackground,
        0.0f, 0.0f, 0.0f, 0.0f, sw, sh, 512.0f, 384.0f,
        1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0));

    /* A restrained dark overlay keeps the original BMP visible behind the menu. */
    gsKit_prim_sprite(g_gs, sw * 0.27f, sh * 0.10f,
                      sw * 0.73f, sh * 0.91f, 2,
                      GS_SETREG_RGBAQ(8, 7, 5, 0x55, 0));

    if (page == 0) {
        drawPixelTextCentered(sw * 0.5f, sh * 0.20f, "BACKROOMS", 4.0f,
            GS_SETREG_RGBAQ(245, 226, 171, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.31f, "LEVEL 0", 2.0f,
            GS_SETREG_RGBAQ(225, 216, 190, 0x80, 0));
        const char *items[] = {"INICIAR", "OPCOES", "CREDITOS"};
        for (int i = 0; i < 3; ++i) {
            const float y = sh * (0.45f + 0.15f * (float)i);
            drawMenuPanel(sw * 0.36f, y - 7.0f, sw * 0.64f, y + 29.0f, i == selected);
            if (i == selected)
                drawPixelText(sw * 0.385f, y + 3.0f, ">", 3.0f,
                    GS_SETREG_RGBAQ(255, 220, 130, 0x80, 0));
            drawPixelTextCentered(sw * 0.5f, y + 3.0f, items[i], 2.5f,
                i == selected ? GS_SETREG_RGBAQ(255, 239, 190, 0x80, 0)
                              : GS_SETREG_RGBAQ(220, 216, 199, 0x80, 0));
        }
        drawPixelTextCentered(sw * 0.5f, sh * 0.84f, "D-PAD: MOVER  X: SELECIONAR", 1.35f,
            GS_SETREG_RGBAQ(225, 218, 194, 0x80, 0));
    } else if (page == 1) {
        drawPixelTextCentered(sw * 0.5f, sh * 0.18f, "OPCOES", 3.5f,
            GS_SETREG_RGBAQ(245, 226, 171, 0x80, 0));
        drawMenuPanel(sw * 0.32f, sh * 0.34f, sw * 0.68f, sh * 0.51f, true);
        drawPixelTextCentered(sw * 0.5f, sh * 0.38f, "DIFICULDADE", 2.0f,
            GS_SETREG_RGBAQ(245, 239, 216, 0x80, 0));
        const char *levels[] = {"FACIL", "NORMAL", "DIFICIL"};
        drawPixelTextCentered(sw * 0.5f, sh * 0.45f, levels[g_difficulty], 2.2f,
            GS_SETREG_RGBAQ(255, 215, 125, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.60f, "ESQUERDA / DIREITA: ALTERAR", 1.45f,
            GS_SETREG_RGBAQ(230, 224, 205, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.69f, "ANALOGICO: MOVER E OLHAR", 1.45f,
            GS_SETREG_RGBAQ(230, 224, 205, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.75f, "X: PULAR  L3: AGACHAR", 1.45f,
            GS_SETREG_RGBAQ(230, 224, 205, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.86f, "O: VOLTAR", 1.6f,
            GS_SETREG_RGBAQ(255, 220, 145, 0x80, 0));
    } else {
        drawPixelTextCentered(sw * 0.5f, sh * 0.18f, "CREDITOS", 3.5f,
            GS_SETREG_RGBAQ(245, 226, 171, 0x80, 0));
        drawMenuPanel(sw * 0.30f, sh * 0.32f, sw * 0.70f, sh * 0.72f, false);
        drawPixelTextCentered(sw * 0.5f, sh * 0.40f, "PROJETO E DIRECAO", 1.8f,
            GS_SETREG_RGBAQ(220, 211, 184, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.47f, "DAVI", 2.8f,
            GS_SETREG_RGBAQ(255, 225, 150, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.56f, "PROGRAMACAO E APOIO", 1.8f,
            GS_SETREG_RGBAQ(220, 211, 184, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.63f, "CHATGPT - OPENAI", 2.3f,
            GS_SETREG_RGBAQ(255, 225, 150, 0x80, 0));
        drawPixelTextCentered(sw * 0.5f, sh * 0.84f, "O: VOLTAR", 1.6f,
            GS_SETREG_RGBAQ(255, 220, 145, 0x80, 0));
    }
}

static void runMainMenu() {
    if (!makeMenuBackgroundTexture()) return;
    int page = 0;
    int selected = 0;
    bool prevUp = false, prevDown = false, prevLeft = false, prevRight = false;
    bool prevCross = false, prevCircle = false;
    for (;;) {
        if (padGetState(0, 0) == PAD_STATE_STABLE) {
            padButtonStatus buttons;
            if (padRead(0, 0, &buttons) != 0) {
                const unsigned short b = buttons.btns;
                const bool up = !(b & PAD_UP), down = !(b & PAD_DOWN);
                const bool left = !(b & PAD_LEFT), right = !(b & PAD_RIGHT);
                const bool cross = !(b & PAD_CROSS), circle = !(b & PAD_CIRCLE);
                if (page == 0) {
                    if (up && !prevUp) selected = (selected + 2) % 3;
                    if (down && !prevDown) selected = (selected + 1) % 3;
                    if (cross && !prevCross) {
                        if (selected == 0) break;
                        page = selected == 1 ? 1 : 2;
                    }
                } else if (page == 1) {
                    if (left && !prevLeft) g_difficulty = (g_difficulty + 2) % 3;
                    if (right && !prevRight) g_difficulty = (g_difficulty + 1) % 3;
                    if (circle && !prevCircle) page = 0;
                } else if (circle && !prevCircle) {
                    page = 0;
                }
                prevUp = up; prevDown = down; prevLeft = left; prevRight = right;
                prevCross = cross; prevCircle = circle;
            }
        }
        drawMainMenuScreen(page, selected);
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
    }
    /* Finish the last menu frame before releasing its resources. */
    for (int i = 0; i < 2; ++i) {
        gsKit_clear(g_gs, GS_SETREG_RGBAQ(5, 5, 4, 0x80, 0));
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
    }

    /* The menu texture is the last VRAM allocation made by this program.
       Rewind gsKit's allocation pointer to its start so the game can reuse it. */
    dmaKit_wait_fast();
    if (g_menuBackground.Mem) {
        free(g_menuBackground.Mem);
        g_menuBackground.Mem = 0;
    }
    if (g_menuBackground.Vram != GSKIT_ALLOC_ERROR) {
        g_gs->CurrentPointer = g_menuBackground.Vram;
    }
    memset(&g_menuBackground, 0, sizeof(GSTEXTURE));
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
    drawPixelTextCentered(screenW*0.5f,screenH*0.34f,"BACKROOMS - LEVEL 0",2.0f,
        GS_SETREG_RGBAQ(235,222,175,0x80,0));
    drawPixelTextCentered(screenW*0.5f,screenH*0.46f,status,2.0f,
        GS_SETREG_RGBAQ(238,236,220,0x80,0));

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
    drawPixelTextCentered(screenW*0.5f,barBottom+13.0f,percent,2.0f,
        GS_SETREG_RGBAQ(230,224,202,0x80,0));

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

static Vec3 viewToWorld(float x, float y, float z) {
    const float rightX=cosf(g_camera.yaw), rightZ=-sinf(g_camera.yaw);
    const float forwardX=sinf(g_camera.yaw), forwardZ=cosf(g_camera.yaw);
    return Vec3{g_camera.x+rightX*x+forwardX*z,
                g_camera.y+y,
                g_camera.z+rightZ*x+forwardZ*z};
}

/* First-person sleeves: the arms stay attached to the camera like DOORS. */
static void drawFirstPersonArm(float side, float swing, float bob) {
    const float cx=side*0.34f;
    const float cy=-0.37f+bob;
    const float cz=0.60f;
    const float hx=0.135f, hy=0.31f, hz=0.14f;
    Vec3 v[8];
    const float xs[8]={-hx,hx,hx,-hx,-hx,hx,hx,-hx};
    const float ys[8]={-hy,-hy,hy,hy,-hy,-hy,hy,hy};
    const float zs[8]={-hz,-hz,-hz,-hz,hz,hz,hz,hz};
    const float c=cosf(swing), sn=sinf(swing);
    for(int i=0;i<8;++i) {
        const float ry=ys[i]*c-zs[i]*sn;
        const float rz=ys[i]*sn+zs[i]*c;
        v[i]=viewToWorld(cx+xs[i],cy+ry,cz+rz);
    }
    /* Use the matching classic 64x64 atlas region for each arm.
       Right arm occupies x=40..56/y=16..32; left arm x=32..48/y=48..64. */
    if (side < 0.0f) {
        drawQuadUV(&g_suit,v[0],v[1],v[2],v[3],44.0f/64,20.0f/64,48.0f/64,32.0f/64); // right arm front
        drawQuadUV(&g_suit,v[5],v[4],v[7],v[6],52.0f/64,20.0f/64,56.0f/64,32.0f/64); // back
        drawQuadUV(&g_suit,v[4],v[0],v[3],v[7],40.0f/64,20.0f/64,44.0f/64,32.0f/64); // side
        drawQuadUV(&g_suit,v[1],v[5],v[6],v[2],48.0f/64,20.0f/64,52.0f/64,32.0f/64); // side
        drawQuadUV(&g_suit,v[3],v[2],v[6],v[7],44.0f/64,16.0f/64,48.0f/64,20.0f/64); // top
        drawQuadUV(&g_suit,v[4],v[5],v[1],v[0],48.0f/64,16.0f/64,52.0f/64,20.0f/64); // bottom
    } else {
        drawQuadUV(&g_suit,v[0],v[1],v[2],v[3],36.0f/64,52.0f/64,40.0f/64,64.0f/64); // left arm front
        drawQuadUV(&g_suit,v[5],v[4],v[7],v[6],44.0f/64,52.0f/64,48.0f/64,64.0f/64); // back
        drawQuadUV(&g_suit,v[4],v[0],v[3],v[7],32.0f/64,52.0f/64,36.0f/64,64.0f/64); // side
        drawQuadUV(&g_suit,v[1],v[5],v[6],v[2],40.0f/64,52.0f/64,44.0f/64,64.0f/64); // side
        drawQuadUV(&g_suit,v[3],v[2],v[6],v[7],36.0f/64,48.0f/64,40.0f/64,52.0f/64); // top
        drawQuadUV(&g_suit,v[4],v[5],v[1],v[0],40.0f/64,48.0f/64,44.0f/64,52.0f/64); // bottom
    }
}


/* Full classic Minecraft player model, using the 64x64 skin atlas.
   Coordinates are relative to the player's feet; yaw turns the whole body.
   Each part has its own UV rectangles: front, back, left, right, top, bottom. */
static Vec3 playerLocalToWorld(const Vec3 &base, float yaw, const Vec3 &p) {
    const float c=cosf(yaw), sn=sinf(yaw);
    const float ct=cosf(g_deathTilt), st=sinf(g_deathTilt);
    const float tiltedY=p.y*ct-p.z*st;
    const float tiltedZ=p.y*st+p.z*ct;
    return Vec3{base.x+p.x*c+tiltedZ*sn, base.y+tiltedY, base.z-p.x*sn+tiltedZ*c};
}
static void drawMinecraftCuboid(const Vec3 &base, float yaw, const Vec3 &center,
                                float w, float h, float d,
                                const float uv[6][4], float swing,
                                const Vec3 &pivot) {
    const float x0=center.x-w*.5f, x1=center.x+w*.5f;
    const float y0=center.y-h*.5f, y1=center.y+h*.5f;
    const float z0=center.z-d*.5f, z1=center.z+d*.5f;
    Vec3 v[8]={
        {x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},
        {x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}
    };
    const float c=cosf(swing), sn=sinf(swing);
    for(int i=0;i<8;++i) {
        const float dy=v[i].y-pivot.y, dz=v[i].z-pivot.z;
        const float ry=pivot.y+dy*c-dz*sn;
        const float rz=pivot.z+dy*sn+dz*c;
        v[i]=playerLocalToWorld(base,yaw,Vec3{v[i].x,ry,rz});
    }
    const int face[6][4]={{0,1,2,3},{5,4,7,6},{4,0,3,7},
                          {1,5,6,2},{3,2,6,7},{4,5,1,0}};
    for(int f=0;f<6;++f) {
        drawQuadUV(&g_suit,v[face[f][0]],v[face[f][1]],
                   v[face[f][2]],v[face[f][3]],
                   uv[f][0]/64.0f,uv[f][1]/64.0f,
                   uv[f][2]/64.0f,uv[f][3]/64.0f);
    }
}
static void drawMinecraftPlayerModel(float x, float feetY, float z, float yaw,
                                     float walkPhase) {
    const Vec3 base={x,feetY,z};
    const float noSwing=0.0f;
    const float headUV[6][4]={
        {8,8,16,16},{24,8,32,16},{0,8,8,16},
        {16,8,24,16},{8,0,16,8},{16,0,24,8}
    };
    const float hairUV[6][4]={
        {40,8,48,16},{56,8,64,16},{32,8,40,16},
        {48,8,56,16},{40,0,48,8},{48,0,56,8}
    };
    const float torsoUV[6][4]={
        {20,20,28,32},{32,20,40,32},{16,20,20,32},
        {28,20,32,32},{20,16,28,20},{28,16,36,20}
    };
    const float rightArmUV[6][4]={
        {44,20,48,32},{52,20,56,32},{40,20,44,32},
        {48,20,52,32},{44,16,48,20},{48,16,52,20}
    };
    const float leftArmUV[6][4]={
        {36,52,40,64},{44,52,48,64},{32,52,36,64},
        {40,52,44,64},{36,48,40,52},{40,48,44,52}
    };
    const float rightLegUV[6][4]={
        {4,20,8,32},{12,20,16,32},{0,20,4,32},
        {8,20,12,32},{4,16,8,20},{8,16,12,20}
    };
    const float leftLegUV[6][4]={
        {20,52,24,64},{28,52,32,64},{16,52,20,64},
        {24,52,28,64},{20,48,24,52},{24,48,28,52}
    };
    const float step=g_playerDead ? 0.0f : sinf(walkPhase)*0.42f;
    const float armFlop=g_playerDead ? g_deathLimbFlop : 0.0f;
    const float legFlop=g_playerDead ? -g_deathLimbFlop*0.55f : 0.0f;
    const Vec3 torsoPivot={0.0f,1.12f,0.0f};
    const Vec3 headPivot={0.0f,1.50f,0.0f};
    const Vec3 leftArmPivot={-0.375f,1.42f,0.0f};
    const Vec3 rightArmPivot={0.375f,1.42f,0.0f};
    const Vec3 leftLegPivot={-0.125f,0.75f,0.0f};
    const Vec3 rightLegPivot={0.125f,0.75f,0.0f};

    /* Legs are drawn first, then torso, arms and head to keep the silhouette clear. */
    drawMinecraftCuboid(base,yaw,Vec3{-0.125f,0.375f,0},0.25f,0.75f,0.25f,
                        leftLegUV,step+legFlop,leftLegPivot);
    drawMinecraftCuboid(base,yaw,Vec3{0.125f,0.375f,0},0.25f,0.75f,0.25f,
                        rightLegUV,-step-legFlop,rightLegPivot);
    drawMinecraftCuboid(base,yaw,Vec3{0,1.125f,0},0.50f,0.75f,0.25f,
                        torsoUV,noSwing,torsoPivot);
    drawMinecraftCuboid(base,yaw,Vec3{-0.375f,1.125f,0},0.25f,0.75f,0.25f,
                        leftArmUV,-step-armFlop,leftArmPivot);
    drawMinecraftCuboid(base,yaw,Vec3{0.375f,1.125f,0},0.25f,0.75f,0.25f,
                        rightArmUV,step+armFlop,rightArmPivot);
    drawMinecraftCuboid(base,yaw,Vec3{0,1.50f,0},0.50f,0.50f,0.50f,
                        headUV,noSwing,headPivot);
    /* Hair/hat outer layer, slightly larger than the head. */
    drawMinecraftCuboid(base,yaw,Vec3{0,1.50f,0},0.515f,0.515f,0.515f,
                        hairUV,noSwing,headPivot);
}

static void drawFirstPersonArms() {
    float swing=0.0f;
    float bob=0.0f;
    if(g_moveAmount>0.08f) {
        const float amplitude=g_running?0.34f:0.19f;
        swing=sinf(g_animTime*(g_running?13.0f:8.5f))*amplitude;
        bob=fabsf(sinf(g_animTime*(g_running?13.0f:8.5f)))*(g_running?0.035f:0.018f);
    } else {
        swing=sinf(g_animTime*1.8f)*0.018f;
    }
    if(g_crouching) swing*=0.55f;
    if(!g_grounded) swing=-0.48f;
    drawFirstPersonArm(-1.0f,swing,bob);
    drawFirstPersonArm(1.0f,-swing,bob);
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

static float stickAxis(unsigned char raw, bool invert) {
    float v=((float)raw-128.0f)/127.0f;
    if(invert) v=-v;
    if(v>-0.18f && v<0.18f) return 0.0f;
    if(v>0.0f) return (v-0.18f)/0.82f;
    return (v+0.18f)/0.82f;
}

static void drawHud() {
    if (!g_loadingFont) return;
    const float sw=(float)g_gs->Width;
    const float left=18.0f, top=18.0f, barW=142.0f, barH=10.0f;
    const float lifeY=42.0f, staminaY=76.0f;

    /* Health emblem: a clear green plus, turning red with critical health. */
    const bool critical=(g_health<=25.0f);
    const u64 lifeColor=critical
        ? GS_SETREG_RGBAQ(220,42,35,0x80,0)
        : GS_SETREG_RGBAQ(45,210,82,0x80,0);
    gsKit_prim_sprite(g_gs,left+5,top+7,left+17,top+19,1,lifeColor);
    gsKit_prim_sprite(g_gs,left,top+11,left+22,top+15,1,lifeColor);
    gsKit_prim_sprite(g_gs,left+9,top+2,left+13,top+24,1,lifeColor);
    drawPixelText(left+29,top+1,"VIDA",2.0f,
        GS_SETREG_RGBAQ(240,240,225,0x80,0));

    /* Stamina uses a small lightning-style mark and a yellow bar. */
    gsKit_prim_sprite(g_gs,left+2,staminaY+5,left+20,staminaY+9,1,
        GS_SETREG_RGBAQ(245,202,54,0x80,0));
    gsKit_prim_sprite(g_gs,left+10,staminaY,left+15,staminaY+17,1,
        GS_SETREG_RGBAQ(245,202,54,0x80,0));
    drawPixelText(left+29,staminaY-3,"STAMINA",2.0f,
        GS_SETREG_RGBAQ(240,240,225,0x80,0));

    const float bx=left+29.0f, ex=bx+barW;
    gsKit_prim_sprite(g_gs,bx-1,lifeY-1,ex+1,lifeY+barH+1,2,
        GS_SETREG_RGBAQ(105,100,82,0x80,0));
    gsKit_prim_sprite(g_gs,bx,lifeY,ex,lifeY+barH,3,
        GS_SETREG_RGBAQ(32,31,27,0x80,0));
    float lifeEnd=bx+(barW-2.0f)*(g_health/100.0f);
    if(lifeEnd>bx+1.0f) gsKit_prim_sprite(g_gs,bx+1,lifeY+1,lifeEnd,lifeY+barH-1,4,lifeColor);

    gsKit_prim_sprite(g_gs,bx-1,staminaY+20,ex+1,staminaY+20+barH+2,2,
        GS_SETREG_RGBAQ(105,100,82,0x80,0));
    gsKit_prim_sprite(g_gs,bx,staminaY+21,ex,staminaY+30,3,
        GS_SETREG_RGBAQ(32,31,27,0x80,0));
    float staminaEnd=bx+(barW-2.0f)*(g_stamina/100.0f);
    if(staminaEnd>bx+1.0f) gsKit_prim_sprite(g_gs,bx+1,staminaY+22,staminaEnd,staminaY+29,4,
        GS_SETREG_RGBAQ(238,190,45,0x80,0));

    if(g_playerDead) {
        if(g_deathTimer>=1.15f) {
            drawPixelTextCentered(sw*0.5f,(float)g_gs->Height*0.68f,
                "CLIQUE X PARA RENASCER",2.0f,
                GS_SETREG_RGBAQ(245,235,205,0x80,0));
        }
    } else if(g_waterMessageTimer>0.0f) {
        drawPixelTextCentered(sw*0.5f,112.0f,"AGUA! STAMINA RESTAURADA",2.0f,
            GS_SETREG_RGBAQ(110,220,245,0x80,0));
    } else if(!g_waterCollected) {
        const float dx=g_camera.x-g_waterX, dz=g_camera.z-g_waterZ;
        if(dx*dx+dz*dz < 3.0f*3.0f) {
            drawPixelTextCentered(sw*0.5f,112.0f,"AGUA: RECUPERA TODA A STAMINA",2.0f,
                GS_SETREG_RGBAQ(180,225,235,0x80,0));
        }
    }

    /* DOORS-style inventory: eight empty slots in one straight horizontal row. */
    const float slotSize=30.0f;
    const float slotGap=5.0f;
    const float totalWidth=slotSize*8.0f+slotGap*7.0f;
    const float startX=(sw-totalWidth)*0.5f;
    const float slotY=(float)g_gs->Height-48.0f;
    for(int i=0;i<8;++i) {
        const float x=startX+i*(slotSize+slotGap);
        const bool selected=(i==0);
        const u64 borderColor=selected
            ? GS_SETREG_RGBAQ(225,205,135,0x80,0)
            : GS_SETREG_RGBAQ(105,101,86,0x80,0);
        const u64 fillColor=selected
            ? GS_SETREG_RGBAQ(58,54,42,0x80,0)
            : GS_SETREG_RGBAQ(28,27,24,0x80,0);
        gsKit_prim_sprite(g_gs,x,slotY,x+slotSize,slotY+slotSize,1,borderColor);
        gsKit_prim_sprite(g_gs,x+2.0f,slotY+2.0f,x+slotSize-2.0f,
                          slotY+slotSize-2.0f,2,fillColor);
    }
}

static void respawnPlayer() {
    /* Return to the original safe starting point of Level 0. */
    g_camera.x=0.0f;
    g_camera.y=1.55f;
    g_camera.z=-1.35f;
    g_camera.yaw=0.0f;
    g_cameraPitch=0.0f;

    g_health=100.0f;
    g_stamina=100.0f;
    g_verticalVelocity=0.0f;
    g_jumpHeight=0.0f;
    g_grounded=true;
    g_running=false;
    g_crouching=false;
    g_moveAmount=0.0f;
    g_waterMessageTimer=0.0f;

    g_deathTimer=0.0f;
    g_deathTilt=0.0f;
    g_deathLimbFlop=0.0f;
    g_playerDead=false;
}

static void updateController() {
    static const float dt=1.0f/60.0f;
    if(padGetState(0,0)!=PAD_STATE_STABLE) return;
    padButtonStatus buttons;
    if(padRead(0,0,&buttons)==0) return;
    const unsigned short b=buttons.btns;
    const bool circle=!(b & PAD_CIRCLE);
    const bool jump=!(b & PAD_CROSS);
    const bool l3=!(b & PAD_L3);

    if(circle && !g_previousCircle && !g_playerDead) {
        g_playerDead=true;
        g_health=0.0f;
        g_deathTimer=0.0f;
        g_deathTilt=0.0f;
        g_deathLimbFlop=0.0f;
        g_deathX=g_camera.x;
        g_deathZ=g_camera.z;
        g_deathYaw=g_camera.yaw;
        g_moveAmount=0.0f;
    }
    g_previousCircle=circle;

    if(g_playerDead) {
        /* X only works after the fall animation and only on a fresh press. */
        if(g_deathTimer>=1.15f && jump && !g_previousJump) {
            respawnPlayer();
        }
        g_previousJump=jump;
        return;
    }
    if(l3 && !g_previousL3) g_crouching=!g_crouching;
    if(jump && !g_previousJump && g_grounded) {
        g_verticalVelocity=4.8f;
        g_grounded=false;
    }
    g_previousL3=l3;
    g_previousJump=jump;
    g_running=(!(b & PAD_R1) && !g_crouching && g_stamina>0.05f);

    /* Read the real DualShock 2 analog sticks, with a dead zone to stop drift. */
    float forward=stickAxis(buttons.ljoy_v,true);
    float strafe=stickAxis(buttons.ljoy_h,false);
    const float lookX=stickAxis(buttons.rjoy_h,false);
    const float lookY=stickAxis(buttons.rjoy_v,true);
    g_camera.yaw += lookX*0.055f;
    g_cameraPitch += lookY*0.035f;
    if(g_cameraPitch>0.85f) g_cameraPitch=0.85f;
    if(g_cameraPitch< -0.85f) g_cameraPitch=-0.85f;

    /* Keep the D-pad as a backup, without overriding analog-stick input. */
    if(forward==0.0f) {
        if(!(b & PAD_UP)) forward=1.0f;
        else if(!(b & PAD_DOWN)) forward=-1.0f;
    }
    if(strafe==0.0f) {
        if(!(b & PAD_RIGHT)) strafe=1.0f;
        else if(!(b & PAD_LEFT)) strafe=-1.0f;
    }

    const float inputMagnitude=sqrtf(forward*forward+strafe*strafe);
    if(inputMagnitude>1.0f) { forward/=inputMagnitude; strafe/=inputMagnitude; }
    g_moveAmount=inputMagnitude;
    if(g_running && inputMagnitude>0.08f) {
        const float drain = (g_difficulty == 0) ? 16.0f : ((g_difficulty == 2) ? 29.0f : 22.0f);
        g_stamina-=drain*dt;
        if(g_stamina<=0.0f) { g_stamina=0.0f; g_running=false; }
    } else {
        const float recovery = (g_difficulty == 0) ? 20.0f : ((g_difficulty == 2) ? 12.0f : 16.0f);
        g_stamina+=recovery*dt;
        if(g_stamina>100.0f) g_stamina=100.0f;
    }
    const float difficultySpeed = (g_difficulty == 0) ? 1.08f : ((g_difficulty == 2) ? 0.92f : 1.0f);
    const float speed=(g_crouching?0.035f:(g_running?0.145f:0.085f))*difficultySpeed;
    forward*=speed;
    strafe*=speed;
    const float fx=sinf(g_camera.yaw), fz=cosf(g_camera.yaw);
    const float nx=g_camera.x+fx*forward+cosf(g_camera.yaw)*strafe;
    const float nz=g_camera.z+fz*forward-sinf(g_camera.yaw)*strafe;
    if(canStand(nx,g_camera.z)) g_camera.x=nx;
    if(canStand(g_camera.x,nz)) g_camera.z=nz;

    /* Pick up the bottle automatically when the player reaches it. */
    if(!g_waterCollected) {
        const float dx=g_camera.x-g_waterX, dz=g_camera.z-g_waterZ;
        if(dx*dx+dz*dz < 1.05f*1.05f) {
            g_waterCollected=true;
            g_stamina=100.0f;
            g_waterMessageTimer=4.0f;
        }
    }
    if(g_waterMessageTimer>0.0f) g_waterMessageTimer-=dt;

    /* Real vertical jump arc: velocity, gravity and camera height all change. */
    if(!g_grounded) {
        g_verticalVelocity-=9.8f*dt;
        g_jumpHeight+=g_verticalVelocity*dt;
        if(g_jumpHeight<=0.0f) {
            g_jumpHeight=0.0f;
            g_verticalVelocity=0.0f;
            g_grounded=true;
        }
    }
    const float walkBob=(g_moveAmount>0.08f && g_grounded)
        ? fabsf(sinf(g_animTime*(g_running?13.0f:8.5f)))*(g_running?0.045f:0.022f)
        : 0.0f;
    const float crouchOffset=g_crouching?0.52f:0.0f;
    g_camera.y=1.55f+g_jumpHeight-crouchOffset+walkBob;
}
static void updateDeathAnimation() {
    if(!g_playerDead) return;
    const float dt=1.0f/60.0f;
    g_deathTimer+=dt;
    const float t=(g_deathTimer<1.15f)?g_deathTimer/1.15f:1.0f;
    const float ease=t*t*(3.0f-2.0f*t);
    g_deathTilt=1.48f*ease;
    g_deathLimbFlop=sinf(g_deathTimer*13.0f)*(1.0f-ease)*0.72f;
    /* Third-person camera settles behind the body and looks slightly downward. */
    g_camera.x=g_deathX-sinf(g_deathYaw)*3.2f;
    g_camera.z=g_deathZ-cosf(g_deathYaw)*3.2f;
    g_camera.y=1.55f;
    g_camera.yaw=g_deathYaw;
    g_cameraPitch=0.24f;
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
    if (padPortOpen(0,0,g_padBuffer)) {
        /* Wait briefly for the DualShock 2, then explicitly enable both analog sticks. */
        for (int wait=0; wait<120 && padGetState(0,0)!=PAD_STATE_STABLE; ++wait) { }
        if (padGetState(0,0)==PAD_STATE_STABLE)
            padSetMainMode(0,0,PAD_MMODE_DUALSHOCK,PAD_MMODE_LOCK);
    }
    drawLoadingScreen("FINALIZANDO CONTROLES",96);
    drawLoadingScreen("CARREGAMENTO CONCLUIDO",100);
    return true;
}

int main(int,char **) {
    if (!initGraphicsAndLoadLevel()) {
        for (;;) { }
    }
    runMainMenu();
    for (;;) {
        updateController();
        updateDeathAnimation();
        gsKit_clear(g_gs,GS_SETREG_RGBAQ(18,17,14,0x80,0));
        renderMaze();
        if(g_playerDead) {
            drawMinecraftPlayerModel(g_deathX,0.035f,g_deathZ,g_deathYaw,0.0f);
        } else {
            drawFirstPersonArms();
        }
        drawHud();
        g_animTime+=1.0f/60.0f;
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
    }
    return 0;
}
