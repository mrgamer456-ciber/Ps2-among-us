#include "game.hpp"

AmongUsGame::AmongUsGame(Tyra::Engine* t_engine) {
    engine = t_engine;
}

AmongUsGame::~AmongUsGame() {
    // Libera os arquivos da VRAM ao fechar para evitar travamentos
    engine->renderer.getTextureRepository().free(textureMap);
    engine->renderer.getTextureRepository().free(texturePlayer);
    engine->renderer.getTextureRepository().free(textureMinimap);
}

void AmongUsGame::init() {
    // 1. Carrega as texturas divididas no repositório do PS2
    auto& textureRepo = engine->renderer.getTextureRepository();
    
    // Tyra exige caminhos relativos partindo da raiz do CD/USB
    textureMap = textureRepo.add(Tyra::FileUtils::fromOwnedFolder("assets/mapa.png"));
    texturePlayer = textureRepo.add(Tyra::FileUtils::fromOwnedFolder("assets/player.png"));
    textureMinimap = textureRepo.add(Tyra::FileUtils::fromOwnedFolder("assets/minimapa.png"));

    // 2. Configura as dimensões das Sprites 2D
    mapSprite.mode = Tyra::SpriteMode::MODE_STRETCH;
    mapSprite.size.set(640.0f, 448.0f); // Renderiza do tamanho da tela

    playerSprite.size.set(32.0f, 48.0f); // Tamanho físico do boneco

    minimapSprite.size.set(640.0f, 400.0f);
    minimapSprite.position.set(0.0f, 24.0f); // Centraliza verticalmente

    // 3. Sistema de som nativo da Tyra (Lê o WAV de 44100Hz de forma estável)
    engine->audio.loadWav(Tyra::FileUtils::fromOwnedFolder("assets/amongus.ogg"));
    bmgAudioId = engine->audio.playWav(0); // Toca no canal zero
    engine->audio.setLoopWav(bmgAudioId, true);
}

void AmongUsGame::update(const float& deltaTime) {
    // Captura o estado dos botões físicos do controle 1 do PS2 via IOP
    auto& pad = engine->pad.getPad(0);

    // Sistema de verificação do botão Quadrado (□)
    if (pad.isSquarePressed() && !buttonPressedLastFrame) {
        showMiniMap = !showMiniMap; // Inverte o estado
    }
    buttonPressedLastFrame = pad.isSquarePressed();

    // Se o mapa estiver aberto, o boneco não se move
    if (showMiniMap) return;

    // Movimentação do Jogador multiplicada pelo deltaTime para ficar suave
    float speed = 150.0f * deltaTime;

    if (pad.isDpadLeftPressed())  playerX -= speed;
    if (pad.isDpadRightPressed()) playerX += speed;
    if (pad.isDpadUpPressed())    playerY -= speed;
    if (pad.isDpadDownPressed())  playerY += speed;

    // Atualiza a posição da câmera baseada no jogador
    camX = playerX - 320.0f;
    camY = playerY - 224.0f;

    // Travas de segurança para a câmera não sair do mapa de 2048x1024
    if (camX < 0.0f) camX = 0.0f;
    if (camX > 2048.0f - 640.0f) camX = 2048.0f - 640.0f;
    if (camY < 0.0f) camY = 0.0f;
    if (camY > 1024.0f - 448.0f) camY = 1024.0f - 448.0f;
}

void AmongUsGame::render() {
    // Inicia a renderização em 2D na tela
    engine->renderer.beginFrame();
    auto& renderer2D = engine->renderer.renderer2D;

    // 1. Desenha a parte visível do mapa grande (Recorte da textura usando a câmera)
    // No Tyra definimos o offset de visualização diretamente nas propriedades da Sprite
    mapSprite.offset.set(camX, camY);
    renderer2D.render(&mapSprite, textureMap);

    // 2. Desenha o jogador na posição correta da tela
    float screenX = playerX - camX;
    float screenY = playerY - camY;
    playerSprite.position.set(screenX, screenY);
    
    // Para animação por frames no Tyra, você mudaria o offset da playerSprite aqui
    renderer2D.render(&playerSprite, texturePlayer);

    // 3. Desenha a sobreposição do Mini-Mapa se ativado por Quadrado
    if (showMiniMap) {
        renderer2D.render(&minimapSprite, textureMinimap);
    }

    // Finaliza o frame e joga para a TV do PS2
    engine->renderer.endFrame();
}
