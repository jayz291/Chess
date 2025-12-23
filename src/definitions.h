#pragma once
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <bitset>

constexpr int SQUARE_SIZE = 95;

struct Piece;

struct Piece {
    int piece_type {};
    int colour {};
};

struct Cell {
    std::string colour {};
    std::shared_ptr<Piece> piece_occupying { nullptr };
    bool selected { false };
};

using Chessboard = std::array<std::array<Cell, 8>, 8>;

struct Coords {
    int row { -1 };
    int col { -1 };
};

struct Move {
    int prev_row { 0 };
    int prev_col { 0 };
    int new_row { 0 };
    int new_col { 0 };
    int turn {};
    int piece {};
    std::shared_ptr<Piece> piece_taken { nullptr };
    std::string special_move { "No" };
    uint8_t castling_rights {};
};

// flags: captured piece, castling, en passant, promotion

typedef u_int16_t Encoded_move;
const uint16_t MASK_FROM = 0x3F;
const uint16_t MASK_TO = 0xFC0;
const uint16_t MASK_FLAGS = 0xF000;

using Chessboard = std::array<std::array<Cell, 8>, 8>;

enum {
    white = 0,
    black = 1
};

enum {
    pawn = 0,
    knight = 1,
    bishop = 2,
    rook = 3,
    queen = 4,
    king = 5
};

extern uint64_t FILE_H;
extern uint64_t FILE_A;
extern uint64_t FILE_B;
extern uint64_t FILE_G;
extern uint64_t FILE_AB;
extern uint64_t FILE_GH;
extern uint64_t RANK_4;
extern uint64_t RANK_5;
extern uint64_t RANK_2;
extern uint64_t RANK_7;

struct Bitboards {
    uint64_t bitboards[2][6];
    uint64_t white_pawns = 0x000000000000FF00ULL;
    uint64_t white_knights = 0x0000000000000042ULL;
    uint64_t white_rooks = 0x0000000000000081ULL;
    uint64_t white_bishops = 0x0000000000000024ULL;
    uint64_t white_queens = 0x0000000000000008ULL;
    uint64_t white_king = 0x0000000000000010ULL;
    uint64_t black_pawns = 0x00FF000000000000ULL;
    uint64_t black_knights = 0x4200000000000000ULL;
    uint64_t black_rooks = 0x8100000000000000ULL;
    uint64_t black_bishops = 0x2400000000000000ULL;
    uint64_t black_queens = 0x0800000000000000ULL;
    uint64_t black_king = 0x1000000000000000ULL;
    uint64_t occupied_tables[2];
    uint64_t white_occupied = white_pawns | white_knights | white_rooks | white_bishops | white_queens | white_king;
    uint64_t black_occupied = black_pawns | black_knights | black_rooks | black_bishops | black_queens | black_king;
    uint64_t occupied = white_occupied | black_occupied;
    uint64_t knight_attacks[64];
    uint64_t king_moves[64];
    uint64_t pawn_attacks[2][64];
    uint64_t pawn_moves[2][64];
    uint64_t between_table[64][64];
    Bitboards() {
        bitboards[0][0] = white_pawns;
        bitboards[0][1] = white_knights;
        bitboards[0][2] = white_bishops;
        bitboards[0][3] = white_rooks;
        bitboards[0][4] = white_queens;
        bitboards[0][5] = white_king;
        bitboards[1][0] = black_pawns;
        bitboards[1][1] = black_knights;
        bitboards[1][2] = black_bishops;
        bitboards[1][3] = black_rooks;
        bitboards[1][4] = black_queens;
        bitboards[1][5] = black_king;
        occupied_tables[0] = white_occupied;
        occupied_tables[1] = black_occupied;
        find_valid_knight_moves();
        find_valid_king_moves();
        make_between_table();
        find_pawn_attacks();
    }
    void update_occupied() {
        white_occupied = bitboards[0][0] | bitboards[0][1] | bitboards[0][2] | bitboards[0][3] | bitboards[0][4] |
        bitboards[0][5];
        black_occupied = bitboards[1][0] | bitboards[1][1] | bitboards[1][2] | bitboards[1][3] | bitboards[1][4] |
        bitboards[1][5];
        occupied_tables[0] = white_occupied;
        occupied_tables[1] = black_occupied;
        occupied = white_occupied | black_occupied;
    }
    void find_valid_knight_moves() {
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t moves = 0;
            moves |= (position >> 17 & ~FILE_H);
            moves |= (position >> 15 & ~FILE_A);
            moves |= (position >> 10 & ~FILE_GH);
            moves |= (position >> 6 & ~FILE_AB);
            moves |= (position << 6 & ~FILE_GH);
            moves |= (position << 10 & ~FILE_AB);
            moves |= (position << 15 & ~FILE_H);
            moves |= (position << 17 & ~FILE_A);
            knight_attacks[cell] = moves;
        }
    }
    void find_valid_king_moves() {
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t moves = 0;
            moves |= (position >> 9 & ~FILE_H);
            moves |= (position >> 8);
            moves |= (position >> 7 & ~FILE_A);
            moves |= (position >> 1 & ~FILE_H);
            moves |= (position << 1 & ~FILE_A);
            moves |= (position << 7 & ~FILE_H);
            moves |= (position << 8);
            moves |= (position << 9 & ~FILE_A);
            king_moves[cell] = moves;
        }
    }
    void make_between_table() {
        for (int from { 0 }; from < 64; from++) {
            for (int to { 0 }; to < 64; to++) {
                int row_change = to / 8 - from / 8;
                int col_change = to % 8 - from % 8;
                uint64_t mask = 0ULL;
                if (std::abs(row_change) == std::abs(col_change) && from != to) {
                    int row_step = ((row_change < 0) ? -1 : 1);
                    int col_step = ((col_change < 0) ? -1 : 1);
                    int curr_square = from + 8 * row_step + col_step;
                    while (curr_square != to) {
                        mask |= (1ULL << curr_square);
                        curr_square += 8 * row_step + col_step;
                    }
                } else if (std::abs(row_change) == 0 && from != to) {
                    int col_step = ((col_change < 0) ? -1 : 1);
                    int curr_square = from + col_step;
                    while (curr_square != to) {
                        mask |= (1ULL << curr_square);
                        curr_square += col_step;
                    }
                } else if (std::abs(col_change) == 0 && from != to) {
                    int row_step = ((row_change < 0) ? -1 : 1);
                    int curr_square = from + 8 * row_step;
                    while (curr_square != to) {
                        mask |= (1ULL << curr_square);
                        curr_square += 8 * row_step;
                    }
                }
                between_table[from][to] = mask;
            }
        }
    }
    void find_pawn_attacks() {
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t non_capture_moves = 0ULL;
            uint64_t capture_moves = 0ULL;
            non_capture_moves |= (position << 8);
            capture_moves |= (position << 7 & ~FILE_H);
            capture_moves |= (position << 9 & ~FILE_A);
            if (position & RANK_2) {
                non_capture_moves |= (position << 16);
            }
            pawn_attacks[white][cell] = capture_moves;
            pawn_moves[white][cell] = non_capture_moves;
        }
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t non_capture_moves = 0ULL;
            uint64_t capture_moves = 0ULL;
            non_capture_moves |= (position >> 8);
            capture_moves |= (position >> 7 & ~FILE_A);
            capture_moves |= (position >> 9 & ~FILE_H);
            if (position & RANK_7) {
                non_capture_moves |= (position >> 16);
            }
            pawn_attacks[black][cell] = capture_moves;
            pawn_moves[black][cell] = non_capture_moves;
        }
    }
};


enum class Gamestate {
    Intro,
    Playing,
    Promoting_pawn,
    Gameover,
    Resetting
};

enum class Gamemode {
    CPUwhite,
    CPUblack,
    Twoplayer
};

std::ostream& operator<<(std::ostream& os, const Move& move);







