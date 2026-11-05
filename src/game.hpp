#pragma once

#include <tyra>

class AmongUsGame : public Tyra::Game {
public:
    explicit AmongUsGame(Tyra::Engine* t_engine);

    void init() override;
    void loop() override;

private:
    Tyra::Engine* engine;

    Tyra::Texture* textureMap = nullptr;
    Tyra::Texture* texturePlayer = nullptr;
    Tyra::Texture* textureMinimap = nullptr;

    Tyra::Sprite playerSprite;
    Tyra::Sprite mapSprite;
    Tyra::Sprite minimapSprite;

    float playerX = 304.0f;
    float playerY = 200.0f;
    bool showMiniMap = false;
};
