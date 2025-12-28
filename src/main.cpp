#include "game.h"
#include "gui.h"
#include "logic.h"

void run_game_loop();

int main() {
    std::srand(std::time(nullptr));
    run_game_loop();
}

void run_game_loop() {
    Game game {};
    Assets assets {};
    game.state = Gamestate::Intro;
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    while (window.isOpen()) {
        if ((game.mode == Gamemode::CPUwhite && game.state == Gamestate::Playing && game.turn == white) ||
            (game.mode == Gamemode::CPUblack && game.state == Gamestate::Playing && game.turn == black)) {
            
            if (!finished) {
                generate_computer_move(game);
            } else {
                std::cout << "Searched: " << positions_searched << '\n';
                update_computer_move(game);
            }
        } 
        handle_input(game, window, assets);
        render(game, window, assets);
    }
}

