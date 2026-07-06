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

/**
 * @brief generates a zobrist hash number for each type of piece in each square,
 * castling, en passant, and the turn
 */
void init_zobrist_table();

/**
 * @brief converts the shorter number representation of the promotion piece in
 * the move data to the longer representation of the piece
 * @param move the move done
 * @param promotion_piece the piece the pawn is to be promoted to
 * @return the longer representation of the piece
 */
uint8_t convert_promotion_piece(const Move& move, const uint8_t& promotion_piece);

/**
 * @brief gets the colour of a piece 
 * @param piece the number representation of the piece
 * @return 0 (WHITE) or 1 (BLACK)
 */
int get_piece_colour(uint8_t piece);

