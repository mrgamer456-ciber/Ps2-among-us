#include "game.hpp"

// Ponto de entrada do Among Us para PS2.
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Inicializa a Engine Tyra com a resolução do PS2.
    Tyra::EngineOptions options;
    options.window.width = 640;
    options.window.height = 448;

    Tyra::Engine engine;
    engine.init(options);

    // Inicia o jogo e entrega o loop principal à Engine.
    AmongUsGame game(&engine);
    engine.run(&game);

    return 0;
}
