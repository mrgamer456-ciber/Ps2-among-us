#include <gsKit.h>
#include <dmaKit.h>

// Primeiro teste gráfico: uma tela azul com um quadrado vermelho.
// Este programa usa gsKit diretamente, sem engine 3D.

int main()
{
    // 1. Cria a estrutura principal do gsKit.
    GSGLOBAL *gsGlobal = gsKit_init_global();

    // 2. Inicializa o canal DMA usado pelo Graphics Synthesizer.
    dmaKit_init(
        D_CTRL_RELE_OFF,
        D_CTRL_MFD_OFF,
        D_CTRL_STS_UNSPEC,
        D_CTRL_STD_OFF,
        D_CTRL_RCYC_8,
        1 << DMA_CHANNEL_GIF
    );
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    // 3. Inicializa a tela e seleciona o modo de desenho.
    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);

    // Cores no formato RGBAQ do gsKit.
    const u64 azul = GS_SETREG_RGBAQ(20, 40, 100, 0x00, 0);
    const u64 vermelho = GS_SETREG_RGBAQ(255, 40, 30, 0x00, 0);

    // 4. Loop principal: desenha um quadrado vermelho sobre fundo azul.
    while (1)
    {
        gsKit_clear(gsGlobal, azul);

        gsKit_prim_sprite(
            gsGlobal,
            240.0f, 160.0f,
            400.0f, 320.0f,
            1,
            vermelho
        );

        // Envia os comandos gráficos e troca o buffer exibido.
        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
    }

    return 0;
}
