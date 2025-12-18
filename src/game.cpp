#include "game.h"

Move calculated_move;
std::atomic<bool> computer_turn { false };
std::atomic<bool> thinking_in_progress { false };
std::atomic<bool> finished { false };
int positions_searched { 0 };

Game::Game() {
    initialise();
}

void Game::initialise() {
    state = Gamestate::Playing;
    current_move = {};
    move_record.clear();
    board_record.clear();
    pawns_on_board = true;
    value_white_pieces = value_black_pieces = 0;
    plys_to_100 = 0;
    winner = piece_selected = -1;
    white_in_check = black_in_check = game_over = false;
    turn = white;
    checkmate = stalemate = repetition = insufficient_material = promoting_pawn = false;
    selected.col = selected.row = -1;
    black_king_position.row = 0;
    white_king_position.row = 7;
    black_king_position.col = white_king_position.col = 4;
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
        board[i][j].piece_occupying = nullptr;
            if ((i + j) % 2 == 0) {
                board[i][j].colour = "yellow";
            } else {
                board[i][j].colour = "brown";
            }
        }
    }
    for (int i { 0 }; i < 8; i++) {
        board[1][i].piece_occupying = std::make_shared<Piece> (pawn, black, 0, 1, i);
    }
    for (int i { 0 }; i < 8; i++) {
        board[6][i].piece_occupying = std::make_shared<Piece> (pawn, white, 0, 6, i);
    }
    board[0][0].piece_occupying = std::make_shared<Piece> (rook, black, 0, 0, 0);
    board[0][1].piece_occupying = std::make_shared<Piece> (knight, black, 0, 0, 1);
    board[0][2].piece_occupying = std::make_shared<Piece> (bishop, black, 0, 0, 2);
    board[0][3].piece_occupying = std::make_shared<Piece> (queen, black, 0, 0, 3);
    board[0][4].piece_occupying = std::make_shared<Piece> (king, black, 0, 0, 4);
    board[0][5].piece_occupying = std::make_shared<Piece> (bishop, black, 0, 0, 5);
    board[0][6].piece_occupying = std::make_shared<Piece> (knight, black, 0, 0, 6);
    board[0][7].piece_occupying = std::make_shared<Piece> (rook, black, 0, 0, 7);
    board[7][0].piece_occupying = std::make_shared<Piece> (rook, white, 0, 7, 0);
    board[7][1].piece_occupying = std::make_shared<Piece> (knight, white, 0, 7, 1);
    board[7][2].piece_occupying = std::make_shared<Piece> (bishop, white, 0, 7, 2);
    board[7][3].piece_occupying = std::make_shared<Piece> (queen, white, 0, 7, 3);
    board[7][4].piece_occupying = std::make_shared<Piece> (king, white, 0, 7, 4);
    board[7][5].piece_occupying = std::make_shared<Piece> (bishop, white, 0, 7, 5);
    board[7][6].piece_occupying = std::make_shared<Piece> (knight, white, 0, 7, 6);
    board[7][7].piece_occupying = std::make_shared<Piece> (rook, white, 0, 7, 7);
}


std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << "Prev row and col: " << move.prev_row << ' ' << move.prev_col << '\n';
    os << "New row and col: " << move.new_row << ' ' << move.new_col << '\n';
    return os;
}

