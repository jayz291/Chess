#pragma once
#include "game.h"

extern int terminate_search;
constexpr int CHECKMATE_THRESHOLD = 300000;
constexpr int NO_ENTRY_FOUND = -999999;

void clear_transposition_table();
void record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move, int ply);
int probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move, int ply);

template<bool update_zobrist> bool make_test_move(Game& game, Move& move);
template<bool update_zobrist> void undo_test_move(Game& game, Move& prev_move);
void make_null_move(Game& game, int& stored_ep_square, uint64_t& stored_hash);
void undo_null_move(Game& game, int stored_ep_square, uint64_t stored_hash);

Move_list determine_possible_moves(Game& game);
Move_list generate_captures_only(Game& game);

void generate_computer_move(Game& game);
void make_computer_move(Game& game);
Move get_best_move(Game& game, int search_allocated_time_ms, int search_depth = 40);
int negamax(Game& game, int depth, int alpha, int beta, int search_allocated_time_ms, int ply, int& seldepth);
inline int find_eval(Game& game, int move_num, int depth, int beta, int alpha, int search_allocated_time_ms,
    int ply, int& seldepth);
int evaluate(Game& game);
int sort_moves_by_priority(Game& game, Move& move);
inline int positional_eval(Game& game, uint64_t bitboard, uint8_t piece, bool black = false);
int quiescence_search(Game& game, int alpha, int beta, int ply, int& seldepth);

