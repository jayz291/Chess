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

void make_game_move(Game& game, int result, Move move, bool is_game_over = false);
void undo_game_move(Game& game, bool is_game_over = false);
void handle_pawn_promotion(Game& game, Move& move, bool piece_already_selected = false, bool is_game_over = false);

bool determine_repetition(Position& position, Result& result);
bool determine_repetition(Position& position);
bool determine_insufficient_material(Position& position, Result& result);
void end_game(Position& position, Result& result);
void is_game_over(Game& game);

uint8_t convert_promotion_piece(Move& move, const uint8_t& promotion_piece);
int get_piece_colour(uint8_t piece);

