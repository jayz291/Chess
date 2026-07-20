#include "game.h"
#include "gui.h"
#include "logic.h"
#include "uci.h"

/**
 * @brief starts the chess game with the user interface
 */
void run_game_loop();

/**
 * @brief checks whether the game is compiled in build or release mode
 */
void check_build_mode();

int main(int argc, char* argv[]) {
    std::srand(std::time(nullptr));
    check_build_mode();
    if (argc > 1 && (std::strcmp("uci", argv[1]) == 0)) {
        run_uci_loop();
    } else {
        run_game_loop();
    }
}

void run_game_loop() {
    Game game {};
    Assets assets {};
    game.state = Gamestate::Intro;
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(sf::VideoMode({1100, 800}), "Chess", sf::Style::Default);
    init_zobrist_table();
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        if ((game.mode == Gamemode::CPUwhite && (game.state == Gamestate::Playing || game.state == Gamestate::Promoting_pawn_premove)
            && game.position.turn == WHITE) ||
            (game.mode == Gamemode::CPUblack && (game.state == Gamestate::Playing || game.state == Gamestate::Promoting_pawn_premove)
            && game.position.turn == BLACK)) {
            
            if (!finished) {
                generate_computer_move(game.position);
            } else {
                std::cout << "Searched: " << positions_searched << '\n';
                make_computer_move(game, assets);
                game.assess_and_make_premove_moves();
                game.premove = false;
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

