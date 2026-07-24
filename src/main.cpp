#include "game.h"
#include "gui.h"
#include "logic.h"
#include "uci.h"

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
        Application application {};
        application.run_game_loop();
    }
}

void Application::run_game_loop() {
 
    init_zobrist_table();
    while (window.isOpen()) {
        if ((game.mode == Gamemode::CPUwhite && (game.state == Gamestate::Playing || game.state == Gamestate::Promoting_pawn_premove)
            && position.turn == WHITE) ||
            (game.mode == Gamemode::CPUblack && (game.state == Gamestate::Playing || game.state == Gamestate::Promoting_pawn_premove)
            && position.turn == BLACK)) {
        
            if (!finished) {
                int time = set_thinking_time(game);
                generate_computer_move(position, time);
            } else {
                std::cout << "Searched: " << positions_searched << '\n';
                make_computer_move();
                game.assess_and_make_premoves();
                game.premove = false;
            }
        } 
        handle_input();
        render();
    }
}


void check_build_mode() {
    #ifdef NDEBUG
        std::cout << "Running in RELEASE mode" << std::endl;
    #else
        std::cout << "Running in DEBUG mode" << std::endl;
    #endif
}

