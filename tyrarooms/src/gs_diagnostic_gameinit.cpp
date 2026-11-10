#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

int main(int, char **) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) {
        for (;;) { }
    }

    /* Match the Backrooms graphics configuration exactly. */
    gs->Mode = GS_MODE_NTSC;
    gs->Width = 640;
    gs->Height = 448;
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_24;
    gs->ZBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_vram_clear(gs);
    gsKit_set_test(gs, GS_ZTEST_ON);
    gsKit_set_clamp(gs, GS_CMODE_REPEAT);
    gsKit_mode_switch(gs, GS_ONESHOT);

    /* If this solid blue screen is clean, the fault is later in game startup
       (font upload, loading-screen drawing, or texture creation). */
    for (;;) {
        gsKit_clear(gs, GS_SETREG_RGBAQ(32, 96, 220, 0x80, 0));
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
