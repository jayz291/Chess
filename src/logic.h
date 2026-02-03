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
uint8_t convert_promotion_piece(Move& move, const uint8_t& promotion_piece);
int get_piece_colour(uint8_t piece);

