#pragma once
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <bitset>
#include <algorithm>

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

const int piece_values[5] = { 100, 300, 300, 500, 900 };

const int white_pawn_square_table[64] = { 
    0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10, 
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5, 
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0 
};
const int black_pawn_square_table[64] = { 
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, -20, -20, 10, 10, 5,
    5, -5, -10, 0, 0, -10, -5, 5, 
    0, 0, 0, 20, 20, 0, 0, 0,
    5, 5, 10, 25, 25, 10, 5, 5,
    10, 10, 20, 30, 30, 20, 10, 10, 
    50, 50, 50, 50, 50, 50, 50, 50,
    0, 0, 0, 0, 0, 0, 0, 0 
};
const int knights_table[64] = { 
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50 
};
const int bishop_table[64] = { 
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20 
};
const int rook_table[64] = { 
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0 
};
const int queen_table[64] = { 
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20 
};
const int king_table_beginning_white[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};
const int king_table_beginning_black[64] = {
    20, 30, 10,  0,  0, 10, 30, 20,
    20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};

const int king_table_endgame_white[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

const int king_table_endgame_black[64] = {
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-20,-10,  0,  0,-10,-20,-30,
    50,-40,-30,-20,-20,-30,-40,-50
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
    uint64_t occupied_tables[2];
    uint64_t white_occupied;
    uint64_t black_occupied;
    uint64_t occupied;
    uint64_t knight_attacks[64];
    uint64_t king_moves[64];
    uint64_t pawn_attacks[2][64];
    uint64_t pawn_moves[2][64];
    uint64_t between_table[64][64];
    Bitboards() {
        bitboards[white][pawn] = 0x000000000000FF00ULL;
        bitboards[white][knight] = 0x0000000000000042ULL;
        bitboards[white][bishop] = 0x0000000000000024ULL;
        bitboards[white][rook] = 0x0000000000000081ULL;
        bitboards[white][queen] = 0x0000000000000008ULL;
        bitboards[white][king] = 0x0000000000000010ULL;
        bitboards[black][pawn] = 0x00FF000000000000ULL;
        bitboards[black][knight] = 0x4200000000000000ULL;
        bitboards[black][bishop] = 0x2400000000000000ULL;
        bitboards[black][rook] = 0x8100000000000000ULL;
        bitboards[black][queen] = 0x0800000000000000ULL;
        bitboards[black][king] = 0x1000000000000000ULL;
        update_occupied();
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







