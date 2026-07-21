#include "game.h"
#include "logic.h"
#include "engine.h"
#include <fstream>
#include <format>

Move calculated_move;
std::atomic<bool> computer_turn { false };
std::atomic<bool> thinking_in_progress { false };
std::atomic<bool> finished { false };
int positions_searched { 0 };

Game::Game() {
    initialise();
}

void Game::initialise() {
    calculated_move = {}, 
    move_ready = false;
    ui.initialise();
    position.initialise();
    log.initialise();
    result.initialise();
    prev_time = std::chrono::steady_clock::now();
    white_time = black_time = time_settings[(int)time_control][0];
    time_increment = time_settings[(int)time_control][1];
    finished = false;  // prevent move spillover from possibly incomplete computer search
    clear_transposition_table();
    premove = false;
}

void Position::initialise() {
    plys_to_100_tracking.clear();
    move_record.clear();
    board_record.clear();
    white_in_check = black_in_check = false;
    board = {};
    bitboards = {};
    plys_to_100 = 0;
    en_passant_square = -1;
    value_white_pieces = value_black_pieces = 0;
    castling_rights = 0b00000000;
    zobrist_hash = 0ULL;
    current_move = {};
    premoves.clear();
}

void Log::initialise() {
    rank_ambiguous = file_ambiguous = conflict = false;
    first_move_filler = false;
    notation_history.clear();
    current_ply_num = 0;
    history_scroll_offset = 0;
    move_num = 1;
    panel_pos = {900.f, 58.f};
    panel_size = {180.f, 660.f};
    line_height = 25;
    max_lines_visible = panel_size.y / line_height;
}

void UI::initialise() {
    invalid_fen_position = false;
    promoting_pawn = false;
    selected_square = -1;
    piece_selected = -1;
    is_dragging = false;
    dragged_piece = EMPTY_SQUARE;
}

void Result::initialise() {
    status = 0b00000000;
    winner = -1;
}

int Game::handle_fen_string() {
    std::string fen_string = entered_fen.toAnsiString();
    final_fen = entered_fen.toAnsiString();
    //std::cout << fen_string << '\n';
    std::vector<std::string> split_fen;
    if (fen_string.size() == 0) {
        final_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        fen_string = final_fen;
    }
    std::stringstream ss(fen_string);
    std::string section;
    while (ss >> section) {
        split_fen.push_back(section);
    }
    if (split_fen.size() != 6) {
        return INVALID;
    }

    if (fill_board(split_fen[0]) == INVALID) {
        return INVALID;
    }
    
    if (split_fen[1] == "b") {
        position.turn = BLACK;
    } else if (split_fen[1] == "w") {
        position.turn = WHITE;
    } else {
        return INVALID;
    }
    
    uint64_t mask = 1ULL;
    for (char letter: split_fen[2]) {    
        if (letter == 'K') {
            position.castling_rights |= (mask << 3);
        } else if (letter == 'Q') {
            position.castling_rights |= (mask << 2);
        } else if (letter == 'k') {
            position.castling_rights |= (mask << 1);
        } else if (letter == 'q') {
            position.castling_rights |= mask;
        } else if (letter == '-') {
            position.castling_rights = 0b00000000;
        } else {
            return INVALID;
        }  
    }
    //std::cout << std::bitset<8>(game.castling_rights);
    if (process_en_passant_square(split_fen[3]) == INVALID) {
        return INVALID;
    }
    position.plys_to_100 = std::stoi(split_fen[4]);
    position.plys_to_100_tracking.push_back(position.plys_to_100);
    //std::cout << "plys to 100 : " << game.plys_to_100 << '\n';
    log.move_num = std::stoi(split_fen[5]);

    if (check_position_validity() == INVALID) {
        return INVALID;
    }
    position.find_position_hash();
    return VALID;
}

int Game::fill_board(std::string& fen_board_section) {
    int curr_square = 56;
    int ranks = 1;
    int row_squares_recorded = 0;
    uint64_t mask = 1ULL;
    position.bitboards.init_bitboards();
    for (int i { 0 }; i < 64; i++) {
        position.board[i] = EMPTY_SQUARE;
    }

    for (char letter: fen_board_section) {
        if (letter == 'r') {
            position.board[curr_square] = BLACK_ROOK;
            position.bitboards.bitboards[BLACK_ROOK] |= mask << curr_square;
        } else if (letter == 'n') {
            position.board[curr_square] = BLACK_KNIGHT;
            position.bitboards.bitboards[BLACK_KNIGHT] |= mask << curr_square;
        } else if (letter == 'b') {
            position.board[curr_square] = BLACK_BISHOP;
            position.bitboards.bitboards[BLACK_BISHOP] |= mask << curr_square;
        } else if (letter == 'q') {
            position.board[curr_square] = BLACK_QUEEN;
            position.bitboards.bitboards[BLACK_QUEEN] |= mask << curr_square;
        } else if (letter == 'k') {
            position.board[curr_square] = BLACK_KING;
            position.bitboards.bitboards[BLACK_KING] |= mask << curr_square;
        } else if (letter == 'p') {
            position.board[curr_square] = BLACK_PAWN;
            position.bitboards.bitboards[BLACK_PAWN] |= mask << curr_square;          
        } else if (letter == 'R') {
            position.board[curr_square] = WHITE_ROOK;
            position.bitboards.bitboards[WHITE_ROOK] |= mask << curr_square;
        } else if (letter == 'N') { 
            position.board[curr_square] = WHITE_KNIGHT;
            position.bitboards.bitboards[WHITE_KNIGHT] |= mask << curr_square;
        } else if (letter == 'B') {
            position.board[curr_square] = WHITE_BISHOP;
            position.bitboards.bitboards[WHITE_BISHOP] |= mask << curr_square;
        } else if (letter == 'Q') {
            position.board[curr_square] = WHITE_QUEEN;
            position.bitboards.bitboards[WHITE_QUEEN] |= mask << curr_square;
        } else if (letter == 'K') {
            position.board[curr_square] = WHITE_KING;
            position.bitboards.bitboards[WHITE_KING] |= mask << curr_square;
        } else if (letter == 'P') {
            position.board[curr_square] = WHITE_PAWN;
            position.bitboards.bitboards[WHITE_PAWN] |= mask << curr_square;          
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
    position.bitboards.update_occupied();
    return VALID;
}

int Game::process_en_passant_square(std::string& en_passant_square) {

    if (en_passant_square == "-") {
        position.en_passant_index = 8;
        if (position.turn == BLACK) {
            log.notation_history.push_back("...");
            log.first_move_filler = true;
        }
        return VALID;
    }
    int col = en_passant_square[0] - 'a';
    int row = '8' - en_passant_square[1];
    int square = 56 - 8 * row + col;
    if (square < 16 || (square > 23 && square < 40) || square > 47) {
        return INVALID;
    }
    uint64_t mask = 1ULL;
    if (mask << square & position.bitboards.occupied) {
        return INVALID;
    }
    
    if (square >= 16 && square <= 23) {
        if ((position.bitboards.bitboards[WHITE_PAWN] & (mask << (square + 8))) &&
            ~position.bitboards.occupied & mask << (square - 8) && position.turn == BLACK) {
            Move prev_move;
            prev_move.set_move(square - 8, square + 8, WHITE_PAWN, EMPTY_SQUARE);
            prev_move.set_castling_flags(position.castling_rights);
            position.move_record.push_back(prev_move);
            position.en_passant_index = square % 8;
            position.en_passant_square = square;
            log.current_ply_num++;
            if (position.turn == WHITE) {
                log.notation_history.push_back("...");
                log.first_move_filler = true;
            }
            return VALID;
        }
    } else if (square >= 40 && square <= 47) {
        if ((position.bitboards.bitboards[BLACK_PAWN] & (mask << (square - 8))) &&
            ~position.bitboards.occupied & mask << (square + 8) && position.turn == WHITE) {
            Move prev_move;
            prev_move.set_move(square + 8, square - 8, BLACK_PAWN, EMPTY_SQUARE);
            prev_move.set_castling_flags(position.castling_rights);
            position.move_record.push_back(prev_move);
            position.en_passant_index = square % 8;
            position.en_passant_square = square;
            log.current_ply_num++;
            if (position.turn == WHITE) {
                log.notation_history.push_back("...");
                log.first_move_filler = true;
            }
            return VALID;
        }
    }
    return INVALID;
}

int Game::check_position_validity() {
    if (__builtin_popcountll(position.bitboards.bitboards[BLACK_KING]) != 1 ||
        __builtin_popcountll(position.bitboards.bitboards[WHITE_KING]) != 1) {
        return INVALID;
    }
    if ((position.bitboards.bitboards[BLACK_PAWN] & RANK_MASKS[0]) ||
        (position.bitboards.bitboards[WHITE_PAWN] & RANK_MASKS[7])) {
        return INVALID;
    }

    int black_king_square = __builtin_ctzll(position.bitboards.bitboards[BLACK_KING]);
    int white_king_square = __builtin_ctzll(position.bitboards.bitboards[WHITE_KING]);

    if (position.is_square_attacked(white_king_square, WHITE)) {
        position.white_in_check = true;
    }
    if (position.is_square_attacked(black_king_square, BLACK)) {
        position.black_in_check = true;
    }
    if ((position.black_in_check && position.turn == WHITE) || 
        (position.white_in_check && position.turn == BLACK)) {
        return INVALID;
    }
    return VALID;
}

std::string Game::to_algebreic_notation() {
    std::string s;
    Move last_move = position.move_record.back();
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
        if (log.conflict) {
            if (!log.file_ambiguous) {
                s += ('a' + prev_col);
            } else if (!log.rank_ambiguous) {
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
    if (result.status & (1ULL << 3)) {
        s += "#";
    } else if (position.black_in_check || position.white_in_check) {
        s += "+";
    }
    return s;
}

void Game::disambiguate(Move& move) {
    int to_square = move.get_to_square();
    int from_square = move.get_from_square();
    log.file_ambiguous = false;
    log.rank_ambiguous = false;
    log.conflict = false;
    uint8_t piece = move.get_piece();
    if (piece == BLACK_PAWN || piece == WHITE_PAWN) {
        return;
    }

    uint64_t pieces = position.bitboards.bitboards[piece];
    while (pieces) {
        int other_from_square = __builtin_ctzll(pieces);
        if (other_from_square == from_square) {
            pieces &= pieces - 1;
            continue;
        }
        Move test_move;
        test_move.set_move(other_from_square, to_square, piece, position.board[to_square]);
        if (position.validate_move(test_move) == VALID) {
            log.conflict = true;
            if (other_from_square % 8 == from_square % 8) {
                log.file_ambiguous = true;
            } else if (other_from_square / 8 == from_square / 8) {
                log.rank_ambiguous = true;
            }
        }
        pieces &= pieces - 1;
    }
}

void Game::create_pgn() {
    const auto time = std::chrono::system_clock::now();
    std::string formatted = std::format("{:%Y.%m.%d}", time);
    std::string file_name = std::format("game_{:%Y-%m-%d_%H-%M-%S}.pgn", time);
    std::ofstream output_file(file_name);
    if (!output_file.is_open()) {
        return;
    }
    output_file << "[Event \"Chess Game\"]\n";
    output_file << "[Site \"Chess in C++ SFML\"]\n";
    output_file << "[Date \"" << formatted << "\"]\n";
    output_file << "[Round \"\"]\n";
    if (mode == Gamemode::CPUwhite) {
        output_file << "[White \"CPU\"]\n";
    } else {
        output_file << "[White \"\"]\n";
    }
    if (mode == Gamemode::CPUblack) {
        output_file << "[Black \"CPU\"]\n";
    } else {
        output_file << "[Black \"\"]\n";
    }
    if (result.winner == WHITE) {
        output_file << "[Result \"1-0\"]\n";
    } else if (result.winner == BLACK) {
        output_file << "[Result \"0-1\"]\n";
    } else {
        output_file << "[Result \"1/2-1/2\"]\n";
    }
    if (final_fen != "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
        output_file << "[SetUp \"1\"]\n";
        output_file << "[FEN \"" << final_fen << "\"]\n";
    }
    output_file << "\n";
    int start_index = 0;
    int end_index = (log.notation_history.size() + 1) / 2;
    int moves_on_line = 0;

    for (int i = start_index; i < end_index; ++i) {
        std::string line_str = std::to_string(i + log.move_num) + ".";
        output_file << line_str << " ";
        if (i * 2 < log.notation_history.size()) {
            output_file << log.notation_history[i * 2] << " ";
        } 
        if (i * 2 + 1 < log.notation_history.size()) {
            output_file << log.notation_history[i * 2 + 1] << " ";
        }
        moves_on_line++;
        if (moves_on_line == 7) {
            output_file << "\n";
            moves_on_line = 0;
        }
    }
    if (result.winner == WHITE) {
        output_file << "1-0";
    } else if (result.winner == BLACK) {
        output_file << "0-1";
    } else {
        output_file << "1/2-1/2";
    }
    output_file.close();
}

void Game::update_time() {
    auto current_time = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = current_time - prev_time;
    prev_time = current_time;
    float delta_time = elapsed.count();
    if (position.turn == WHITE) {
        white_time -= delta_time; 
        if (white_time <= 0) {
            white_time = 0;
            terminate_search = true;
            thinking_in_progress = false;
            end_game(true);
            state = Gamestate::Gameover;
        }
    } else {
        black_time -= delta_time;
        if (black_time <= 0) {
            black_time = 0;
            terminate_search = true;
            thinking_in_progress = false;
            end_game(true);
            state = Gamestate::Gameover;
        }
    }
    //std::cout << "White time: " << white_time << " Black time: " << black_time << '\n';
}

void Game::add_time_increment() {
    if (time_control == Timesetting::Untimed) {
        return;
    }
    if (position.turn == WHITE) {
        white_time += time_increment;
    } else {
        black_time += time_increment;
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

void verify_board_sync(Position& position) {
    int piece;
    std::vector<int> bitboards_filled;
    for (int square { 0 }; square < 63; square++) {
        bitboards_filled.clear();
        piece = position.board[square];
        if (piece != EMPTY_SQUARE) {
            if (!(position.bitboards.bitboards[position.board[square]] & (1ULL << square))) {
                std::cout << "[DESYNC] Missing piece number " << piece << "at square " << square << '\n';
            }
            if (bit_filled_count(position, bitboards_filled, square) > 1) {
                std::cout << "[DESYNC] The following piece bitboards are filled at square " << square << ": ";
                for (int piece_num : bitboards_filled) {
                    std::cout << piece_num << ' ';
                }
                std::cout << "\nBut the piece is " << piece << '\n';
            }
        } else {
            if (bit_filled_count(position, bitboards_filled, square) > 0) {
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

int bit_filled_count(Position& position, std::vector<int>& bitboards_filled, int square) {
    int count = 0;
    for (int i { 1 }; i < 6; i++) {
        if (position.bitboards.bitboards[i] & (1ULL << square)) {
            count++;
            bitboards_filled.push_back(i);
        }
    }
    for (int i { 9 }; i < 14; i++) {
        if (position.bitboards.bitboards[i] & (1ULL << square)) {
            count++;
            bitboards_filled.push_back(i);
        }
    }
    return count;
}

bool verify_zobrist_sync(Position& position) {

    uint64_t stored_hash = position.zobrist_hash;

    position.zobrist_hash = 0; 
    position.find_position_hash();
    uint64_t calculated_hash = position.zobrist_hash;

    position.zobrist_hash = stored_hash;

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


