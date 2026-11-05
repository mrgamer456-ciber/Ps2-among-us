#include "game.hpp"

namespace {
constexpr float kMapWidth = 2048.0f;
constexpr float kMapHeight = 1024.0f;
constexpr float kScreenWidth = 640.0f;
constexpr float kScreenHeight = 448.0f;
constexpr float kPlayerWidth = 32.0f;
constexpr float kPlayerHeight = 48.0f;
constexpr float kPlayerSpeed = 150.0f;
}  // namespace

AmongUsGame::AmongUsGame(Tyra::Engine* t_engine) {
    engine = t_engine;
}

AmongUsGame::~AmongUsGame() {
    // Libera as texturas carregadas antes de encerrar o jogo.
    auto& textureRepo = engine->renderer.getTextureRepository();
    if (textureMap) textureRepo.free(textureMap);
    if (texturePlayer) textureRepo.free(texturePlayer);
    if (textureMinimap) textureRepo.free(textureMinimap);
}

void AmongUsGame::init() {
    auto& textureRepo = engine->renderer.getTextureRepository();

    // Os caminhos são relativos à raiz dos arquivos do jogo.
    textureMap = textureRepo.add(
        Tyra::FileUtils::fromOwnedFolder("assets/images/mapa.png"));
    texturePlayer = textureRepo.add(
        Tyra::FileUtils::fromOwnedFolder("assets/images/player.png"));
    textureMinimap = textureRepo.add(
        Tyra::FileUtils::fromOwnedFolder("assets/images/minimapa.png"));

    mapSprite.mode = Tyra::SpriteMode::MODE_STRETCH;
    mapSprite.size.set(kScreenWidth, kScreenHeight);

    playerSprite.size.set(kPlayerWidth, kPlayerHeight);

    minimapSprite.size.set(kScreenWidth, 400.0f);
    minimapSprite.position.set(0.0f, 24.0f);

    engine->audio.loadWav(
        Tyra::FileUtils::fromOwnedFolder("assets/music/amongus.wav"));
    bmgAudioId = engine->audio.playWav(0);
    engine->audio.setLoopWav(bmgAudioId, true);

    // Centraliza a câmera no jogador já no primeiro quadro.
    camX = playerX - (kScreenWidth / 2.0f);
    camY = playerY - (kScreenHeight / 2.0f);
}

void AmongUsGame::update(const float& deltaTime) {
    auto& pad = engine->pad.getPad(0);

    const bool squarePressed = pad.isSquarePressed();
    if (squarePressed && !buttonPressedLastFrame) {
        showMiniMap = !showMiniMap;
    }
    buttonPressedLastFrame = squarePressed;

    if (showMiniMap) return;

    const float speed = kPlayerSpeed * deltaTime;

    if (pad.isDpadLeftPressed())  playerX -= speed;
    if (pad.isDpadRightPressed()) playerX += speed;
    if (pad.isDpadUpPressed())    playerY -= speed;
    if (pad.isDpadDownPressed())  playerY += speed;

    // Mantém o personagem dentro dos limites do mapa.
    if (playerX < 0.0f) playerX = 0.0f;
    if (playerY < 0.0f) playerY = 0.0f;
    if (playerX > kMapWidth - kPlayerWidth)
        playerX = kMapWidth - kPlayerWidth;
    if (playerY > kMapHeight - kPlayerHeight)
        playerY = kMapHeight - kPlayerHeight;

    // A câmera acompanha o jogador sem ultrapassar as bordas do cenário.
    camX = playerX - (kScreenWidth / 2.0f);
    camY = playerY - (kScreenHeight / 2.0f);

    if (camX < 0.0f) camX = 0.0f;
    if (camY < 0.0f) camY = 0.0f;
    if (camX > kMapWidth - kScreenWidth)
        camX = kMapWidth - kScreenWidth;
    if (camY > kMapHeight - kScreenHeight)
        camY = kMapHeight - kScreenHeight;
}

void AmongUsGame::render() {
    engine->renderer.beginFrame();
    auto& renderer2D = engine->renderer.renderer2D;

    mapSprite.offset.set(camX, camY);
    renderer2D.render(&mapSprite, textureMap);

    const float screenX = playerX - camX;
    const float screenY = playerY - camY;
    playerSprite.position.set(screenX, screenY);
    renderer2D.render(&playerSprite, texturePlayer);

    if (showMiniMap) {
        renderer2D.render(&minimapSprite, textureMinimap);
    }

    engine->renderer.endFrame();
}
