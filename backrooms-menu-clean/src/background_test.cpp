#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "background.h"

// Diagnostico isolado: somente a textura 512x512, sem fonte, botoes ou overlays.
int main(int, char**) {
    GSGLOBAL *gs = gsKit_init_global();
    if (!gs) for (;;) {}

    gs->Mode = GS_MODE_NTSC;
    gs->Width = 640;
    gs->Height = 448;
    gs->PSM = GS_PSM_CT32;
    gs->PSMZ = GS_PSMZ_16;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_vram_clear(gs);
    gsKit_set_clamp(gs, GS_CMODE_CLAMP);
    gsKit_mode_switch(gs, GS_PERSISTENT);

    GSTEXTURE bg;
    memset(&bg, 0, sizeof(bg));
    bg.Width = 512;
    bg.Height = 512;
    bg.PSM = GS_PSM_CT32;
    bg.Filter = GS_FILTER_LINEAR;
    bg.Mem = (u32*)memalign(128, 512 * 512 * sizeof(uint32_t));
    if (!bg.Mem) for (;;) {}

    memcpy(bg.Mem, cleanMenuPixels, 512 * 512 * sizeof(uint32_t));
    bg.Vram = gsKit_vram_alloc(gs,
        gsKit_texture_size(bg.Width, bg.Height, bg.PSM),
        GSKIT_ALLOC_USERBUFFER);
    if (bg.Vram == GSKIT_ALLOC_ERROR) for (;;) {}

    // gsKit_texture_upload calcula TBW e envia a textura por DMA.
    gsKit_texture_upload(gs, &bg);

    for (;;) {
        gsKit_set_test(gs, GS_ZTEST_OFF);
        gsKit_clear(gs, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
        gsKit_prim_sprite_texture(gs, &bg,
            0, 0, 0, 0,
            640, 448, 512, 512,
            1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0));

        // Mesma sequencia do exemplo alpha.c para GS_PERSISTENT.
        gsKit_sync_flip(gs);
        gsKit_queue_exec(gs);
        gsKit_queue_reset(gs->Per_Queue);
    }
    return 0;
}
