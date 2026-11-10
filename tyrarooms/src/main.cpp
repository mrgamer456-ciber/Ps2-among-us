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
static GSTEXTURE g_suit;
static Camera g_camera = { 0.0f, 1.55f, -2.0f, 0.0f };
static float g_animTime = 0.0f;

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
            } else if (kind == 2) { // teto claro com painéis fluorescentes
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
            } else { // tecido amarelo da roupa, diferente do papel de parede
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



static void drawSolidQuad(const Vec3 &a, const Vec3 &b,
                          const Vec3 &c, const Vec3 &d,
                          u64 color) {
    float x1, y1, x2, y2, x3, y3, x4, y4;
    int z1, z2, z3, z4;
    if (!projectVertex(a, x1, y1, z1) ||
        !projectVertex(b, x2, y2, z2) ||
        !projectVertex(c, x3, y3, z3) ||
        !projectVertex(d, x4, y4, z4)) return;

    gsKit_prim_quad_3d(g_gs,
        x1, y1, z1, x2, y2, z2,
        x3, y3, z3, x4, y4, z4, color);
}

static Vec3 rotateAroundX(const Vec3 &p, const Vec3 &pivot, float angle) {
    const float dy = p.y - pivot.y;
    const float dz = p.z - pivot.z;
    const float c = cosf(angle);
    const float sn = sinf(angle);
    Vec3 out;
    out.x = p.x;
    out.y = pivot.y + dy * c - dz * sn;
    out.z = pivot.z + dy * sn + dz * c;
    return out;
}

static void drawBoxPart(const Vec3 &pivot, const Vec3 &centerOffset,
                        float width, float height, float depth,
                        float swing, const GSTEXTURE *texture) {
    const Vec3 center = {
        pivot.x + centerOffset.x,
        pivot.y + centerOffset.y,
        pivot.z + centerOffset.z
    };
    const float x0 = -width * 0.5f, x1 = width * 0.5f;
    const float y0 = -height * 0.5f, y1 = height * 0.5f;
    const float z0 = -depth * 0.5f, z1 = depth * 0.5f;

    Vec3 v[8] = {
        {center.x+x0, center.y+y0, center.z+z0},
        {center.x+x1, center.y+y0, center.z+z0},
        {center.x+x1, center.y+y1, center.z+z0},
        {center.x+x0, center.y+y1, center.z+z0},
        {center.x+x0, center.y+y0, center.z+z1},
        {center.x+x1, center.y+y0, center.z+z1},
        {center.x+x1, center.y+y1, center.z+z1},
        {center.x+x0, center.y+y1, center.z+z1}
    };

    for (int i = 0; i < 8; ++i)
        v[i] = rotateAroundX(v[i], pivot, swing);

    const float repeatU = 1.0f;
    const float repeatV = 1.0f;
    drawQuad(texture, v[0], v[1], v[2], v[3], repeatU, repeatV); // frente
    drawQuad(texture, v[5], v[4], v[7], v[6], repeatU, repeatV); // costas
    drawQuad(texture, v[4], v[0], v[3], v[7], repeatU, repeatV); // esquerda
    drawQuad(texture, v[1], v[5], v[6], v[2], repeatU, repeatV); // direita
    drawQuad(texture, v[3], v[2], v[6], v[7], repeatU, repeatV); // topo
    drawQuad(texture, v[4], v[5], v[1], v[0], repeatU, repeatV); // base
}

static void drawYellowMaskedCharacter() {
    // O boneco é montado com blocos 3D independentes, estilo Minecraft.
    // A animação usa balanço alternado de braços/pernas e oscilação do tronco.
    const float walk = sinf(g_animTime * 6.0f);
    const float opposite = sinf(g_animTime * 6.0f + 3.14159265f);
    const float bodyBob = fabsf(sinf(g_animTime * 6.0f)) * 0.045f;
    const float torsoY = 1.48f + bodyBob;
    const float charZ = 4.7f;

    // Pernas articuladas no quadril.
    const Vec3 leftHip = {-0.19f, 0.91f + bodyBob, charZ};
    const Vec3 rightHip = {0.19f, 0.91f + bodyBob, charZ};
    drawBoxPart(leftHip, Vec3{0.0f, -0.39f, 0.0f},
                0.34f, 0.82f, 0.38f, walk * 0.48f, &g_suit);
    drawBoxPart(rightHip, Vec3{0.0f, -0.39f, 0.0f},
                0.34f, 0.82f, 0.38f, opposite * 0.48f, &g_suit);

    // Tronco quadrado com roupa amarela.
    drawBoxPart(Vec3{0.0f, torsoY, charZ}, Vec3{0.0f, 0.0f, 0.0f},
                0.86f, 0.98f, 0.48f, 0.0f, &g_suit);

    // Braços pendurados nos ombros, em oposição às pernas.
    const Vec3 leftShoulder = {-0.53f, 1.86f + bodyBob, charZ};
    const Vec3 rightShoulder = {0.53f, 1.86f + bodyBob, charZ};
    drawBoxPart(leftShoulder, Vec3{0.0f, -0.37f, 0.0f},
                0.30f, 0.78f, 0.36f, opposite * 0.58f, &g_suit);
    drawBoxPart(rightShoulder, Vec3{0.0f, -0.37f, 0.0f},
                0.30f, 0.78f, 0.36f, walk * 0.58f, &g_suit);

    // Cabeça quadrada e máscara escura no lado voltado para a câmera (-Z).
    const Vec3 head = {0.0f, 2.48f + bodyBob, charZ};
    drawBoxPart(head, Vec3{0.0f, 0.0f, 0.0f},
                0.66f, 0.66f, 0.62f, sinf(g_animTime * 3.0f) * 0.035f, &g_suit);

    const float faceZ = charZ - 0.315f;
    const float faceY = head.y - 0.04f;
    drawSolidQuad(
        Vec3{-0.23f, faceY-0.14f, faceZ},
        Vec3{ 0.23f, faceY-0.14f, faceZ},
        Vec3{ 0.23f, faceY+0.13f, faceZ},
        Vec3{-0.23f, faceY+0.13f, faceZ},
        GS_SETREG_RGBAQ(38, 43, 42, 0x80, 0));
    // Duas lentes/aberturas da máscara.
    drawSolidQuad(
        Vec3{-0.17f, faceY+0.015f, faceZ-0.006f},
        Vec3{-0.035f, faceY+0.015f, faceZ-0.006f},
        Vec3{-0.035f, faceY+0.085f, faceZ-0.006f},
        Vec3{-0.17f, faceY+0.085f, faceZ-0.006f},
        GS_SETREG_RGBAQ(145, 160, 145, 0x80, 0));
    drawSolidQuad(
        Vec3{0.035f, faceY+0.015f, faceZ-0.006f},
        Vec3{0.17f, faceY+0.015f, faceZ-0.006f},
        Vec3{0.17f, faceY+0.085f, faceZ-0.006f},
        Vec3{0.035f, faceY+0.085f, faceZ-0.006f},
        GS_SETREG_RGBAQ(145, 160, 145, 0x80, 0));
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
    makeTexture(&g_ceiling, 2);\n    makeTexture(&g_suit, 3);
}

int main(int, char **) {
    initGraphics();

    for (;;) {
        gsKit_clear(g_gs, GS_SETREG_RGBAQ(18, 17, 14, 0x80, 0));
        renderRoom();
        drawYellowMaskedCharacter();
        g_animTime += 1.0f / 60.0f;
        gsKit_queue_exec(g_gs);
        gsKit_sync_flip(g_gs);
    }
    return 0;
}
