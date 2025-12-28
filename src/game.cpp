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
    winner = piece_selected = none;
    white_in_check = black_in_check = game_over = false;
    turn = white;
    checkmate = stalemate = repetition = insufficient_material = promoting_pawn = false;
    selected_square = none;
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

void handle_fen_string(Game& game) {
    std::string fen_string = game.fen_string.toAnsiString();
    std::cout << fen_string << '\n';
    std::vector<std::string> split_fen;
    if (fen_string.size() == 0) {
        return;
    }
    std::stringstream ss(fen_string);
    std::string section;
    while (ss >> section) {
        split_fen.push_back(section);
    }
    if (split_fen.size() != 6) {
        return;
    }
    int curr_square = 56;
    Game proposed_game;
    proposed_game.castling_rights = 0b10000000;
    Chessboard board;
    memset(proposed_game.bitboards.bitboards, 0, sizeof(proposed_game.bitboards.bitboards));
    uint64_t mask = 1ULL;
    for (char letter: split_fen[0]) {
        if (letter == 'r') {
            board[curr_square].piece_occupying = { rook, black };
            proposed_game.bitboards.bitboards[black][rook] |= mask << curr_square;
        } else if (letter == 'n') {
            board[curr_square].piece_occupying = { knight, black };
            proposed_game.bitboards.bitboards[black][knight] |= mask << curr_square;
        } else if (letter == 'b') {
            board[curr_square].piece_occupying = { bishop, black };
            proposed_game.bitboards.bitboards[black][bishop] |= mask << curr_square;
        } else if (letter == 'q') {
            board[curr_square].piece_occupying = { queen, black };
            proposed_game.bitboards.bitboards[black][queen] |= mask << curr_square;
        } else if (letter == 'k') {
            board[curr_square].piece_occupying = { king, black };
            proposed_game.bitboards.bitboards[black][king] |= mask << curr_square;
        } else if (letter == 'p') {
            board[curr_square].piece_occupying = { pawn, black };
            proposed_game.bitboards.bitboards[black][pawn] |= mask << curr_square;          
        } else if (letter == 'R') {
            board[curr_square].piece_occupying = { rook, white };
            proposed_game.bitboards.bitboards[white][rook] |= mask << curr_square;
        } else if (letter == 'N') { 
            board[curr_square].piece_occupying = { knight, white };
            proposed_game.bitboards.bitboards[white][knight] |= mask << curr_square;
        } else if (letter == 'B') {
            board[curr_square].piece_occupying = { bishop, white };
            proposed_game.bitboards.bitboards[white][bishop] |= mask << curr_square;
        } else if (letter == 'Q') {
            board[curr_square].piece_occupying = { queen, white };
            proposed_game.bitboards.bitboards[white][queen] |= mask << curr_square;
        } else if (letter == 'K') {
            board[curr_square].piece_occupying = { king, white };
            proposed_game.bitboards.bitboards[white][king] |= mask << curr_square;
        } else if (letter == 'P') {
            board[curr_square].piece_occupying = { pawn, white };
            proposed_game.bitboards.bitboards[white][pawn] |= mask << curr_square;          
        } 
        std::cout << curr_square << '\n';
        if (curr_square >= 65) {
            std::cout << "here\n";
            return;
        }
        if (letter >= 49 && letter <= 56) {
            int empty_squares = letter - '0';
            curr_square += empty_squares;
        }
        if (letter == '/') {
            std::cout << "next row\n";
            int row = curr_square / 8;
            curr_square = 8 * (row - 2);
            std::cout << "new curr square: " << curr_square << '\n';
        }  
        if (letter > 57) {
            curr_square++;
            std::cout << "added\n";
        }
    }
    proposed_game.bitboards.update_occupied();
 
    if (split_fen[1] == "b") {
        proposed_game.turn = black;
    } else if (split_fen[1] == "w") {
        proposed_game.turn = white;
    } else {
        return;
    }
    
    for (char letter: split_fen[2]) {    
        if (letter == 'K') {
            proposed_game.castling_rights |= (mask << 3);
        } else if (letter == 'Q') {
            proposed_game.castling_rights |= (mask << 2);
        } else if (letter == 'k') {
            proposed_game.castling_rights |= (mask << 1);
        } else if (letter == 'q') {
            proposed_game.castling_rights |= mask;
        } else if (letter == '-') {
            proposed_game.castling_rights = 0b00000000;
        } else {
            return;
        }  
    }
    proposed_game.en_passant_square = split_fen[3];
    proposed_game.plys_to_100 = std::stoi(split_fen[4]);
    std::cout << "plys to 100 : " << game.plys_to_100 << '\n';
    int move_num = std::stoi(split_fen[5]);
    proposed_game.board = board;
    game = proposed_game;
    std::cout << std::bitset<8>(game.castling_rights) << '\n';
    for (int i { 0 }; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        if ((row + col) % 2 == 0) {
            game.board[i].colour = "yellow";
        } else {
            game.board[i].colour = "brown";
        }
    }
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

