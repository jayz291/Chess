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
    selected_square = -1;
    bitboards = {};
    castling_rights = 0b00001111;
    for (int i { 0 }; i < 64; i++) {
        board[i].piece_occupying = { none, none };
        int row = i / 8;
        int col = i % 8;
        if ((row + col) % 2 == 0) {
            board[i].colour = "yellow";
        } else {
            board[i].colour = "brown";
        }
    }
    for (int i { 48 }; i < 56; i++) {
        board[i].piece_occupying = { pawn, black };
    }
    for (int i { 8 }; i < 16; i++) {
        board[i].piece_occupying = { pawn, white };
    }
    board[56].piece_occupying = { rook, black };
    board[57].piece_occupying = { knight, black };
    board[58].piece_occupying = { bishop, black };
    board[59].piece_occupying = { queen, black };
    board[60].piece_occupying = { king, black };
    board[61].piece_occupying = { bishop, black };
    board[62].piece_occupying = { knight, black };
    board[63].piece_occupying = { rook, black };
    board[0].piece_occupying = { rook, white };
    board[1].piece_occupying = { knight, white };
    board[2].piece_occupying = { bishop, white };
    board[3].piece_occupying = { queen, white };
    board[4].piece_occupying = { king, white };
    board[5].piece_occupying = { bishop, white };
    board[6].piece_occupying = { knight, white };
    board[7].piece_occupying = { rook, white };
}


std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << "Piece: " << move.piece << '\n';
    os << "Prev row and col: " << move.prev_square << ' ' << move.new_square << '\n';
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

