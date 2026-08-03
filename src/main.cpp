#include <tyra.hpp>

int main(int argc, char* argv[]) {
    Tyra::EngineOptions options;

    Tyra::Engine engine;
    engine.init(options);

    while (true) {
    }

    return 0;
}
#include "game.hpp"

// Ponto de entrada padrão exigido pelo ecossistema do PS2DEV / Tyra
int main(int argc, char* argv[]) {
    // Inicializa a Engine Tyra e define as configurações de hardware
    Tyra::EngineOptions options;
    options.window.width = 640;
    options.window.height = 448;

    Tyra::Engine engine;
    engine.init(options);

    // Instancia a classe principal do seu Among Us
    AmongUsGame game(&engine);
    
    // Inicia o loop infinito de processamento do PS2
    engine.run(&game);

    return 0;
}
