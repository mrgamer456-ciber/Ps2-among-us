#include "game.hpp"

// Entrada compatível com a API da Tyra usada neste projeto.
int main() {
    Tyra::Engine engine;
    AmongUsGame game(&engine);
    engine.run(&game);
    return 0;
}
