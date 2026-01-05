#pragma once
#include "game.h"
#include "logic.h"

void clear_transposition_table();
void record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move);
int probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move);

void make_test_move(Game& game, Move& move);
void undo_test_move(Game& game, Move& prev_move);

Move_list determine_possible_moves(Game& game);
inline void add_pawn_moves(Game& game, Move_list& moves);
inline void add_knight_moves(Game& game, Move_list& moves);
inline void add_bishop_moves(Game& game, Move_list& moves);
inline void add_rook_moves(Game& game, Move_list& moves);
inline void add_queen_moves(Game& game, Move_list& moves);
inline void add_king_moves(Game& game, Move_list& moves);

void generate_computer_move(Game& game);
void make_computer_move(Game& game);
Move get_best_move(Game& game, int search_allocated_time_ms);
int negamax(Game& game, int depth, int alpha, int beta, int search_allocated_time_ms);
inline int find_eval(Game& game, int move_num, int depth, int beta, int alpha, int search_allocated_time_ms);
int evaluate(Game& game);
int sort_moves_by_priority(Game& game, Move& move);
inline int positional_eval(Game& game, uint64_t bitboard, int piece, int colour);

