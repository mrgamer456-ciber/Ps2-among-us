#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

/*
 * BACKROOMS REBUILD 02 - ONE RECTANGLE
 * Same clean GS setup as stage 01.
 * Adds exactly one untextured 2D rectangle to isolate primitive rendering.
 */
int main(int, char **) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) {
        for (;;) { }
    }

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
        gsKit_clear(gs, GS_SETREG_RGBAQ(24, 32, 48, 0x80, 0));
        gsKit_prim_sprite(gs,
                          160.0f, 112.0f,
                          480.0f, 336.0f,
                          1,
                          GS_SETREG_RGBAQ(220, 40, 40, 0x80, 0));
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
