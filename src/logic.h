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
void find_position_hash(Position& position);

int validate_move(Position& position, Move& move);
int validate_pawn_move(Position& position, Move& move);
int validate_knight_move(Bitboards& bitboards, Move& move);
int validate_bishop_move(Bitboards& bitboards, Move& move);
int validate_rook_move(Bitboards& bitboards, Move& move);
int validate_queen_move(Bitboards& bitboards, Move& move);
int validate_king_move(Position& position, Move& move);
void evaluate_king_checks(Position& position);
int validate_castling(Position& position, Move& move);
int is_square_attacked(Bitboards& bitboard_copy, int square, int turn);

void make_game_move(Game& game, int result, Move move, bool is_game_over = false);
void undo_game_move(Game& game, bool is_game_over = false);
void handle_pawn_promotion(Game& game, Move& move, bool piece_already_selected = false, bool is_game_over = false);

bool determine_repetition(Game& game);
bool determine_repetition(Position& position);
bool more_moves_available(Position& position, Move_list moves);
bool determine_insufficient_material(Game& game);
void end_game(Position& position, Result& result);
void is_game_over(Game& game);

uint8_t convert_promotion_piece(Move& move, const uint8_t& promotion_piece);
int get_piece_colour(uint8_t piece);

template<bool update_zobrist> void update_castling_flags(Position& position, Move& move);
template<bool update_zobrist> void replace_piece(Position& position, int turn, 
    uint8_t prev_piece, uint8_t new_piece, int target_square);
template<bool update_zobrist> void remove_piece(Position& position, int turn, uint8_t target_piece, 
    int target_square);
template<bool update_zobrist> void place_piece(Position& position, int turn, uint8_t target_piece, int target_square);
template<bool update_zobrist> void restore_zobrist_en_passant_and_castling(Position& position, Move& prev_move);
template<bool update_zobrist> void update_zobrist_en_passant(Position& position, Move& move);
template<bool update_zobrist> void move_piece(Position& position, uint8_t target_piece, int from_square, int to_square, int turn);
template<bool update_zobrist> void undo_move(Position& position, Move& prev_move);
