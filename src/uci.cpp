#include "game.h"
#include "logic.h"
#include "perft.h"
#include "engine.h"
#include "uci.h"

void parse_position(Game& game, std::istringstream& stream);
void parse_go(Game& game, std::istringstream& stream);

void run_uci_loop() {
    Game game {};
    init_zobrist_table();
    std::string line, command;

    std::setbuf(stdout, NULL);
    std::setbuf(stdin, NULL);

    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        stream >> command;

        if (command == "uci") {
            std::cout << "id name chess_engine" << std::endl;
            std::cout << "id author __ " << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl; 
        } else if (command == "ucinewgame") {
            game.initialise();
            //game.final_fen = fen_string;
            game.handle_fen_string();
        } else if (command == "position") {
            parse_position(game, stream);
        } else if (command == "go") {
            parse_go(game, stream);
        } else if (command == "stop") {
            terminate_search = true;
        } else if (command == "quit") {
            break;
        } else if (command == "perft") {
            int depth;
            if (stream >> depth) {
                run_perft_suite(game.position, depth);
            } else {
                std::cout << "no depth specified" << std::endl;
            }
        }
    }
}

void parse_position(Game& game, std::istringstream& stream) {
    std::string token, fen;
    stream >> token;
    if (token == "startpos") {
        game.initialise();
        game.entered_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        game.handle_fen_string();
        stream >> token;
    } else if (token == "fen") {
        std::string fen_part;
        while (stream >> fen_part && fen_part != "moves") {
            fen += fen_part + " ";
        }
        game.entered_fen = fen;
        if (game.handle_fen_string() != 0) {
            std::cout << "invalid fen string" << std::endl;
        };
        if (fen_part == "moves") {
            token = "moves";
        }
    }
    if (token == "moves") {
        std::string move_string;
        while (stream >> move_string) {
            Move move = parse_move_string(game, move_string);
            int result = game.position.validate_move(move);
            if (result >= 0) {
                game.make_game_move(result, move);
                if (game.ui.promoting_pawn) {
                    game.handle_pawn_promotion(move, true);
                }
            } else {
                std::cout << "invalid move inputted" << std::endl;
            }
        }
    }
}

void parse_go(Game& game, std::istringstream& stream) {
    std::string token;
    int wtime = -1, btime = -1, winc = 0, binc = 0;
    int movetime = -1, depth = -1;
    while (stream >> token) {
        if (token == "wtime") {
            stream >> wtime;
        } else if (token == "btime") {
            stream >> btime;
        } else if (token == "winc") {
            stream >> winc;
        } else if (token == "binc") {
            stream >> binc;
        } else if (token == "movetime") {
            stream >> movetime;
        } else if (token == "depth") {
            stream >> depth;
        }
    }
    int allocated_time = std::numeric_limits<int>::max();
    if (movetime != -1) {
        allocated_time = movetime;
    } else if (wtime != -1 && btime != -1) {
        int time_left = (game.position.turn == WHITE) ? wtime : btime;
        int inc = (game.position.turn == WHITE) ? winc : binc;
        allocated_time = (time_left / 20) + inc;
    }

    int search_depth = (depth != -1) ? depth : 40;
   
    std::thread search_thread([game, allocated_time, search_depth]() mutable {
        Engine engine(game.position, allocated_time);
        Move best_move = engine.get_best_move(search_depth);
        std::cout << "bestmove " << to_chess_notation(best_move) << std::endl;
    });

    search_thread.detach();
}

std::string format_score(int score) {
    if (score > CHECKMATE_THRESHOLD || score < -CHECKMATE_THRESHOLD) {
        int plies_to_mate = 400000 - std::abs(score);
        int moves_to_mate = (plies_to_mate + 1) / 2;
        if (score > 0) {
            return "mate " + std::to_string(moves_to_mate);
        } else {
            return "mate -" + std::to_string(moves_to_mate);
        }
    } 
    return "cp " + std::to_string(score);
}

