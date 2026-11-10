#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

/*
 * BACKROOMS_ISOLAMENTO_01
 * Uses the same NTSC/framebuffer setup as the game, but never calls any
 * game renderer, 3D projection, texture upload, or menu code.
 * Only flat 2D GS sprites are submitted after the clear.
 */
int main(int, char **) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) for (;;) { }

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

    for (;;) {
        /* No depth test, textures, 3D vertices, or game code. */
        gsKit_set_test(gs, GS_ZTEST_OFF);
        gsKit_clear(gs, GS_SETREG_RGBAQ(18, 18, 18, 0x80, 0));

        gsKit_prim_sprite(gs, 32, 32, 304, 208, 1,
                          GS_SETREG_RGBAQ(220, 35, 35, 0x80, 0));
        gsKit_prim_sprite(gs, 336, 32, 608, 208, 1,
                          GS_SETREG_RGBAQ(35, 190, 65, 0x80, 0));
        gsKit_prim_sprite(gs, 32, 240, 304, 416, 1,
                          GS_SETREG_RGBAQ(35, 95, 220, 0x80, 0));
        gsKit_prim_sprite(gs, 336, 240, 608, 416, 1,
                          GS_SETREG_RGBAQ(235, 205, 35, 0x80, 0));

        /* White center cross confirms exact 2D sprite coordinates. */
        gsKit_prim_sprite(gs, 304, 208, 336, 240, 1,
                          GS_SETREG_RGBAQ(245, 245, 245, 0x80, 0));

        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
