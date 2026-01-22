#include "game.h"
#include "logic.h"
#include "engine.h"

Move calculated_move;
std::atomic<bool> computer_turn { false };
std::atomic<bool> thinking_in_progress { false };
std::atomic<bool> finished { false };
int positions_searched { 0 };

Game::Game() {
    initialise();
}

void Game::initialise() {
    //state = Gamestate::Playing;
    current_move = {};
    calculated_move = {}, 
    dragged_piece = EMPTY_SQUARE, 
    //en_passant_index = 8,
    zobrist_hash = 0ULL,
    value_white_pieces = value_black_pieces = 0;
    winner = piece_selected = selected_square = -1;
    white_in_check = black_in_check = promoting_pawn = move_ready = false;
    game_status = 0b00000000;
    bitboards = {};
    castling_rights = 0b00001111;
    move_record.clear();
    board_record.clear();
    clear_transposition_table();
}

int handle_fen_string(Game& game) {
    std::string fen_string = game.entered_fen.toAnsiString();
    game.final_fen = game.entered_fen.toAnsiString();
    //std::cout << fen_string << '\n';
    std::vector<std::string> split_fen;
    if (fen_string.size() == 0) {
        game.final_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        fen_string = game.final_fen;
    }
    std::stringstream ss(fen_string);
    std::string section;
    while (ss >> section) {
        split_fen.push_back(section);
    }
    if (split_fen.size() != 6) {
        return -1;
    }

    if (fill_board(game, split_fen[0]) == -1) {
        return -1;
    }
    
    if (split_fen[1] == "b") {
        game.turn = BLACK;
    } else if (split_fen[1] == "w") {
        game.turn = WHITE;
    } else {
        return -1;
    }
    
    uint64_t mask = 1ULL;
    for (char letter: split_fen[2]) {    
        if (letter == 'K') {
            game.castling_rights |= (mask << 3);
        } else if (letter == 'Q') {
            game.castling_rights |= (mask << 2);
        } else if (letter == 'k') {
            game.castling_rights |= (mask << 1);
        } else if (letter == 'q') {
            game.castling_rights |= mask;
        } else if (letter == '-') {
            game.castling_rights = 0b00000000;
        } else {
            return -1;
        }  
    }
    //std::cout << std::bitset<8>(game.castling_rights);
    if (process_en_passant_square(game, split_fen[3]) == -1) {
        return -1;
    }
    game.plys_to_100 = std::stoi(split_fen[4]);
    //std::cout << "plys to 100 : " << game.plys_to_100 << '\n';
    int move_num = std::stoi(split_fen[5]);

    if (check_position_validity(game) == -1) {
        return -1;
    }
    find_position_hash(game);
    return 0;
}

int fill_board(Game& game, std::string& fen_board_section) {
    int curr_square = 56;
    int ranks = 1;
    int row_squares_recorded = 0;
    uint64_t mask = 1ULL;
    game.bitboards.init_bitboards();
    for (int i { 0 }; i < 64; i++) {
        game.board[i] = EMPTY_SQUARE;
    }

    for (char letter: fen_board_section) {
        if (letter == 'r') {
            game.board[curr_square] = BLACK_ROOK;
            game.bitboards.bitboards[BLACK_ROOK] |= mask << curr_square;
        } else if (letter == 'n') {
            game.board[curr_square] = BLACK_KNIGHT;
            game.bitboards.bitboards[BLACK_KNIGHT] |= mask << curr_square;
        } else if (letter == 'b') {
            game.board[curr_square] = BLACK_BISHOP;
            game.bitboards.bitboards[BLACK_BISHOP] |= mask << curr_square;
        } else if (letter == 'q') {
            game.board[curr_square] = BLACK_QUEEN;
            game.bitboards.bitboards[BLACK_QUEEN] |= mask << curr_square;
        } else if (letter == 'k') {
            game.board[curr_square] = BLACK_KING;
            game.bitboards.bitboards[BLACK_KING] |= mask << curr_square;
        } else if (letter == 'p') {
            game.board[curr_square] = BLACK_PAWN;
            game.bitboards.bitboards[BLACK_PAWN] |= mask << curr_square;          
        } else if (letter == 'R') {
            game.board[curr_square] = WHITE_ROOK;
            game.bitboards.bitboards[WHITE_ROOK] |= mask << curr_square;
        } else if (letter == 'N') { 
            game.board[curr_square] = WHITE_KNIGHT;
            game.bitboards.bitboards[WHITE_KNIGHT] |= mask << curr_square;
        } else if (letter == 'B') {
            game.board[curr_square] = WHITE_BISHOP;
            game.bitboards.bitboards[WHITE_BISHOP] |= mask << curr_square;
        } else if (letter == 'Q') {
            game.board[curr_square] = WHITE_QUEEN;
            game.bitboards.bitboards[WHITE_QUEEN] |= mask << curr_square;
        } else if (letter == 'K') {
            game.board[curr_square] = WHITE_KING;
            game.bitboards.bitboards[WHITE_KING] |= mask << curr_square;
        } else if (letter == 'P') {
            game.board[curr_square] = WHITE_PAWN;
            game.bitboards.bitboards[WHITE_PAWN] |= mask << curr_square;          
        } 
        //std::cout << curr_square << '\n';
        if (curr_square >= 65) {
            return -1;
        }

        if (letter >= 49 && letter <= 56) {
            int empty_squares = letter - '0';
            curr_square += empty_squares;
            row_squares_recorded += empty_squares;
        }

        if (letter == '/') {
            ranks++;
            if (row_squares_recorded != 8) {
                return -1;
            }
            int row = curr_square / 8;
            curr_square = 8 * (row - 2);
            row_squares_recorded = 0;
        }  

        if (letter > 57) {
            curr_square++;
            row_squares_recorded++;
            //std::cout << "added\n";
        }
    }
    if (row_squares_recorded != 8 || ranks != 8) {
        return -1;
    }
    game.bitboards.update_occupied();
    return 0;
}

int process_en_passant_square(Game& game, std::string& en_passant_square) {

    if (en_passant_square == "-") {
        game.en_passant_index = 8;
        return 0;
    }
    int col = en_passant_square[0] - 'a';
    int row = '8' - en_passant_square[1];
    int square = 56 - 8 * row + col;
    std::cout << square << '\n';
    if (square < 16 || (square > 23 && square < 40) || square > 47) {
        return -1;
    }
    uint64_t mask = 1ULL;
    if (mask << square & game.bitboards.occupied) {
        return -1;
    }
    
    if (square >= 16 && square <= 23) {
        if ((game.bitboards.bitboards[WHITE_PAWN] & (mask << (square + 8))) &&
            ~game.bitboards.occupied & mask << (square - 8) && game.turn == BLACK) {
            Move prev_move;
            prev_move.set_from_square(square - 8);
            prev_move.set_to_square(square + 8);
            prev_move.set_piece(WHITE_PAWN);
            prev_move.set_castling_flags(game.castling_rights);
            game.move_record.push_back(prev_move);
            game.en_passant_index = square % 8;
            return 0;
        }
    } else if (square >= 40 && square <= 47) {
        if ((game.bitboards.bitboards[BLACK_PAWN] & (mask << (square - 8))) &&
            ~game.bitboards.occupied & mask << (square + 8) && game.turn == WHITE) {
            Move prev_move;
            prev_move.set_from_square(square + 8);
            prev_move.set_to_square(square - 8);
            prev_move.set_piece(BLACK_PAWN);
            prev_move.set_castling_flags(game.castling_rights);
            game.move_record.push_back(prev_move);
            game.en_passant_index = square % 8;
            return 0;
        }
    }
    return -1;
}

int check_position_validity(Game& game) {
    if (game.bitboards.bitboards[BLACK_KING] == 0 || 
        (game.bitboards.bitboards[BLACK_KING] & (game.bitboards.bitboards[BLACK_KING] - 1)) != 0 ||
        game.bitboards.bitboards[WHITE_KING] == 0 || 
        ((game.bitboards.bitboards[WHITE_KING] & (game.bitboards.bitboards[WHITE_KING] - 1)) != 0)) {
        return -1;
    }

    int black_king_square = __builtin_ctzll(game.bitboards.bitboards[BLACK_KING]);
    int white_king_square = __builtin_ctzll(game.bitboards.bitboards[WHITE_KING]);

    if (is_square_attacked(game.bitboards, white_king_square, WHITE)) {
        game.white_in_check = true;
    }
    if (is_square_attacked(game.bitboards, black_king_square, BLACK)) {
        game.black_in_check = true;
    }
    if ((game.black_in_check && game.turn == WHITE) || 
        (game.white_in_check && game.turn == BLACK)) {
        return -1;
    }
    return 0;
}


std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << "Piece: " << move.get_piece() << ' ';
    os << "Move: " << move.get_from_square() << ' ' << move.get_to_square() << ' ';
    return os;
}

bool operator==(Move& move1, Move& move2) {
    if (move1.get_from_square() == move2.get_from_square() &&
        move1.get_to_square() == move2.get_to_square() &&
        move1.get_piece() == move2.get_piece()) {
        return true;
    }
    return false;
}

void print_board(std::array<uint8_t, 64> board) {
    for (int i { 0 }; i < 64; i++) {
        std::cout << static_cast<int>(board[i]) << ' ';
        if (( i + 1) % 8 == 0) {
            std::cout << '\n';
        }
    }
    std::cout << '\n';
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
    for (int i = 1; i <= 6; i++) {
        if (i == 1) {
            std::cout << "WHITE pawns";
        } else if (i == 2) {
            std::cout << "WHITE knights";
        } else if (i == 3) {
            std::cout << "WHITE bishops";
        } else if (i == 4) {
            std::cout << "WHITE rooks";
        } else if (i == 5) {
            std::cout << "WHITE queens";
        } else if (i == 6) {
            std::cout << "WHITE king";
        }
        std::cout << '\n';
        print_bitboard(bitboards.bitboards[i]);
    }
    for (int i = 9; i <= 14; i++) {
        if (i == 9) {
            std::cout << "BLACK pawns";
        } else if (i == 10) {
            std::cout << "BLACK knights";
        } else if (i == 11) {
            std::cout << "BLACK bishops";
        } else if (i == 12) {
            std::cout << "BLACK rooks";
        } else if (i == 13) {
            std::cout << "BLACK queens";
        } else if (i == 14) {
            std::cout << "BLACK king";
        }
        std::cout << '\n';
        print_bitboard(bitboards.bitboards[i]);
    }
}

void verify_board_sync(Game& game) {
    int piece;
    std::vector<int> bitboards_filled;
    for (int square { 0 }; square < 63; square++) {
        bitboards_filled.clear();
        piece = game.board[square];
        if (piece != EMPTY_SQUARE) {
            if (!(game.bitboards.bitboards[game.board[square]] & (1ULL << square))) {
                std::cout << "[DESYNC] Missing piece number " << piece << "at square " << square << '\n';
            }
            if (bit_filled_count(game, bitboards_filled, square) > 1) {
                std::cout << "[DESYNC] The following piece bitboards are filled at square " << square << ": ";
                for (int piece_num : bitboards_filled) {
                    std::cout << piece_num << ' ';
                }
                std::cout << "\nBut the piece is " << piece << '\n';
            }
        } else {
            if (bit_filled_count(game, bitboards_filled, square) > 0) {
                std::cout << "[DESYNC] 1D array says the board is empty at square " << square <<
                " but the following bitboards are filled: ";
                for (int piece_num : bitboards_filled) {
                    std::cout << piece_num << ' ';
                }
                std::cout << '\n';
            }
        }
    }
}

int bit_filled_count(Game& game, std::vector<int>& bitboards_filled, int square) {
    int count = 0;
    for (int i { 1 }; i < 6; i++) {
        if (game.bitboards.bitboards[i] & (1ULL << square)) {
            count++;
            bitboards_filled.push_back(i);
        }
    }
    for (int i { 9 }; i < 14; i++) {
        if (game.bitboards.bitboards[i] & (1ULL << square)) {
            count++;
            bitboards_filled.push_back(i);
        }
    }
    return count;
}

bool verify_zobrist_sync(Game& game) {

    uint64_t stored_hash = game.zobrist_hash;

    game.zobrist_hash = 0; 
    find_position_hash(game);
    uint64_t calculated_hash = game.zobrist_hash;

    game.zobrist_hash = stored_hash;

    if (calculated_hash != stored_hash) {
        std::cout << "HASH MISMATCH!\n";
        std::cout << "Stored (Incremental):   " << stored_hash << "\n";
        std::cout << "Calculated (Scratch):   " << calculated_hash << "\n";
        std::cout << "Difference (XOR):       " << (stored_hash ^ calculated_hash) << "\n";
        debug_diff(stored_hash ^ calculated_hash);
        return false;
    }
    return true;
}

void debug_diff(uint64_t diff) {
    if (diff == zobrist_black_turn) {
        std::cout << "[ERROR] Side-to-move is flipped incorrectly.\n";
        return;
    }

    for (int piece = 1; piece < 14; piece++) { 
        if (piece == 7 || piece == 8) {
            continue;
        }
        for (int square = 0; square < 64; square++) {
            if (diff == zobrist_table[piece][square]) {
                std::cout << "[ERROR] Discrepancy matches Piece-Type-" << piece 
                          << " at square " << square << "\n";
                return;
            }
        }
    }
    
    for (int col = 0; col < 9; col++) {
         if (diff == zobrist_en_passant[col]) {
             std::cout << "[ERROR] En Passant file discrepancy at col " << col << "\n";
             return;
         }
    }

    std::cout << "[ERROR] Complex desync\n";
}


