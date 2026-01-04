#pragma once
#include "game.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>

extern uint64_t zobrist_table[12][64];
extern uint64_t zobrist_castling[16];
extern uint64_t zobrist_en_passant[9];
extern uint64_t zobrist_black_turn;

void init_zobrist_table();
void find_position_hash(Game& game);

int validate_move(Game& game, Move& move);
int validate_pawn_move(Game& game, Move& move);
int validate_knight_move(Bitboards& bitboards, Move& move);
int validate_bishop_move(Bitboards& bitboards, Move& move);
int validate_rook_move(Bitboards& bitboards, Move& move);
int validate_queen_move(Bitboards& bitboards, Move& move);
int validate_king_move(Game& game, Move& move);
int is_in_check(Game& game, Move& move);
void evaluate_king_checks(Game& game);
int validate_castling(Game& game, Move& move);
int validate_en_passant(Game& game, Move& move);
int is_square_attacked(Bitboards& bitboard_copy, int square, int turn);
void update_castling_flags(Game& game, Move& move);

void replace_piece(Game& game, int turn, int prev_piece, int new_piece, int target_square);
void remove_piece(Game& game, int turn, int target_piece, int target_square);
void place_piece(Game& game, int turn, int target_piece, int target_square);
void restore_zobrist_en_passant_and_castling(Game& game, Move& prev_move);
void update_zobrist_en_passant(Game& game, Move& move);
void make_game_move(Game& game, int result, Move move);
void make_test_move(Game& game, Move& move);
void move_piece(Game& game, Move& move);
void undo_game_move(Game& game);
void undo_test_move(Game& game, Move& prev_move);
void undo_move(Game& game, Move& prev_move);
void handle_pawn_promotion(Game& game, Move& move);
void flip_move(Move& move);

Move_list determine_possible_moves(Game& game);
inline void add_pawn_moves(Game& game, Move_list& moves);
inline void add_knight_moves(Game& game, Move_list& moves);
inline void add_bishop_moves(Game& game, Move_list& moves);
inline void add_rook_moves(Game& game, Move_list& moves);
inline void add_queen_moves(Game& game, Move_list& moves);
inline void add_king_moves(Game& game, Move_list& moves);
bool determine_repetition(Game& game);
bool determine_insufficient_material(Game& game);
void end_game(Game& game);
void is_game_over(Game& game);

void generate_computer_move(Game& game);
void make_computer_move(Game& game);
Move get_best_move(Game& game, int depth);
int negamax(Game& game, int depth, int alpha, int beta);
int evaluate(Game& game, Bitboards& bitboards);
int sort_moves_by_priority(Game& game, Move& move);
inline int positional_eval(Game& game, uint64_t bitboard, int piece, int colour);

