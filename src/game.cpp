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
    castling_rights = 0b00000000;
    move_record.clear();
    en_passant_square = -1;
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
        return INVALID;
    }

    if (fill_board(game, split_fen[0]) == INVALID) {
        return INVALID;
    }
    
    if (split_fen[1] == "b") {
        game.turn = BLACK;
    } else if (split_fen[1] == "w") {
        game.turn = WHITE;
    } else {
        return INVALID;
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
            return INVALID;
        }  
    }
    //std::cout << std::bitset<8>(game.castling_rights);
    if (process_en_passant_square(game, split_fen[3]) == INVALID) {
        return INVALID;
    }
    game.plys_to_100 = std::stoi(split_fen[4]);
    //std::cout << "plys to 100 : " << game.plys_to_100 << '\n';
    int move_num = std::stoi(split_fen[5]);

    if (check_position_validity(game) == INVALID) {
        return INVALID;
    }
    find_position_hash(game);
    return VALID;
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
            return INVALID;
        }

        if (letter >= 49 && letter <= 56) {
            int empty_squares = letter - '0';
            curr_square += empty_squares;
            row_squares_recorded += empty_squares;
        }

        if (letter == '/') {
            ranks++;
            if (row_squares_recorded != 8) {
                return INVALID;
            }
            int row = curr_square / 8;
            curr_square = 8 * (row - 2);
            row_squares_recorded = 0;
        }  

        if (letter > 57) {
            curr_square++;
            row_squares_recorded++;
        }
    }
    if (row_squares_recorded != 8 || ranks != 8) {
        return INVALID;
    }
    game.bitboards.update_occupied();
    return VALID;
}

int process_en_passant_square(Game& game, std::string& en_passant_square) {

    if (en_passant_square == "-") {
        game.en_passant_index = 8;
        return VALID;
    }
    int col = en_passant_square[0] - 'a';
    int row = '8' - en_passant_square[1];
    int square = 56 - 8 * row + col;
    if (square < 16 || (square > 23 && square < 40) || square > 47) {
        return INVALID;
    }
    uint64_t mask = 1ULL;
    if (mask << square & game.bitboards.occupied) {
        return INVALID;
    }
    
    if (square >= 16 && square <= 23) {
        if ((game.bitboards.bitboards[WHITE_PAWN] & (mask << (square + 8))) &&
            ~game.bitboards.occupied & mask << (square - 8) && game.turn == BLACK) {
            Move prev_move;
            prev_move.set_move(square - 8, square + 8, WHITE_PAWN, EMPTY_SQUARE);
            prev_move.set_castling_flags(game.castling_rights);
            game.move_record.push_back(prev_move);
            game.en_passant_index = square % 8;
            game.en_passant_square = square;
            return VALID;
        }
    } else if (square >= 40 && square <= 47) {
        if ((game.bitboards.bitboards[BLACK_PAWN] & (mask << (square - 8))) &&
            ~game.bitboards.occupied & mask << (square + 8) && game.turn == WHITE) {
            Move prev_move;
            prev_move.set_move(square + 8, square - 8, BLACK_PAWN, EMPTY_SQUARE);
            prev_move.set_castling_flags(game.castling_rights);
            game.move_record.push_back(prev_move);
            game.en_passant_index = square % 8;
            game.en_passant_square = square;
            return VALID;
        }
    }
    return INVALID;
}

// ensures that:
// - there is exactly one king for each side on the board
// - no pawns on its colour's promotion rank
// - the king cannot be captured on the next turn 
int check_position_validity(Game& game) {
    if (__builtin_popcountll(game.bitboards.bitboards[BLACK_KING]) != 1 ||
        __builtin_popcountll(game.bitboards.bitboards[WHITE_KING]) != 1) {
        return INVALID;
    }
    if ((game.bitboards.bitboards[BLACK_PAWN] & RANK_MASKS[0]) ||
        (game.bitboards.bitboards[WHITE_PAWN] & RANK_MASKS[7])) {
        return INVALID;
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
        return INVALID;
    }
    return VALID;
}

std::string to_algebreic_notation(Game& game) {
    std::string s;
    Move last_move = game.move_record.back();
    int from_square = last_move.get_from_square();
    int to_square = last_move.get_to_square();
    uint8_t piece = last_move.get_piece();
    if (last_move.get_move_type() == CASTLING) {
        if (from_square - to_square > 0) {
            s = "O-O-O";
        } else {
            s = "O-O";
        }
    } else {
        int new_row = 7 - last_move.get_to_square() / 8;
        int new_col = last_move.get_to_square() % 8;
        int prev_row = 7 - last_move.get_from_square() / 8;
        int prev_col = last_move.get_from_square() % 8;
        std::string new_square_coords;
      
        new_square_coords += ('a' + new_col);
        new_square_coords += ('8' - new_row);
        if ((piece == WHITE_PAWN || piece == BLACK_PAWN) && last_move.get_captured_piece() != EMPTY_SQUARE) {
            s += ('a' + prev_col);
        } else if (piece == WHITE_KNIGHT || piece == BLACK_KNIGHT) {
            s += "N";
        } else if (piece == WHITE_BISHOP || piece == BLACK_BISHOP) {
            s += "B";
        } else if (piece == WHITE_ROOK || piece == BLACK_ROOK) {
            s += "R";
        } else if (piece == WHITE_QUEEN || piece == BLACK_QUEEN) {
            s += "Q";
        } else if (piece == WHITE_KING || piece == BLACK_KING) {
            s += "K";
        } 
        if (game.conflict) {
            if (!game.file_ambiguous) {
                s += ('a' + prev_col);
            } else if (!game.rank_ambiguous) {
                s += ('8' - prev_row);
            } else {
                s += ('a' + prev_col);
                s += ('8' - prev_row);
            }
        }
        if (last_move.get_captured_piece() != EMPTY_SQUARE) {
            s += "x";
        }
        s += new_square_coords;
        if (last_move.get_move_type() == PROMOTION) {
            s += "=";
            uint8_t promotion_piece = last_move.get_promotion_piece();
            if (promotion_piece == P_KNIGHT) {
                s += "N";
            } else if (promotion_piece == P_BISHOP) {
                s += "B";
            } else if (promotion_piece == P_ROOK) {
                s += "R";
            } else if (promotion_piece == P_QUEEN) {
                s += "Q";
            }
        }
    }
    if (game.game_status & (1ULL << 3)) {
        s += "#";
    } else if (game.black_in_check || game.white_in_check) {
        s += "+";
    }
    return s;
}

void disambiguate(Game& game, Move& move) {
    int to_square = move.get_to_square();
    int from_square = move.get_from_square();
    game.file_ambiguous = false;
    game.rank_ambiguous = false;
    game.conflict = false;
    uint8_t piece = move.get_piece();
    if (piece == BLACK_PAWN || piece == WHITE_PAWN) {
        return;
    }

    uint64_t pieces = game.bitboards.bitboards[piece];
    while (pieces) {
        int other_from_square = __builtin_ctzll(pieces);
        if (other_from_square == from_square) {
            pieces &= pieces - 1;
            continue;
        }
        Move test_move;
        test_move.set_move(other_from_square, to_square, piece, game.board[to_square]);
        if (validate_move(game, test_move) == VALID) {
            game.conflict = true;
            if (other_from_square % 8 == from_square % 8) {
                game.file_ambiguous = true;
            } else if (other_from_square / 8 == from_square / 8) {
                game.rank_ambiguous = true;
            }
        }
        pieces &= pieces - 1;
    }
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


