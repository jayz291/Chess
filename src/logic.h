#pragma once
#include "game.h"
#include "engine.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>

extern uint64_t zobrist_table[16][64];
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

void replace_piece(Game& game, int turn, uint8_t prev_piece, uint8_t new_piece, int target_square);
void remove_piece(Game& game, int turn, uint8_t target_piece, int target_square);
void place_piece(Game& game, int turn, uint8_t target_piece, int target_square);
void restore_zobrist_en_passant_and_castling(Game& game, Move& prev_move);
void update_zobrist_en_passant(Game& game, Move& move);
void make_game_move(Game& game, int result, Move move);
void move_piece(Game& game, int target_piece, int from_square, int to_square, int turn);
void undo_game_move(Game& game);
void undo_move(Game& game, Move& prev_move);
void handle_pawn_promotion(Game& game, Move& move);
void flip_move(Move& move);

bool determine_repetition(Game& game);
bool determine_insufficient_material(Game& game);
void end_game(Game& game);
void is_game_over(Game& game);

uint8_t convert_promotion_piece(Move& move, const uint8_t& promotion_piece);
int get_piece_colour(uint8_t piece);
