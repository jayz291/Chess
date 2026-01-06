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
    move_record.clear();
    board_record.clear();
    clear_transposition_table();
    value_white_pieces = value_black_pieces = 0;
    plys_to_100 = 0;
    winner = piece_selected = -1;
    white_in_check = black_in_check = promoting_pawn = false;
    turn = white;
    game_status = 0b00000000;
    selected_square = -1;
    bitboards = {};
    castling_rights = 0b00001111;
    for (int i { 0 }; i < 64; i++) {
        board[i] = EMPTY_SQUARE;
    }
    for (int i { 48 }; i < 56; i++) {
        board[i] = BLACK_PAWN;
    }
    for (int i { 8 }; i < 16; i++) {
        board[i] = WHITE_PAWN;
    }
    board[56] = BLACK_ROOK;
    board[57] = BLACK_KNIGHT;
    board[58] = BLACK_BISHOP;
    board[59] = BLACK_QUEEN;
    board[60] = BLACK_KING;
    board[61] = BLACK_BISHOP;
    board[62] = BLACK_KNIGHT;
    board[63] = BLACK_QUEEN;
    board[0] = WHITE_ROOK;
    board[1] = WHITE_KNIGHT;
    board[2] = WHITE_BISHOP;
    board[3] = WHITE_QUEEN;
    board[4] = WHITE_KING;
    board[5] = WHITE_BISHOP;
    board[6] = WHITE_KNIGHT;
    board[7] = WHITE_ROOK;
}

int handle_fen_string(Game& game) {
    std::string fen_string = game.fen_string.toAnsiString();
    std::cout << fen_string << '\n';
    std::vector<std::string> split_fen;
    if (fen_string.size() == 0) {
        find_position_hash(game);
        return 0;
    }
    std::stringstream ss(fen_string);
    std::string section;
    while (ss >> section) {
        split_fen.push_back(section);
    }
    if (split_fen.size() != 6) {
        return -1;
    }

    Game proposed_game;
    proposed_game.castling_rights = 0b00000000;
    for (int i { 0 }; i < 64; i++) {
        proposed_game.board[i] = EMPTY_SQUARE;
    }
    memset(proposed_game.bitboards.bitboards, 0, sizeof(proposed_game.bitboards.bitboards));

    if (fill_board(proposed_game, split_fen[0]) == -1) {
        return -1;
    }
    
    if (split_fen[1] == "b") {
        proposed_game.turn = black;
    } else if (split_fen[1] == "w") {
        proposed_game.turn = white;
    } else {
        return -1;
    }
    
    uint64_t mask = 1ULL;
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
            return -1;
        }  
    }
    //std::cout << std::bitset<8>(proposed_game.castling_rights);
    if (process_en_passant_square(proposed_game, split_fen[3]) == -1) {
        return -1;
    }
    proposed_game.plys_to_100 = std::stoi(split_fen[4]);
    //std::cout << "plys to 100 : " << game.plys_to_100 << '\n';
    int move_num = std::stoi(split_fen[5]);
    proposed_game.view = game.view;
    proposed_game.mode = game.mode;
    proposed_game.fen_string = game.fen_string;

    if (check_position_validity(proposed_game) == -1) {
        return -1;
    }
    find_position_hash(proposed_game);
    game = proposed_game;
    game.default_position = false;
    return 0;
}

int fill_board(Game& proposed_game, std::string& fen_board_section) {
    int curr_square = 56;
    int ranks = 1;
    int row_squares_recorded = 0;
    uint64_t mask = 1ULL;

    for (char letter: fen_board_section) {
        if (letter == 'r') {
            proposed_game.board[curr_square] = BLACK_ROOK;
            proposed_game.bitboards.bitboards[BLACK_ROOK] |= mask << curr_square;
        } else if (letter == 'n') {
            proposed_game.board[curr_square] = BLACK_KNIGHT;
            proposed_game.bitboards.bitboards[BLACK_KNIGHT] |= mask << curr_square;
        } else if (letter == 'b') {
            proposed_game.board[curr_square] = BLACK_BISHOP;
            proposed_game.bitboards.bitboards[BLACK_BISHOP] |= mask << curr_square;
        } else if (letter == 'q') {
            proposed_game.board[curr_square] = BLACK_QUEEN;
            proposed_game.bitboards.bitboards[BLACK_QUEEN] |= mask << curr_square;
        } else if (letter == 'k') {
            proposed_game.board[curr_square] = BLACK_KING;
            proposed_game.bitboards.bitboards[BLACK_KING] |= mask << curr_square;
        } else if (letter == 'p') {
            proposed_game.board[curr_square] = BLACK_PAWN;
            proposed_game.bitboards.bitboards[BLACK_PAWN] |= mask << curr_square;          
        } else if (letter == 'R') {
            proposed_game.board[curr_square] = WHITE_ROOK;
            proposed_game.bitboards.bitboards[WHITE_ROOK] |= mask << curr_square;
        } else if (letter == 'N') { 
            proposed_game.board[curr_square] = WHITE_KNIGHT;
            proposed_game.bitboards.bitboards[WHITE_KNIGHT] |= mask << curr_square;
        } else if (letter == 'B') {
            proposed_game.board[curr_square] = WHITE_BISHOP;
            proposed_game.bitboards.bitboards[WHITE_BISHOP] |= mask << curr_square;
        } else if (letter == 'Q') {
            proposed_game.board[curr_square] = WHITE_QUEEN;
            proposed_game.bitboards.bitboards[WHITE_QUEEN] |= mask << curr_square;
        } else if (letter == 'K') {
            proposed_game.board[curr_square] = WHITE_KING;
            proposed_game.bitboards.bitboards[WHITE_KING] |= mask << curr_square;
        } else if (letter == 'P') {
            proposed_game.board[curr_square] = WHITE_PAWN;
            proposed_game.bitboards.bitboards[WHITE_PAWN] |= mask << curr_square;          
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
    proposed_game.bitboards.update_occupied();
    return 0;
}

int process_en_passant_square(Game& proposed_game, std::string& en_passant_square) {

    if (en_passant_square == "-") {
        proposed_game.en_passant_index = 8;
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
    if (mask << square & proposed_game.bitboards.occupied) {
        return -1;
    }
    
    if (square >= 16 && square <= 23) {
        if ((proposed_game.bitboards.bitboards[WHITE_PAWN] & (mask << (square + 8))) &&
            ~proposed_game.bitboards.occupied & mask << (square - 8) && proposed_game.turn == black) {
            Move prev_move;
            prev_move.set_from_square(square - 8);
            prev_move.set_to_square(square + 8);
            prev_move.set_piece(WHITE_PAWN);
            prev_move.set_castling_flags(proposed_game.castling_rights);
            proposed_game.move_record.push_back(prev_move);
            proposed_game.en_passant_index = square % 8;
            return 0;
        }
    } else if (square >= 40 && square <= 47) {
        if ((proposed_game.bitboards.bitboards[BLACK_PAWN] & (mask << (square - 8))) &&
            ~proposed_game.bitboards.occupied & mask << (square + 8) && proposed_game.turn == white) {
            Move prev_move;
            prev_move.set_from_square(square + 8);
            prev_move.set_to_square(square - 8);
            prev_move.set_piece(BLACK_PAWN);
            prev_move.set_castling_flags(proposed_game.castling_rights);
            proposed_game.move_record.push_back(prev_move);
            proposed_game.en_passant_index = square % 8;
            return 0;
        }
    }
    return -1;
}

int check_position_validity(Game& proposed_game) {
    if (proposed_game.bitboards.bitboards[BLACK_KING] == 0 || 
        (proposed_game.bitboards.bitboards[BLACK_KING] & (proposed_game.bitboards.bitboards[BLACK_KING] - 1)) != 0 ||
        proposed_game.bitboards.bitboards[WHITE_KING] == 0 || 
        ((proposed_game.bitboards.bitboards[WHITE_KING] & (proposed_game.bitboards.bitboards[WHITE_KING] - 1)) != 0)) {
        return -1;
    }

    int black_king_square = __builtin_ctzll(proposed_game.bitboards.bitboards[BLACK_KING]);
    int white_king_square = __builtin_ctzll(proposed_game.bitboards.bitboards[WHITE_KING]);

    if (is_square_attacked(proposed_game.bitboards, white_king_square, white)) {
        proposed_game.white_in_check = true;
    }
    if (is_square_attacked(proposed_game.bitboards, black_king_square, black)) {
        proposed_game.black_in_check = true;
    }
    if ((proposed_game.black_in_check && proposed_game.turn == white) || 
        (proposed_game.white_in_check && proposed_game.turn == black)) {
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

bool operator==(Piece& piece1, Piece& piece2) {
    if (piece1.colour == piece2.colour &&
        piece1.piece == piece2.piece) {
        return true;
    }
    return false;
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
            
            //print_bitboard(bitboards.bitboards[i][j]);
        }
    }
}

