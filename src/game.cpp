#include "game.hpp"

namespace {
constexpr float kScreenWidth = 640.0f;
constexpr float kScreenHeight = 448.0f;
constexpr float kPlayerWidth = 32.0f;
constexpr float kPlayerHeight = 48.0f;
constexpr float kPlayerSpeed = 3.0f;
}

AmongUsGame::AmongUsGame(Tyra::Engine* t_engine) : engine(t_engine) {}

void AmongUsGame::init() {
    auto& textureRepo = engine->renderer.getTextureRepository();

    textureMap = textureRepo.add(
        Tyra::FileUtils::fromCwd("assets/images/mapa.png"));
    texturePlayer = textureRepo.add(
        Tyra::FileUtils::fromCwd("assets/images/player.png"));
    textureMinimap = textureRepo.add(
        Tyra::FileUtils::fromCwd("assets/images/minimapa.png"));

    // Na Tyra desta versão, cada textura precisa ser ligada ao ID do sprite.
    textureMap->addLink(mapSprite.id);
    texturePlayer->addLink(playerSprite.id);
    textureMinimap->addLink(minimapSprite.id);

    mapSprite.position.set(0.0f, 0.0f);
    mapSprite.size.set(kScreenWidth, kScreenHeight);
    mapSprite.mode = Tyra::MODE_STRETCH;

    playerSprite.size.set(kPlayerWidth, kPlayerHeight);
    playerSprite.position.set(playerX, playerY);

    minimapSprite.position.set(0.0f, 24.0f);
    minimapSprite.size.set(kScreenWidth, 400.0f);
    minimapSprite.mode = Tyra::MODE_STRETCH;

    engine->audio.song.load(
        Tyra::FileUtils::fromCwd("assets/music/amongus.wav"));
    engine->audio.song.inLoop = true;
    engine->audio.song.play();
}

void AmongUsGame::loop() {
    const Tyra::PadButtons& pressed = engine->pad.getPressed();
    const Tyra::PadButtons& clicked = engine->pad.getClicked();

    if (clicked.Square) {
        showMiniMap = !showMiniMap;
    }

    if (!showMiniMap) {
        if (pressed.DpadLeft)  playerX -= kPlayerSpeed;
        if (pressed.DpadRight) playerX += kPlayerSpeed;
        if (pressed.DpadUp)    playerY -= kPlayerSpeed;
        if (pressed.DpadDown)  playerY += kPlayerSpeed;
    }

    if (playerX < 0.0f) playerX = 0.0f;
    if (playerY < 0.0f) playerY = 0.0f;
    if (playerX > kScreenWidth - kPlayerWidth)
        playerX = kScreenWidth - kPlayerWidth;
    if (playerY > kScreenHeight - kPlayerHeight)
        playerY = kScreenHeight - kPlayerHeight;

    playerSprite.position.set(playerX, playerY);

    engine->renderer.beginFrame();
    engine->renderer.renderer2D.render(mapSprite);
    engine->renderer.renderer2D.render(playerSprite);

    if (showMiniMap) {
        engine->renderer.renderer2D.render(minimapSprite);
    }

    engine->renderer.endFrame();
}
