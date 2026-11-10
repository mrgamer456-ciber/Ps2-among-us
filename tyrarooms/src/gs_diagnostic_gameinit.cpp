#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <string.h>

static GSTEXTURE testTexture;

int main(int, char **) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) for (;;) { }

    /* Match the current Backrooms framebuffer and Z-buffer formats. */
    gs->Mode = GS_MODE_NTSC;
    gs->Width = 640;
    gs->Height = 448;
    gs->PSM = GS_PSM_CT32;
    gs->PSMZ = GS_PSMZ_16;
    gs->ZBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_vram_clear(gs);
    gsKit_set_clamp(gs, GS_CMODE_CLAMP);
    gsKit_mode_switch(gs, GS_ONESHOT);

    memset(&testTexture, 0, sizeof(testTexture));
    testTexture.Width = 128;
    testTexture.Height = 128;
    testTexture.TBW = 2;
    testTexture.PSM = GS_PSM_CT32;
    testTexture.Filter = GS_FILTER_NEAREST;
    testTexture.Mem = (u32 *)memalign(128, 128 * 128 * sizeof(u32));
    if (!testTexture.Mem) for (;;) { }

    /* High-contrast quadrants and a fine grid expose UV/stride corruption. */
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            u32 color;
            if (x < 64 && y < 64) color = 0x800000FFu;
            else if (x >= 64 && y < 64) color = 0x8000FF00u;
            else if (x < 64 && y >= 64) color = 0x80FF0000u;
            else color = 0x80FFFFFFu;
            if ((x % 16) == 0 || (y % 16) == 0) color = 0x80000000u;
            testTexture.Mem[y * 128 + x] = color;
        }
    }

    testTexture.Vram = gsKit_vram_alloc(gs,
        gsKit_texture_size(testTexture.Width, testTexture.Height,
                           testTexture.PSM), GSKIT_ALLOC_USERBUFFER);
    if (testTexture.Vram == GSKIT_ALLOC_ERROR) for (;;) { }
    gsKit_texture_upload(gs, &testTexture);

    for (;;) {
        gsKit_set_test(gs, GS_ZTEST_OFF);
        gsKit_clear(gs, GS_SETREG_RGBAQ(12, 12, 12, 0x80, 0));
        gsKit_prim_sprite_texture(gs, &testTexture,
            0.0f, 0.0f, 0.0f, 0.0f,
            640.0f, 448.0f, 128.0f, 128.0f,
            1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0));

        /* Four solid overlay panels test untextured 2D sprite drawing. */
        gsKit_prim_sprite(gs, 24, 24, 104, 84, 1,
                          GS_SETREG_RGBAQ(255, 255, 255, 0x80, 0));
        gsKit_prim_sprite(gs, 536, 24, 616, 84, 1,
                          GS_SETREG_RGBAQ(255, 220, 32, 0x80, 0));
        gsKit_prim_sprite(gs, 24, 364, 104, 424, 1,
                          GS_SETREG_RGBAQ(32, 220, 255, 0x80, 0));
        gsKit_prim_sprite(gs, 536, 364, 616, 424, 1,
                          GS_SETREG_RGBAQ(255, 48, 48, 0x80, 0));

        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
