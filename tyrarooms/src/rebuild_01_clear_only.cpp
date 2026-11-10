#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

/*
 * BACKROOMS REBUILD 01 - CLEAR ONLY
 * Fresh minimal graphics test.
 * No menu, no game code, no textures, no 2D sprites, no 3D, no Z buffer.
 * If this fails visually, the problem is below the game renderer level.
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
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
