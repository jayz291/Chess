#pragma once
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>

constexpr int SQUARE_SIZE = 95;

struct Piece;

struct Piece {
    std::string piece_type {};
    std::string colour {};
    int moves { 0 };
    int row;
    int col; 
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
    std::string turn {};
    std::string piece {};
    std::shared_ptr<Piece> piece_taken { nullptr };
    std::string special_move { "No" };
    int eval { 0 };
};

using Chessboard = std::array<std::array<Cell, 8>, 8>;

struct Bitboards {
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
    uint64_t white_occupied = white_pawns | white_knights | white_rooks | white_bishops | white_king;
    uint64_t black_occupied = black_pawns | black_knights | black_rooks | black_bishops | black_king;
    uint64_t occupied = white_occupied | black_occupied;
    uint64_t knight_attacks[64];
    uint64_t king_moves[64];
    void update_occupied() {
        white_occupied = white_pawns | white_knights | white_rooks | white_bishops | white_king;
        black_occupied = black_pawns | black_knights | black_rooks | black_bishops | black_king;
        occupied = white_occupied | black_occupied;
    }
};

extern uint64_t FILE_H;
extern uint64_t FILE_A;
extern uint64_t FILE_B;
extern uint64_t FILE_G;
extern uint64_t FILE_AB;
extern uint64_t FILE_GH;
extern uint64_t RANK_4;
extern uint64_t RANK_5;


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







