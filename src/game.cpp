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
    bitboards = {};
    castling_rights = 0b00001111;
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
        board[1][i].piece_occupying = std::make_unique<Piece> (pawn, black);
    }
    for (int i { 0 }; i < 8; i++) {
        board[6][i].piece_occupying = std::make_unique<Piece> (pawn, white);
    }
    board[0][0].piece_occupying = std::make_unique<Piece> (rook, black);
    board[0][1].piece_occupying = std::make_unique<Piece> (knight, black);
    board[0][2].piece_occupying = std::make_unique<Piece> (bishop, black);
    board[0][3].piece_occupying = std::make_unique<Piece> (queen, black);
    board[0][4].piece_occupying = std::make_unique<Piece> (king, black);
    board[0][5].piece_occupying = std::make_unique<Piece> (bishop, black);
    board[0][6].piece_occupying = std::make_unique<Piece> (knight, black);
    board[0][7].piece_occupying = std::make_unique<Piece> (rook, black);
    board[7][0].piece_occupying = std::make_unique<Piece> (rook, white);
    board[7][1].piece_occupying = std::make_unique<Piece> (knight, white);
    board[7][2].piece_occupying = std::make_unique<Piece> (bishop, white);
    board[7][3].piece_occupying = std::make_unique<Piece> (queen, white);
    board[7][4].piece_occupying = std::make_unique<Piece> (king, white);
    board[7][5].piece_occupying = std::make_unique<Piece> (bishop, white);
    board[7][6].piece_occupying = std::make_unique<Piece> (knight, white);
    board[7][7].piece_occupying = std::make_unique<Piece> (rook, white);
}


std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << "Piece: " << move.piece << '\n';
    os << "Prev row and col: " << move.prev_row << ' ' << move.prev_col << '\n';
    os << "New row and col: " << move.new_row << ' ' << move.new_col << '\n';
    return os;
}

void print_bitboard(uint64_t bitboard) {
    std::cout << "\n";

    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << "  "; 
        
        for (int file = 0; file < 8; ++file) {
            int square_index = rank * 8 + file;
            uint64_t mask = 1ULL << square_index;

            if (bitboard & mask) {
                std::cout << "1 "; 
            } else {
                std::cout << ". "; 
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n\n";
}

void print_all_bitboards(Bitboards& bitboards) {
    std::cout << "-----------------------------------------------\n";
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            std::string colour = ((i == 0) ? "white" : "black");
            
            if (j == 0) {
                std::cout << colour << " pawns";
            }
            if (j == 1) {
                std::cout << colour << " knights";
            }
            if (j == 2) {
                std::cout << colour << " bishops";
            }
            if (j == 3) {
                std::cout << colour << " rooks";
            }
            if (j == 4) {
                std::cout << colour << " queens";
            }
            if (j == 5) {
                std::cout << colour << " king";
            }
            
            print_bitboard(bitboards.bitboards[i][j]);
        }
    }
}

