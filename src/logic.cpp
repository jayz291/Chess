#include "logic.h"
#include "movegen.h"
#include "perft.h"

uint64_t zobrist_table[16][64];
uint64_t zobrist_castling[16];
uint64_t zobrist_en_passant[9];
uint64_t zobrist_black_turn;

const int RANK_SCORES[8] = { 0, 10, 15, 20, 40, 80, 160, 0 };

void init_zobrist_table() {
    std::mt19937_64 rng(12345);
    for (int piece { 1 }; piece <= 6; piece++) {
        for (int square { 0 }; square < 64; square++) {
            zobrist_table[piece][square] = rng();
        }
    }
    for (int piece { 9 }; piece <= 14; piece++) {
        for (int square { 0 }; square < 64; square++) {
            zobrist_table[piece][square] = rng();
        }       
    }
    for (int i { 0 }; i < 16; i++) {
        zobrist_castling[i] = rng();
    }
    for (int i { 0 }; i < 9; i++) {
        zobrist_en_passant[i] = rng();
    }
    zobrist_black_turn = rng();
}

void Position::find_position_hash() {
    zobrist_hash = 0ULL;
    uint64_t occupied_board = bitboards.occupied;
    while (occupied_board) {
        
        int square = __builtin_ctzll(occupied_board);
        zobrist_hash ^= zobrist_table[board[square]][square];
        occupied_board &= occupied_board - 1;
    }
    zobrist_hash ^= zobrist_castling[castling_rights];
    zobrist_hash ^= zobrist_en_passant[en_passant_index];

    if (turn == BLACK) {
        zobrist_hash ^= zobrist_black_turn;
    }
}

template<bool update_zobrist> void Position::replace_piece(int turn, uint8_t prev_piece, 
    uint8_t new_piece, int target_square) {
    assert(board[target_square] != EMPTY_SQUARE);
    remove_piece<update_zobrist>(turn, prev_piece, target_square);
    place_piece<update_zobrist>(turn, new_piece, target_square);
}

template<bool update_zobrist> inline void Position::remove_piece(int turn, uint8_t target_piece, int target_square) {

    // update bitboards
    bitboards.bitboards[target_piece] &= ~(1ULL << target_square);
    bitboards.occupied_tables[turn] &= ~(1ULL << target_square);
    bitboards.occupied &= ~(1ULL << target_square);

    // update 1D array
    board[target_square] = EMPTY_SQUARE;

    // update zobrist hash
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_table[target_piece][target_square];
    }
}

template<bool update_zobrist> inline void Position::place_piece(int turn, uint8_t target_piece, int target_square) {
    //int zobrist_offset = (turn == WHITE) ? 0 : 6;

    // update bitboards
    bitboards.bitboards[target_piece] |= (1ULL << target_square);
    bitboards.occupied_tables[turn] |= (1ULL << target_square);
    bitboards.occupied |= (1ULL << target_square);

    // update 1D array
    board[target_square] = target_piece;

    // update zobrist hash
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_table[target_piece][target_square];
    }
}

template<bool update_zobrist> void Position::restore_zobrist_en_passant_and_castling(Move& prev_move) {
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_castling[castling_rights];
    }
    castling_rights = prev_move.get_castling_rights();
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_castling[castling_rights];
    }
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_en_passant[en_passant_index]; 
    }
    en_passant_index = prev_move.get_en_passant_index(); 
    if (en_passant_index != 8) {
        en_passant_square = (turn == BLACK) ? 
        (40 + en_passant_index) : (16 + en_passant_index);
    } else {
        en_passant_square = -1;
    }
   
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_en_passant[en_passant_index];
    }   
}

template<bool update_zobrist> void Position::update_zobrist_en_passant(Move& move) {
    move.set_en_passant_index(en_passant_index);

    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_en_passant[en_passant_index];
    }
    assert(en_passant_index >= 0 && en_passant_index <= 8);

    int from_square = move.get_from_square();
    int to_square = move.get_to_square();
    uint8_t piece = move.get_piece();

    if (std::abs(from_square - to_square) == 16 && (piece == WHITE_PAWN || piece == BLACK_PAWN)) {
        en_passant_index = ((from_square + to_square) / 2) % 8;
        en_passant_square = (piece == WHITE_PAWN) ? from_square + 8 : from_square - 8;
    } else {
        en_passant_index = 8;
        en_passant_square = -1;
    }
    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_en_passant[en_passant_index];
    }
}

template<bool update_zobrist> void Position::undo_move(Move& prev_move) {

    int from_square = prev_move.get_from_square();
    int to_square = prev_move.get_to_square();
    int turn = prev_move.get_turn();
    uint8_t move_type = prev_move.get_move_type();
    
    uint8_t original_piece = prev_move.get_piece(); 
    if (move_type == PROMOTION) {
        uint8_t piece = (prev_move.get_opposing_turn() == WHITE) ? BLACK_PAWN : WHITE_PAWN;
        uint8_t promotion_piece = convert_promotion_piece(prev_move, prev_move.get_promotion_piece());
        replace_piece<update_zobrist>(turn, promotion_piece, piece, to_square);
    }
    move_piece<update_zobrist>(prev_move.get_piece(), to_square, from_square, turn);

    if (move_type != EN_PASSANT && prev_move.get_captured_piece() != EMPTY_SQUARE) {
        place_piece<update_zobrist>(prev_move.get_opposing_turn(), prev_move.get_captured_piece(), to_square);
    } else if (move_type == EN_PASSANT) {
        int captured_square = ((turn == WHITE) ? to_square - 8 : to_square + 8);
        uint8_t captured = (turn == WHITE) ? BLACK_PAWN : WHITE_PAWN;
        place_piece<update_zobrist>(prev_move.get_opposing_turn(), captured, captured_square);
    }

    if (move_type == CASTLING) {
        int castling_row = (turn == BLACK) ? 0 : 7;
        uint8_t piece = (turn == BLACK) ? BLACK_ROOK : WHITE_ROOK;
        if (to_square - from_square == 2) {
            move_piece<update_zobrist>(piece, 56 - 8 * castling_row + 5, 56 - 8 * castling_row + 7, turn);
        } else {
            move_piece<update_zobrist>(piece, 56 - 8 * castling_row + 3, 56 - 8 * castling_row, turn);
        }
    }  
}

void Game::undo_game_move(bool is_game_over) {

    if (position.move_record.size() == 0) {
        return;
    }
    ui.selected_square = -1;

    Move prev_move; 
    if (!is_game_over) {
        prev_move = position.move_record[position.move_record.size() - 1];
    } else {
        prev_move = position.move_record[log.current_ply_num - 1];
    }
    //std::cout << game.move_record.size() - 1 << '\n';
    log.current_ply_num--;
    position.restore_zobrist_en_passant_and_castling<true>(prev_move);

    position.undo_move<true>(prev_move);
    if (!is_game_over && !position.plys_to_100_tracking.empty()) {
        position.plys_to_100 = position.plys_to_100_tracking.back();
        position.plys_to_100_tracking.pop_back();
    }
    
    // std::cout << "previous: " << std::bitset<8>(game.castling_rights) << '\n';
    if (!is_game_over) {
        if (position.board_record.size() > 0) {
            position.board_record.pop_back();
        }
        position.move_record.pop_back();
        log.notation_history.pop_back();
        int total_lines = (log.notation_history.size() + 1) / 2;
        
        if (total_lines <= 26) {
            log.history_scroll_offset = 0;
        } else {
            log.history_scroll_offset = total_lines - 26;
        }
    }

    position.turn = ((position.turn == WHITE) ? BLACK : WHITE);
    position.zobrist_hash ^= zobrist_black_turn;
    position.evaluate_king_checks();
}

void Game::make_game_move(int result, Move move, bool is_game_over) {
    Chessboard& board = position.board;
    if (!is_game_over) {
        disambiguate(move);
    }
    
    if (!is_game_over) {
        position.plys_to_100_tracking.push_back(position.plys_to_100);
        if (move.get_piece() == WHITE_PAWN || move.get_piece() == BLACK_PAWN 
            || board[move.get_to_square()] != EMPTY_SQUARE) {
            position.plys_to_100 = 0;
        } else {
            position.plys_to_100++;
        }
    }

    position.update_zobrist_en_passant<true>(move);

    if ((position.turn == BLACK && move.get_piece() == BLACK_PAWN && move.get_to_square() / 8 == 0) || 
        (position.turn == WHITE && move.get_piece() == WHITE_PAWN && move.get_to_square() / 8 == 7)) {
        ui.promoting_pawn = true;
        return;
    }

    if (move.get_from_square() != move.get_to_square()) {
        position.move_piece<true>(move.get_piece(), move.get_from_square(), move.get_to_square(), move.get_turn());
    }   

    if (result == KINGSIDE_CASTLING_MOVE || result == QUEENSIDE_CASTLING_MOVE) {
        if (result == KINGSIDE_CASTLING_MOVE && position.turn == WHITE) { 
            position.move_piece<true>(WHITE_ROOK, 7, 5, WHITE);
        } else if (result == QUEENSIDE_CASTLING_MOVE && position.turn == WHITE) {
            position.move_piece<true>(WHITE_ROOK, 0, 3, WHITE);
        } else if (result == KINGSIDE_CASTLING_MOVE && position.turn == BLACK) {
            position.move_piece<true>(BLACK_ROOK, 63, 61, BLACK);
        } else if (result == QUEENSIDE_CASTLING_MOVE && position.turn == BLACK) {
            position.move_piece<true>(BLACK_ROOK, 56, 59, BLACK);
        }
        move.set_move_type(CASTLING);
    } else if (result == EN_PASSANT_MOVE) {
        
        int captured_square = ((position.turn == WHITE) ? move.get_to_square() - 8 : move.get_to_square() + 8);
        int opposing_turn = ((position.turn == WHITE)) ? BLACK : WHITE;
        move.set_move_type(EN_PASSANT);
        move.set_captured(board[captured_square]);
        position.remove_piece<true>(opposing_turn, move.get_captured_piece(), captured_square);
    } 
    add_time_increment();

    position.turn = ((position.turn == WHITE) ? BLACK : WHITE);
    position.zobrist_hash ^= zobrist_black_turn;
    position.update_castling_flags<true>(move);
    if (!is_game_over) {
        position.move_record.push_back(move);
    }
    log.current_ply_num++;
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void Game::assess_and_make_premove_moves() {
    if (position.premoves.empty()) {
        return;
    }
    /*for (auto& move : position.premoves) {
        std::cout << "Piece: " << move.get_piece() << " From: " << move.get_from_square() << " To: "
        << move.get_to_square() << " Promotion piece: " << move.get_promotion_piece() << '\n';
    }*/
    Move move_to_consider = position.premoves.front();
    position.premoves.pop_front();
    move_to_consider.set_piece(position.board[move_to_consider.get_from_square()]);
    if (move_to_consider.get_piece() == EMPTY_SQUARE)  {
        position.premoves.clear();
        //std::cout << (int) position.board[move_to_consider.get_from_square()] << '\n';
        //std::cout << (int) move_to_consider.get_piece() << '\n';
        return;
    } else if (get_piece_colour(move_to_consider.get_piece()) != position.turn) {
        position.premoves.clear();
        return;
    }
    int to_square_piece = position.board[move_to_consider.get_to_square()];
    if (to_square_piece != EMPTY_SQUARE) {
        if (get_piece_colour(to_square_piece) == get_piece_colour(move_to_consider.get_piece())) {
            position.premoves.clear();
            return;
        } else {
            move_to_consider.set_captured(to_square_piece);
        }
    }
    int result = position.validate_move(move_to_consider);
    if (result != INVALID) {
        make_game_move(result, move_to_consider);
        if (ui.promoting_pawn) {
            move_to_consider.set_promotion_piece(ui.piece_selected);
            handle_pawn_promotion(move_to_consider);
        }
        verify_board_sync(position);
        verify_zobrist_sync(position);
        is_game_over();
    } else {
        position.premoves.clear();
    }
}

template<bool update_zobrist> void Position::update_castling_flags(Move& move) {
    uint64_t mask = 1ULL;
    move.set_castling_flags(castling_rights);
    if (update_zobrist) {
        zobrist_hash ^= zobrist_castling[castling_rights];
    }
    if (~bitboards.bitboards[BLACK_KING] & mask << 60) {
        castling_rights &= ~3;
    }
    if (~bitboards.bitboards[BLACK_ROOK] & mask << 56) {
        castling_rights &= ~mask; 
    }
    if (~bitboards.bitboards[BLACK_ROOK] & mask << 63) {
        castling_rights &= ~(mask << 1);
    }
    if (~bitboards.bitboards[WHITE_KING] & mask << 4) {
        castling_rights &= ~12;
    }
    if (~bitboards.bitboards[WHITE_ROOK] & mask << 0) {
        castling_rights &= ~(mask << 2);
    }
    if (~bitboards.bitboards[WHITE_ROOK] & mask << 7) {
        castling_rights &= ~(mask << 3); 
    }  
    if (update_zobrist) {
        zobrist_hash ^= zobrist_castling[castling_rights];
    }
}

void Game::is_game_over() {
    position.evaluate_king_checks();
    position.board_record.push_back(position.zobrist_hash);
    
    Position position_copy = position;
    MoveGen move_generator(position_copy);
    Move_list moves = move_generator.determine_possible_moves();
    //std::cout << game.plys_to_100 << '\n';
    if (!position_copy.more_moves_available(moves) || determine_repetition() || 
        determine_insufficient_material() || position.plys_to_100 == 100) {

        end_game();
        state = Gamestate::Gameover;
    }
    if (!position.move_record.empty()) {
        std::string algebreic_move = to_algebreic_notation();
        log.notation_history.push_back(algebreic_move);
        int total_lines = (log.notation_history.size() + 1) / 2;
        if (total_lines > 26) {
            log.history_scroll_offset = total_lines - 26;
        }
    }
}

bool Game::determine_insufficient_material() {
    position.value_black_pieces = 0;
    position.value_white_pieces = 0;
    for (int piece { 1 }; piece < 6; piece++) {
        position.value_white_pieces += __builtin_popcountll(position.bitboards.bitboards[piece]) * piece_values[piece];
    }
    for (int piece { 9 }; piece < 14; piece++) {
        position.value_black_pieces += __builtin_popcountll(position.bitboards.bitboards[piece]) * piece_values[piece - 8];
    }
    bool pawns_on_board = ((position.bitboards.bitboards[WHITE_PAWN] | position.bitboards.bitboards[BLACK_PAWN]) == 0) ? 
    false : true;
    if (position.value_black_pieces <= 330 && position.value_white_pieces <= 330 && !pawns_on_board) {
        result.status |= 1UL;
        std::cout << "insufficient material\n";
        return true;
    }
    return false;
}

bool Game::determine_repetition() {
    int occurrences { 1 };
    int latest_move { static_cast<int>(position.board_record.size() - 1)};
    // std::cout << occurrences << '\n';
    //std::cout << "Latest move: " << latest_move << '\n';
    for (int i { latest_move - 1 }; i >= 0; i--) {
        if (position.board_record[i] == position.board_record[latest_move]) {
            occurrences++;
        }
        if (occurrences == 3) {
            result.status |= (1UL << 1);
            return true;
        }
    }
    return false;
}

void Game::handle_pawn_promotion(Move& move, bool piece_already_selected, bool is_game_over) {

    assert(move.get_turn() == position.turn);

    position.move_piece<true>(move.get_piece(), move.get_from_square(), move.get_to_square(), move.get_turn());
    move.set_move_type(PROMOTION);

    if (!piece_already_selected) {
        move.set_promotion_piece(ui.piece_selected);
    }
    uint8_t promotion_piece = convert_promotion_piece(move, move.get_promotion_piece());

    position.replace_piece<true>(move.get_turn(), move.get_piece(), promotion_piece, move.get_to_square());

    position.bitboards.update_occupied();
    position.update_castling_flags<true>(move);
    position.zobrist_hash ^= zobrist_black_turn;
    position.turn = ((position.turn == WHITE) ? BLACK : WHITE);
    if (!is_game_over) {
        position.move_record.push_back(move);
    }
    log.current_ply_num++;
    ui.piece_selected = -1;
    ui.promoting_pawn = false;

}

void Position::evaluate_king_checks() {
    uint8_t piece = (turn == WHITE) ? WHITE_KING : BLACK_KING;
    int square = __builtin_ctzll(bitboards.bitboards[piece]);
    int new_result = is_square_attacked(square, turn);
    if (turn == WHITE) {
        white_in_check = ((new_result == 1) ? true : false);
        black_in_check = false;
    } else {
        black_in_check = ((new_result == 1) ? true : false);
        white_in_check = false;
    }
}

void Game::end_game(bool on_time) {
    result.status |= (1UL << 7);
    //game.state = Gamestate::Gameover;
    if (on_time) {
        result.status |= (1UL << 4);
        if (white_time <= 0) {
            result.winner = BLACK;
        } else {
            result.winner = WHITE;
        }
        return;
    }
    if ((result.status & (1UL << 1)) | (result.status & 1UL) || position.plys_to_100 == 100) {
        return;
    }
    if (position.turn == BLACK) {
        if (position.black_in_check) {
            result.status |= (1UL << 3); // checkmate
            result.winner = WHITE;
        } else {
            result.status |= (1UL << 2); // stalemate
        }
    } else {
        if (position.white_in_check) {
            result.status |= (1UL << 3);
            result.winner = BLACK;
        } else {
            result.status |= (1UL << 2);
        }
    }
}

template<bool update_zobrist> void Position::move_piece(uint8_t target_piece, int from_square, int to_square, int turn) {

    uint8_t captured = board[to_square];
    remove_piece<update_zobrist>(turn, target_piece, from_square);
    if (captured != EMPTY_SQUARE) {
        int opposing_turn = (turn == WHITE) ? BLACK : WHITE;
        remove_piece<update_zobrist>(opposing_turn, captured, to_square);
    }
    place_piece<update_zobrist>(turn, target_piece, to_square);
    assert(game.board[to_square] != EMPTY_SQUARE);
}

int Position::validate_move(Move& move) {
    int result { 0 };
    if (move.get_to_square() < 0 || move.get_to_square() > 63) {
        return INVALID;
    }
    uint8_t piece = move.get_piece();

    if ((move.get_from_square() != move.get_to_square()) && 
        (board[move.get_to_square()] == EMPTY_SQUARE || 
        get_piece_colour(board[move.get_to_square()]) != move.get_turn())) {
        
        if (piece == WHITE_PAWN || piece == BLACK_PAWN) {
            result = validate_pawn_move(move);
        } else if (piece == WHITE_KNIGHT || piece == BLACK_KNIGHT) {
            result = validate_knight_move(move);
        } else if (piece == WHITE_BISHOP || piece == BLACK_BISHOP) {
            result = validate_bishop_move(move);
        } else if (piece == WHITE_ROOK || piece == BLACK_ROOK) {
            result = validate_rook_move(move);
        } else if (piece == WHITE_QUEEN || piece == BLACK_QUEEN) {
            result = validate_queen_move(move);
        } else if (piece == WHITE_KING || piece == BLACK_KING) {
            result = validate_king_move(move);
        } else {
            return INVALID;
        }
        if (result >= 0) {
            Position position_copy = *this;
            if (position_copy.make_test_move<false>(move)) {
                return result;
            }
            return INVALID;
        } else {
            return INVALID;
        } 
    } else {
        return INVALID;
    }
}

int Position::validate_pawn_move(Move& move) {
    uint64_t mask = 1ULL;
    int direction = (turn == WHITE) ? 1 : -1;
    int from = move.get_from_square();
    int to = move.get_to_square();

    if (to == en_passant_square && ((to - from == 7 * direction) || 
    (to - from == 9 * direction)) && (std::abs(to % 8 - from % 8) == 1)) {
        move.set_move_type(EN_PASSANT);
        if (board[move.get_to_square() - 8 * direction] == EMPTY_SQUARE) {
            return INVALID;
        }
        return EN_PASSANT_MOVE; 
    } 

    int opposing_turn = (turn == WHITE) ? BLACK : WHITE;
    int rank = (turn == WHITE) ? 1 : 6;
    //std::cout << to - from << '\n';
    if (to - from == 8 * direction) {
        if (mask << to & ~bitboards.occupied) {
            return VALID;
        }
    } else if (to - from == 16 * direction) {
        if ((mask << to & ~bitboards.occupied) && (mask << (to - 8 * direction) & ~bitboards.occupied) &&
            mask << from & RANK_MASKS[rank]) {
            return VALID;
        }
    } else if (to - from == 7 * direction || to - from == 9 * direction) {
        if (mask << to & bitboards.occupied_tables[opposing_turn]) {
            return VALID;
        }
    }
    return INVALID;
}

int Position::validate_knight_move(Move& move) {
    u_int64_t mask = 1ULL << move.get_to_square();
    if (bitboards.knight_attacks[move.get_from_square()] & mask) {
        return VALID;
    }
    return INVALID;
}

int Position::validate_bishop_move(Move& move) {
    if (std::abs(move.get_to_square() / 8 - move.get_from_square() / 8) ==
        std::abs(move.get_to_square() % 8 - move.get_from_square() % 8)) {
        uint64_t path = bitboards.between_table[move.get_from_square()][move.get_to_square()];
        if (path & bitboards.occupied) {
            return INVALID;
        }
        return VALID;
    }
    return INVALID;
}

int Position::validate_rook_move(Move& move) {
    if ((move.get_to_square() / 8 - move.get_from_square() / 8) == 0 || (move.get_to_square() % 8 - move.get_from_square() % 8) == 0) {
        uint64_t path = bitboards.between_table[move.get_from_square()][move.get_to_square()];
        if (path & bitboards.occupied) {
            return INVALID;
        }
        return VALID;
    }
    return INVALID;
}

int Position::validate_queen_move(Move& move) {
    if (validate_rook_move(move) == VALID || validate_bishop_move(move) == VALID) {
        return VALID;
    } 
    return INVALID;
}

int Position::validate_king_move(Move& move) {
    if (bitboards.king_moves[move.get_from_square()] & 1ULL << move.get_to_square()) {
        return VALID;
    } else if (move.get_from_square() / 8 == move.get_to_square() / 8 && std::abs(move.get_to_square() - move.get_from_square()) == 2) {
        Position position_copy = *this;
        return validate_castling(move);
    }
    return INVALID;
}

inline int Position::is_square_attacked(int square, int turn) {
    //int opposing_colour = ((turn == WHITE) ? BLACK : WHITE);
    int offset = (turn == WHITE) ? 8 : 0;

    if (bitboards.knight_attacks[square] & bitboards.bitboards[WHITE_KNIGHT + offset]) {
        return 1;
    }
    if (bitboards.pawn_attacks[turn][square] & bitboards.bitboards[WHITE_PAWN + offset]) {
        return 1;
    }
    if (bitboards.king_moves[square] & bitboards.bitboards[WHITE_KING + offset]) {
        return 1;
    }
    uint64_t bishop_attacks = bitboards.get_bishop_attacks(square, bitboards.occupied);
    uint64_t rook_attacks = bitboards.get_rook_attacks(square, bitboards.occupied);
    if ((bishop_attacks & bitboards.bitboards[WHITE_BISHOP + offset]) || 
        (bishop_attacks & bitboards.bitboards[WHITE_QUEEN + offset])) {
        return 1;
    }
    if ((rook_attacks & bitboards.bitboards[WHITE_ROOK + offset]) || 
        (rook_attacks & bitboards.bitboards[WHITE_QUEEN + offset])) {
        return 1;
    }
    
    return 0;
}

int Position::validate_castling(Move& move) {

    uint64_t castle_mask_right_w = (1ULL << 5) | (1ULL << 6);
    uint64_t castle_mask_left_w = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
    uint64_t castle_mask_right_b = (1ULL << 62) | (1ULL << 61);
    uint64_t castle_mask_left_b = (1ULL << 59) | (1ULL << 58) | (1ULL << 57);
    uint8_t mask = 1ULL;

    int prev_square = move.get_from_square();
    int new_square = move.get_to_square();
    int turn = move.get_turn();
    //print_bitboard(bitboard_copy.occupied);
    if (turn == WHITE && prev_square == 4 && !is_square_attacked(4, turn)) {
        if (new_square == 2 && (mask << 2 & castling_rights)) {
            if ((bitboards.occupied & castle_mask_left_w) == 0) {
                if (!is_square_attacked(3, turn) && !is_square_attacked(2, turn)) {
                    move.set_move_type(CASTLING);
                    return QUEENSIDE_CASTLING_MOVE;
                }
            }
        } else if (new_square == 6 && (mask << 3 & castling_rights)) {
            if ((bitboards.occupied & castle_mask_right_w) == 0) {
                if (!is_square_attacked(5, turn) && !is_square_attacked(6, turn)) {
                    move.set_move_type(CASTLING);
                    return KINGSIDE_CASTLING_MOVE;
                }
            }
        }
    } else if (turn == BLACK && prev_square == 60 && !is_square_attacked(60, turn)) { 
        if (new_square == 58 && (mask & castling_rights)) {
            if ((bitboards.occupied & castle_mask_left_b) == 0) {
                if (!is_square_attacked(59, turn) && !is_square_attacked(58, turn)) {
                    move.set_move_type(CASTLING);
                    return QUEENSIDE_CASTLING_MOVE;
                }
            }
        } else if (new_square == 62 && (mask << 1 & castling_rights)) {
            if ((bitboards.occupied & castle_mask_right_b) == 0) {
                if (!is_square_attacked(61, turn) && !is_square_attacked(62, turn)) {
                    move.set_move_type(CASTLING);
                    return KINGSIDE_CASTLING_MOVE;
                }
            }
        }
    }
    return INVALID;
}

uint8_t convert_promotion_piece(const Move& move, const uint8_t& promotion_piece) {
    uint8_t converted = promotion_piece + 2;
    if (move.get_turn() == BLACK) {
        converted |= (1UL << 3);
    }
    return converted;
}

int get_piece_colour(uint8_t piece) {
    return (piece >> 3);
}

bool Position::more_moves_available(Move_list moves) {
    for (int i { 0 }; i < moves.num_moves; i++) {
        if (make_test_move<false>(moves.list[i])) {
            return true;
        }
    }
    return false;
}

template void Position::replace_piece<false>(int turn, uint8_t prev_piece, 
    uint8_t new_piece, int target_square); 
template void Position::replace_piece<true>(int turn, uint8_t prev_piece, 
    uint8_t new_piece, int target_square); 
template void Position::remove_piece<false>(int turn, uint8_t target_piece, int target_square);
template void Position::remove_piece<true>(int turn, uint8_t target_piece, int target_square);
template void Position::move_piece<false>(uint8_t target_piece, int from_square, int to_square, int turn);
template void Position::move_piece<true>(uint8_t target_piece, int from_square, int to_square, int turn);
template void Position::place_piece<false>(int turn, uint8_t target_piece, int target_square);
template void Position::place_piece<true>(int turn, uint8_t target_piece, int target_square);
template void Position::restore_zobrist_en_passant_and_castling<false>(Move& prev_move);
template void Position::restore_zobrist_en_passant_and_castling<true>(Move& prev_move);
template void Position::update_zobrist_en_passant<false>(Move& move);
template void Position::update_zobrist_en_passant<true>(Move& move);
template void Position::update_castling_flags<false>(Move& move);
template void Position::update_castling_flags<true>(Move& move);
template void Position::undo_move<false>(Move& prev_move);
template void Position::undo_move<true>(Move& prev_move);