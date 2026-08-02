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

    // Gerenciadores de texturas separadas (essencial para poupar a VRAM do PS2)
    Tyra::Texture* textureMap;
    Tyra::Texture* texturePlayer;
    Tyra::Texture* textureMinimap;

    // Sprite 2D do jogador e mapas
    Tyra::Sprite playerSprite;
    Tyra::Sprite mapSprite;
    Tyra::Sprite minimapSprite;

    // Variáveis de Estado
    float playerX = 1024.0f;
    float playerY = 512.0f;
    float camX = 0.0f;
    float camY = 0.0f;
    bool showMiniMap = false;
    bool buttonPressedLastFrame = false;
    
    // Canal de áudio para o arquivo .wav convertida para 44100Hz
    s32 bmgAudioId;
};
