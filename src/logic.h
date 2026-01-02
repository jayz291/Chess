#pragma once
#include "game.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

int validate_move(Game& game, Move& move, bool only_checking_checks = false);
int validate_move_pawn(Game& game, Move& move);
int validate_move_knight(Bitboards& bitboards, Move& move);
int validate_move_bishop(Bitboards& bitboards, Move& move);
int validate_move_rook(Bitboards& bitboards, Move& move);
int validate_move_queen(Bitboards& bitboards, Move& move);
int validate_move_king(Game& game, Move& move);
int is_in_check(Game& game, Move& move);
void evaluate_king_checks(Game& game);
int test_castling(Game& game, Move& move);
int validate_en_passant(Game& game, Move& move);
int is_square_attacked(Bitboards& bitboard_copy, int square, int turn);
void update_castling_flags(Game& game, Move& move);

void make_game_move(Game& game, int result, Move move);
void make_test_move(Game& game, Move& move);
void move_piece(Game& game, Move& move, bool undo = false);
void undo_game_move(Game& game);
void undo_test_move(Game& game, Move& prev_move);
void undo_move(Game& game, Move& prev_move, int turn);
void handle_pawn_promotion(Game& game, Move& move);
void switch_move(Move& move);

Move_list determine_possible_moves(Game& game, bool CPU = false);
inline void add_pawn_moves(Game& game, Move_list& moves);
inline void add_knight_moves(Game& game, Move_list& moves);
inline void add_bishop_moves(Game& game, Move_list& moves);
inline void add_rook_moves(Game& game, Move_list& moves);
inline void add_queen_moves(Game& game, Move_list& moves);
inline void add_king_moves(Game& game, Move_list& moves);
int determine_repetition(Game& game);
int determine_insufficient_material(Game& game);
void end_game(Game& game);
void record_board(Game& game);
void is_game_over(Game& game);
void record_piece_points(Game& game, int piece_type, int piece_colour);

void generate_computer_move(Game& game);
void update_computer_move(Game& game);
Move get_best_move(Game& game, int depth);
int negamax(Game& game, int depth, int alpha, int beta);
int evaluate(Bitboards& bitboards);
int sort_moves_by_priority(Move& move);
int positional_eval(uint64_t bitboard, const int table[]);

