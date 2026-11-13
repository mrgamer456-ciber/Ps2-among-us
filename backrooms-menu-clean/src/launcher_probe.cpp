#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

int main(int, char**) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) {
        for (;;) {}
    }

    // Minimal video-only probe: no textures, font, controller, or background assets.
    gs->Mode = GS_MODE_NTSC;
    gs->Width = 640;
    gs->Height = 448;
    gs->PSM = GS_PSM_CT32;
    gs->PSMZ = GS_PSMZ_16;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(
        D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
        D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF
    );
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_vram_clear(gs);
    gsKit_mode_switch(gs, GS_PERSISTENT);

    for (;;) {
        // A flat blue screen makes leftover boot graphics easy to identify.
        gsKit_clear(gs, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
        gsKit_prim_sprite(
            gs, 0, 0, 640, 448, 1,
            GS_SETREG_RGBAQ(24, 72, 180, 0x80, 0)
        );
        gsKit_sync_flip(gs);
        gsKit_queue_exec(gs);
        gsKit_queue_reset(gs->Per_Queue);
    }

    return 0;
}
