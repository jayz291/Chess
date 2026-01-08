#include "game.h"
#include "gui.h"
#include "logic.h"

void run_game_loop();
void check_build_mode();

int main() {
    std::srand(std::time(nullptr));
    check_build_mode();
    run_game_loop();
}

void run_game_loop() {
    Game game {};
    Assets assets {};
    game.state = Gamestate::Intro;
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    init_zobrist_table();
    window.setFramerateLimit(60);
    game.zobrist_hash = 0;
    find_position_hash(game);

    while (window.isOpen()) {
        if ((game.mode == Gamemode::CPUwhite && game.state == Gamestate::Playing && game.turn == WHITE) ||
            (game.mode == Gamemode::CPUblack && game.state == Gamestate::Playing && game.turn == BLACK)) {
            
            if (!finished) {
                generate_computer_move(game);
            } else {
                std::cout << "Searched: " << positions_searched << '\n';
                make_computer_move(game);
            }
        } 
        handle_input(game, window, assets);
        render(game, window, assets);
    }
}


void check_build_mode() {
    #ifdef NDEBUG
        std::cout << "Running in RELEASE mode" << std::endl;
    #else
        std::cout << "Running in DEBUG mode" << std::endl;
    #endif
}

