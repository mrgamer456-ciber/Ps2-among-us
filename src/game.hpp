#pragma once

#include <tyra>

class AmongUsGame : public Tyra::Game {
public:
    AmongUsGame(Tyra::Engine* t_engine);
    ~AmongUsGame();

    void init() override;
    void update(const float& deltaTime) override;
    void render() override;

private:
    Tyra::Engine* engine;

    // Gerenciadores de texturas
    Tyra::Texture* textureMap = nullptr;
    Tyra::Texture* texturePlayer = nullptr;
    Tyra::Texture* textureMinimap = nullptr;

    // Sprites
    Tyra::Sprite playerSprite;
    Tyra::Sprite mapSprite;
    Tyra::Sprite minimapSprite;

    // Posição do jogador
    float playerX = 1024.0f;
    float playerY = 512.0f;

    // Câmera
    float camX = 0.0f;
    float camY = 0.0f;

    // Estado
    bool showMiniMap = false;
    bool buttonPressedLastFrame = false;

    // Áudio
    s32 bmgAudioId = -1;
};