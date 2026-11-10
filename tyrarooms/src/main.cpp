#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

static const int TEX_SIZE = 64;
static const float ROOM_HALF_WIDTH = 4.0f;
static const float ROOM_HEIGHT = 3.2f;
static const float ROOM_LENGTH = 12.0f;

struct Vec3 {
    float x, y, z;
};

struct Camera {
    float x, y, z;
    float yaw;
};

static GSGLOBAL *g_gs = 0;
static GSTEXTURE g_wall;
static GSTEXTURE g_floor;
static GSTEXTURE g_ceiling;
static Camera g_camera = { 0.0f, 1.62f, -2.0f, 0.0f };

static uint32_t packColor(unsigned char r, unsigned char g, unsigned char b) {
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0x80000000u;
}

static unsigned int noise(unsigned int x, unsigned int y, unsigned int seed) {
    unsigned int n = x * 374761393u + y * 668265263u + seed * 1442695041u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return (n ^ (n >> 16)) & 31u;
}

static void makeTexture(GSTEXTURE *tex, int kind) {
    memset(tex, 0, sizeof(GSTEXTURE));
    tex->Width = TEX_SIZE;
    tex->Height = TEX_SIZE;
    tex->TBW = 1;
    tex->PSM = GS_PSM_CT32;
    tex->Filter = GS_FILTER_LINEAR;
    tex->Mem = (u32 *)memalign(128, TEX_SIZE * TEX_SIZE * sizeof(u32));

    for (unsigned int y = 0; y < TEX_SIZE; ++y) {
        for (unsigned int x = 0; x < TEX_SIZE; ++x) {
            unsigned int n = noise(x, y, (unsigned int)(kind + 7));
            unsigned char r = 0, g = 0, b = 0;

            if (kind == 0) { // papel de parede amarelo, com pequenas variações
                int seam = (x % 32 == 0) ? 7 : 0;
                r = (unsigned char)(190 + n / 3 - seam);
                g = (unsigned char)(174 + n / 3 - seam);
                b = (unsigned char)(91 + n / 4 - seam);
                if ((y % 32) == 0) { r -= 3; g -= 3; }
            } else if (kind == 1) { // carpete gasto e úmido
                int fleck = (n > 24) ? 9 : 0;
                r = (unsigned char)(65 + n / 2 + fleck);
                g = (unsigned char)(61 + n / 2 + fleck);
                b = (unsigned char)(43 + n / 3);
                if (((x * 3 + y * 5) % 29) == 0) { r -= 8; g -= 8; b -= 5; }
            } else { // teto claro com painéis fluorescentes
                bool lamp = ((x > 7 && x < 25) || (x > 39 && x < 57)) &&
                            (y > 26 && y < 38);
                if (lamp) {
                    r = (unsigned char)(238 + n / 5);
                    g = (unsigned char)(239 + n / 5);
                    b = (unsigned char)(219 + n / 5);
                } else {
                    r = (unsigned char)(173 + n / 3);
                    g = (unsigned char)(169 + n / 3);
                    b = (unsigned char)(145 + n / 3);
                }
            }
            tex->Mem[y * TEX_SIZE + x] = packColor(r, g, b);
        }
    }

    tex->Vram = gsKit_vram_alloc(g_gs,
        gsKit_texture_size(tex->Width, tex->Height, tex->PSM),
        GSKIT_ALLOC_USERBUFFER);
    gsKit_texture_upload(g_gs, tex);
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
    sy = (float)g_gs->Height * 0.5f -
         ((p.y - g_camera.y) * focal / depth);
    sz = (int)(depth * 1800.0f);
    if (sz < 1) sz = 1;
    return true;
}

static void drawQuad(const GSTEXTURE *tex,
                     const Vec3 &a, const Vec3 &b,
                     const Vec3 &c, const Vec3 &d,
                     float tileU, float tileV) {
    float x1, y1, x2, y2, x3, y3, x4, y4;
    int z1, z2, z3, z4;

    // A sala inicial fica inteiramente à frente da câmera; se uma face
    // cruzar o plano da câmera, ela será ignorada nesta primeira versão.
    if (!projectVertex(a, x1, y1, z1) ||
        !projectVertex(b, x2, y2, z2) ||
        !projectVertex(c, x3, y3, z3) ||
        !projectVertex(d, x4, y4, z4)) return;

    gsKit_prim_quad_texture_3d(g_gs, (GSTEXTURE *)tex,
        x1, y1, z1, 0.0f, 0.0f,
        x2, y2, z2, tileU, 0.0f,
        x3, y3, z3, tileU, tileV,
        x4, y4, z4, 0.0f, tileV,
        GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0));
}

static void renderRoom() {
    const float w = ROOM_HALF_WIDTH;
    const float h = ROOM_HEIGHT;
    const float l = ROOM_LENGTH;

    // Piso: um plano 3D horizontal real.
    drawQuad(&g_floor,
        Vec3{-w, 0.0f, 0.0f}, Vec3{ w, 0.0f, 0.0f},
        Vec3{ w, 0.0f, l},   Vec3{-w, 0.0f, l},
        8.0f, 32.0f);

    // Parede esquerda e direita: planos verticais reais.
    drawQuad(&g_wall,
        Vec3{-w, 0.0f, 0.0f}, Vec3{-w, h, 0.0f},
        Vec3{-w, h, l},       Vec3{-w, 0.0f, l},
        8.0f, 4.0f);
    drawQuad(&g_wall,
        Vec3{ w, 0.0f, l}, Vec3{ w, h, l},
        Vec3{ w, h, 0.0f}, Vec3{ w, 0.0f, 0.0f},
        8.0f, 4.0f);

    // Parede do fundo.
    drawQuad(&g_wall,
        Vec3{-w, 0.0f, l}, Vec3{ w, 0.0f, l},
        Vec3{ w, h, l},    Vec3{-w, h, l},
        8.0f, 4.0f);

    // Teto: outro plano 3D, com textura de placas e fluorescentes.
    drawQuad(&g_ceiling,
        Vec3{-w, h, l}, Vec3{ w, h, l},
        Vec3{ w, h, 0.0f}, Vec3{-w, h, 0.0f},
        8.0f, 32.0f);
}

static void initGraphics() {
    g_gs = gsKit_init_global();
    g_gs->PSM = GS_PSM_CT24;
    g_gs->PSMZ = GS_PSMZ_24;
    g_gs->ZBuffering = GS_SETTING_ON;
    g_gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_vram_clear(g_gs);
    gsKit_init_screen(g_gs);
    gsKit_set_test(g_gs, GS_ZTEST_ON);
    gsKit_set_clamp(g_gs, GS_CMODE_REPEAT);
    gsKit_mode_switch(g_gs, GS_ONESHOT);

    makeTexture(&g_wall, 0);
    makeTexture(&g_floor, 1);
    makeTexture(&g_ceiling, 2);
}

int main(int, char **) {
    initGraphics();

    for (;;) {
        gsKit_clear(g_gs, GS_SETREG_RGBAQ(18, 17, 14, 0x80, 0));
        renderRoom();
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
    }
    return 0;
}
