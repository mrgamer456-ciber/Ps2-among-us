#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

int main(int, char **) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) {
        for (;;) { }
    }

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    /* Deliberately use gsKit's detected video mode, without game assets,
       textures, 3D rendering, menu code, or custom framebuffer settings. */
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);

    for (;;) {
        gsKit_clear(gs, GS_SETREG_RGBAQ(32, 96, 220, 0x80, 0));
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }
    return 0;
}
