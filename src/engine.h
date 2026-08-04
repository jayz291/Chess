#pragma once
#include "game.h"
#include "gui.h"

extern std::atomic<bool> terminate_search;
constexpr int CHECKMATE_THRESHOLD = 300000;
constexpr int NO_ENTRY_FOUND = -999999;

/**
 * @brief flags for different types of positions stored in the transposition table
 */
enum tt_flag {
    tt_exact, ///< position that stayed within the window (good enough for both players) (eval score is exact)
    tt_alpha, ///< position isn't good enough for the current player (eval score is an upper bound)
    tt_beta, ///< position will be avoided by the opposing player (eval score is a lower bound)
};

struct table_entry {
    uint64_t zobrist_key;
    int eval;
    int depth;
    tt_flag flag;
    Move best_move;
};

constexpr int TABLE_SIZE = 1048576;
inline table_entry transposition_table[TABLE_SIZE];
inline int history_heuristic_table[64][64];


constexpr int piece_values[8] = { 0, 100, 320, 330, 500, 900, 20000, 0 };
constexpr int RANK_SCORES[8] = { 0, 10, 15, 20, 40, 80, 160, 0 }; ///< score bonus for pawns 

constexpr int game_phase_vals[12] = {0,0,1,1,2,4,0,0};

constexpr int mg_value[6] = { 82, 337, 365, 477, 1025, 0 };
constexpr int eg_value[6] = { 94, 281, 297, 512,  936, 0 };

constexpr int mobility_bonuses[2][4] = { {2, 2, 3, 3}, {3, 3, 2, 2}};

constexpr int start_value_tables[6][64] = {
    { 0,   0,   0,   0,   0,   0,   0,   0,
    -35,  -1, -20, -23, -15,  24,  38, -22,
    -26,  -4,  -4, -10,   3,   3,  33, -12,
    -27,  -2,  -5,  12,  17,   6,  10, -25,
    -14,  13,   6,  21,  23,  12,  17, -23,
     -6,   7,  26,  31,  65,  56,  25, -20,
     98, 134,  61,  95,  68, 126,  34, -11,
      0,   0,   0,   0,   0,   0,   0,   0 },
    { -105, -21, -58, -33, -17, -28, -19, -23,
    -29, -53, -12,  -3,  -1,  18, -14, -19,
    -23,  -9,  12,  10,  19,  17,  25, -16,
    -13,   4,  16,  13,  28,  19,  21,  -8,
     -9,  17,  19,  53,  37,  69,  18,  22,
    -47,  60,  37,  65,  84, 129,  73,  44,
    -73, -41,  72,  36,  23,  62,   7, -17,
   -167, -89, -34, -49,  61, -97, -15, -107 }, 
   { -33,  -3, -14, -21, -13, -12, -39, -21,
      4,  15,  16,   0,   7,  21,  33,   1,
      0,  15,  15,  15,  14,  27,  18,  10,
     -6,  13,  13,  26,  34,  12,  10,   4,
     -4,   5,  19,  50,  37,  37,   7,  -2,
    -16,  37,  43,  40,  35,  50,  37,  -2,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -29,   4, -82, -37, -25, -42,   7,  -8 },
    { -19, -13,   1,  17,  16,   7, -37, -26,
    -44, -16, -20,  -9,  -1,  11,  -6, -71,
    -45, -25, -16, -17,   3,   0,  -5, -33,
    -36, -26, -12,  -1,   9,  -7,   6, -23,
    -24, -11,   7,  26,  24,  35,  -8, -20,
     -5,  19,  26,  36,  17,  45,  61,  16,
     27,  32,  58,  62,  80,  67,  26,  44,
     32,  42,  32,  51,  63,   9,  31,  43 },
     { -1, -18,  -9,  10, -15, -25, -31, -50,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -28,   0,  29,  12,  59,  44,  43,  45 },
    { -15,  36,  12, -54,   8, -28,  24,  14,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -14, -14, -22, -46, -44, -30, -15, -27,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -17, -20, -12, -27, -30, -25, -14, -36,
     -9,  24,   2, -16, -20,   6,  22, -22,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
    -65,  23,  16, -15, -56, -34,   2,  13 }
};

constexpr int endgame_value_tables[6][64] = {
    { 0,   0,   0,   0,   0,   0,   0,   0,
     13,   8,   8,  10,  13,   0,   2,  -7,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
     32,  24,  13,   5,  -2,   4,  17,  17,
     94, 100,  85,  67,  56,  53,  82,  84,
    178, 173, 158, 134, 147, 132, 165, 187,
      0,   0,   0,   0,   0,   0,   0,   0},
    { -29, -51, -23, -15, -22, -18, -50, -64,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -58, -38, -13, -28, -31, -27, -63, -99 },
    { -23,  -9, -23,  -5,  -9, -16,  -5, -17,
    -14, -18,  -7,  -1,   4,  -9, -15, -27,
    -12,  -3,   8,  10,  13,   3,  -7, -15,
     -6,   3,  13,  19,   7,  10,  -3,  -9,
     -3,   9,  12,   9,  14,  10,   3,   2,
      2,  -8,   0,  -1,  -2,   6,   0,   4,
     -8,  -4,   7, -12,  -3, -13,  -4, -14,
    -14, -21, -11,  -8,  -7,  -9, -17, -24 },
    { -9,   2,   3,  -1,  -5, -13,   4, -20,
     -6,  -6,   0,   2,  -9,  -9, -11,  -3,
     -4,   0,  -5,  -1,  -7, -12,  -8, -16,
      3,   5,   8,   4,  -5,  -6,  -8, -11,
      4,   3,  13,   1,   2,   1,  -1,   2,
      7,   7,   7,   5,   4,  -3,  -5,  -3,
     11,  13,  13,  11,  -3,   3,   8,   3,
     13,  10,  18,  15,  12,  12,   8,   5 },
     { -33, -28, -22, -43,  -5, -32, -20, -41,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -18,  28,  19,  47,  31,  34,  39,  23,
      3,  22,  24,  45,  57,  40,  57,  36,
    -20,   6,   9,  49,  47,  35,  19,   9,
    -17,  20,  32,  41,  58,  25,  30,   0,
     -9,  22,  22,  27,  27,  19,  10,  20 },
    { -53, -34, -21, -11, -28, -14, -24, -43,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -18,  -4,  21,  24,  27,  23,   9, -11,
     -8,  22,  24,  27,  26,  33,  26,   3,
     10,  17,  23,  15,  20,  45,  44,  13,
    -12,  17,  14,  17,  17,  38,  23,  11,
    -74, -35, -18, -18, -11,  15,   4, -17}
};

inline int mg_table[6][64];
inline int eg_table[6][64];

/**
 * @brief initiates the algorithms used to generate the computer's move
 * @param position class containing info on the current position
 * @param time the amount of time the computer is allowed to evaluate for
 */
void generate_computer_move(Position& position, const double& time);

/**
 * @brief clears all the data in the transposition table 
 */
void clear_transposition_table();

/**
 * @brief initialises the 2D table for the history heuristic 
 */
void init_history_heuristic_table();

/**
 * @brief initialises the piece square tables based off of PeSTO's evaluation function
 */
void init_pesto_tables();

/**
 * @brief scales down existing (stale) entries in the history table 
 */
void scale_down_history_table();

/**
 * @brief sets the thinking time of the computer
 * @param game class containg all game variables/classes
 * @return the set time in milliseconds 
 */
int set_thinking_time(Game& game);

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
     * @return the evaluation for the move
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
     * @param move the move to be scored
     * @return a score for the move (higher scores are prioritised)
     */
    int sort_moves_by_priority(const Move& move, const Move& stored_move);

    /**
     * @brief returns a score for a capture move based on MVV-LVA (most valuable victim, least
     * valuable attacker)
     * @param move the move to be scored
     * @return the score of the move, determined by subtracting the value of the captured piece
     * from the value of the capturing piece
     */
    int mvv_lva(const Move& move);

    /**
     * @brief returns the evaluation for a certain type of piece based on the position
     * of the pieces on the board, using a preset piece square table. 
     * @param bitboard the bitboard showing which square the piece occupies 
     * @param piece the piece being evaluated
     * @param black a boolean flag which is true if the piece is black and false otherwise 
     * @param start_value_table a boolean flag which is true if the values from the midgame PeSTO table are to be
     * used, and false if the values from the endgame PeSTO tables are to be used
     * @return the evaluation for the position (positive for white leading, negative for black leading)
     */
    inline int positional_eval(uint64_t bitboard, uint8_t piece, bool black, bool start_value_table);

    /**
     * @brief search captures deeper, until the position is "quiet": defined as if there are
     * no capture moves remaining
     * @param alpha the highest score that the maximising player can guarantee
     * @param beta the lowest score that the minimising player can guarantee
     * @param ply how deep the search is from the root position 
     * @param seldepth how deep the chess engine searched for certain lines (passed as reference, to be updated)
     * @return the alpha score generated
     */
    int quiescence_search(int alpha, int beta, int ply, int& seldepth);

    /**
     * @brief evaluates the pawn structure, penalizing doubled-up pawns 
     * @return the evaluation (positive if white is leading and negative if black is leading)
     */
    inline int pawn_structure_eval();

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
     * @brief calculates a mobility evaluation based on how many free squares each major piece has, 
     * and also king safety
     * @param game_phase number that roughly indicates the value of the pieces on the board
     * @return the evaluation (positive if white has more mobile pieces, negative if black does)
     */
    int mobility_eval(const int& game_phase);

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



