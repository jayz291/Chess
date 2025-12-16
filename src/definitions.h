#pragma once
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <memory>

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







