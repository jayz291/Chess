#include "definitions.h"
#include "game.h"
#include "logic.h"

long long perft(Position& position, int depth);
void run_perft_suite(Position& position, int depth);
std::string to_chess_notation(const Move& move);
Move parse_move_string(Game& game, const std::string& s);


