#include "definitions.h"
#include "game.h"
#include "logic.h"

/**
 * @brief calculates how many positions there are to a certain depth
 * @param position class containing info on the current position
 * @param depth how much more ply there is to search
 * @return number of positions found
 */
long long perft(Position& position, int depth);

/**
 * @brief function to run the perft test and print detailed results.
 * Prints the number of positions reached from each move from the starting position.
 * @param position class containing info on the current position
 * @param depth the depth to search toa
 */
void run_perft_suite(Position& position, int depth);

/**
 * @brief converts the move encoding to long algebraic chess notation
 * @param move the move to convert
 * @return long algebraic chess notation representing the move
 */
std::string to_chess_notation(const Move& move);

/**
 * @brief parses moves inputted in uci mode - converts moves from long algebraic chess
 * notation to a move encoding
 * @param game class containing all game variables/classes
 * @param s move in long algebraic chess notation
 * @return the move encoding 
 */
Move parse_move_string(Game& game, const std::string& s);


