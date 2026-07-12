#pragma once
#include "game.h"
#include "gui.h"

extern std::atomic<bool> terminate_search;
constexpr int CHECKMATE_THRESHOLD = 300000;
constexpr int NO_ENTRY_FOUND = -999999;

/**
 * @brief makes the move selected by the negamax function
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 */
void make_computer_move(Game& game, Assets& assets);

/**
 * @brief initiates the algorithms used to generate the computer's move
 * @param position class containing info on the current position
 */
void generate_computer_move(Position& position);

/**
 * @brief clears all the data in the transposition table 
 */
void clear_transposition_table();

struct Engine {
    Position& position;
    int search_allocated_time_ms;
    long long nodes_searched { 0 };
    std::chrono::steady_clock::time_point start_time;
    Engine(Position& position, int search_allocated_time_ms) : position(position), 
    search_allocated_time_ms(search_allocated_time_ms) {
        nodes_searched = 0;
        terminate_search = false;
        start_time = std::chrono::steady_clock::now();
    }

    /**
     * @brief records an entry in the transposition table
     * @param key 
     * @param eval the eval score for the position 
     * @param depth how deep the position was searched (in ply)
     * @param flag records the nature of the score (either exact, upper bound or lower bound)
     * @param best_move the best move calculated from this position
     */
    void record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move, int ply);

    /**
     * @brief retrieves a position and its evaluation from the transposition table
     * @param key the zobrist hash for the position 
     * @param depth how deep the algorithm searched to reach this evaluation 
     * @param alpha the highest score that the maximising player can guarantee
     * @param beta the lowest score that the minimising player can guarantee
     * @param best_move variable passed in as reference, which may store the best move recorded in the 
     * transposition table for that position 
     * @param ply distance to the root
     * @return the evaluation score from the transposition table, or the sentinel value for NO_ENTRY_FOUND
     * if no entry if found
     */
    int probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move, int ply);

    /**
     * @brief function that initiates the move evaluation and search
     * @param search_depth how deep the position should search (through iterative deepening).
     * However, the search will likely be cut off by time. 
     * @return the best move calculated
     */
    Move get_best_move(int search_depth = 40);

    /**
     * @brief negamax function for evaluating position scores.
     * Uses quiescence search, check extensions, and transposition tables to improve the efficiency/quality 
     * of the search. 
     * @param depth how much more ply there is to search 
     * @param alpha the highest score that the maximising player can guarantee
     * @param beta the lowest score that the minimising player can guarantee
     * @param ply how deep the search is from the root position 
     * @param seldepth how deep the chess engine searched for certain lines (passed as reference, to be updated)
     * @return the evaluation score for the player assuming that both players make optimal moves
     */
    int negamax(int depth, int alpha, int beta, int ply, int& seldepth);

    /**
     * @brief principal variation search. 
     * It does a full window search on only the first move, and a narrow window search on the
     * rest of the moves. If a move is better than expected, it does a new full window search. 
     * @param move_num
     * @param depth how much more ply there is to search 
     * @param beta the lowest score that the minimising player can guarantee
     * @param alpha the highest score that the maximising player can guarantee
     * @param ply how deep the search is from the root position 
     * @param seldepth how deep the chess engine searched for certain lines (passed as reference, to be updated)
     * @return 
     */
    inline int find_eval(int move_num, int depth, int beta, int alpha, int ply, int& seldepth);

    /**
     * @brief evaluation function, based on material and piece square tables.
     * A positive evaluation means that white is leading, while a negative evaluate means black is leading.
     * @return the evaluation
     */
    int evaluate();

    /**
     * @brief helper function to order moves so that moves likelier to be better are first.
     * Uses MVV-LVA heavily (Most valuable victim, least valuable attacker)
     * @param move the move for the evaluation guess 
     * @return a score for the move (higher scores are prioritised)
     */
    int sort_moves_by_priority(const Move& move);

    /**
     * @brief returns the evaluation for a certain type of piece based on the position
     * of the pieces on the board, using a preset piece square table. 
     * @param bitboard the bitboard showing which square the piece occupies 
     * @param piece the piece being evaluated
     * @param material_phase measure of how many pieces remain on the board
     * @param black a boolean flag which is true if the piece is black and false otherwise 
     * @return the evaluation for the position (positive for white leading, negative for black leading)
     */
    inline int positional_eval(uint64_t bitboard, uint8_t piece, const double& material_phase, bool black = false);

    /**
     * @brief search captures deeper, until the position is "quiet": defined as if there are
     * no capture moves remaining
     * @param alpha the highest score that the maximising player can guarantee
     * @param beta the lowest score that the minimising player can guarantee
     * @param ply how deep the search is from the root position 
     * @param seldepth how deep the chess engine searched for certain lines (passed as reference, to be updated)
     * @return 
     */
    int quiescence_search(int alpha, int beta, int ply, int& seldepth);

    /**
     * @brief evaluates the pawn structure, penalizing doubled-up pawns 
     * @return the evaluation (positive if white is leading and negative if black is leading)
     */
    int pawn_structure_eval();

    /**
     * @brief determines whether a position has been repeated. This prevents the engine from
     * generating the same move again. 
     * @return true if a position has been repeated and false otherwise 
     */
    bool determine_repetition();

    /**
     * @brief checks whether there are still non-pawn and non-king pieces on the board
     * @return true if there are, and false otherwise 
     */
    bool major_pieces_present();

    /**
     * @brief checks whether the king is in check (colour of the current turn)
     * @return true if the king is in check and false otherwise 
     */
    bool is_in_check();

    /**
     * @brief calculates the evaluation for checkmate/stalemate situations
     * @param king the colour of the king 
     * @param ply how deep the search is from the root position
     * @return the evaluation (0 for stalemate, -checkmate number + ply for checkmates)
     */
    int calculate_checkmate_or_stalemate_eval(const uint8_t& king, const int& ply);

    /**
     * @brief calculates a mobility evaluation based on how many free squares each major piece has
     * @return the evaluation (positive if white has more mobile pieces, negative if black does)
     */
    int mobility_eval();

    /**
     * @brief returns an enum value describing the nature of the score being stored in the transposition
     * table
     * @param original_alpha the highest score that the maximising player can guarantee (original baseline)
     * @param beta the lowest score that the minimising player can guarantee
     * @param max_eval maximum evaluation found
     * @return a flag describing whether the evaluation is a lower bound, upper bound, or exact
     */
    inline tt_flag set_entry_flag(const int& original_alpha, const int& beta, const int& max_eval);

    /**
     * @brief checks whether all the allocated time for the search has passed, every 2048 nodes
     * @return true if all the time has elapsed and false otherwise 
     */
    bool is_time_over();
};



