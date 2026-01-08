#include "logic.h"
#include "engine.h"
#include "perft.h"
#include <iostream>

uint64_t Bitboards::knight_attacks[64];
uint64_t Bitboards::king_moves[64];
uint64_t Bitboards::pawn_attacks[2][64];
uint64_t Bitboards::pawn_moves[2][64];
uint64_t Bitboards::between_table[64][64];
uint64_t Bitboards::rook_attack_table[64][4096];
uint64_t Bitboards::bishop_attack_table[64][512];
uint64_t Bitboards::rook_masks[64];
uint64_t Bitboards::bishop_masks[64];
bool Bitboards::initialised = false;

uint64_t zobrist_table[16][64];
uint64_t zobrist_castling[16];
uint64_t zobrist_en_passant[9];
uint64_t zobrist_black_turn;

uint64_t FILE_H = 0x8080808080808080ULL;
uint64_t FILE_A = 0x0101010101010101ULL;
uint64_t FILE_B = 0x0202020202020202ULL;
uint64_t FILE_G = 0x4040404040404040ULL;
uint64_t FILE_AB = FILE_A | FILE_B;
uint64_t FILE_GH = FILE_G | FILE_H;
uint64_t RANK_2 = 0x000000000000FF00ULL;
uint64_t RANK_7 = 0x00FF000000000000ULL;
uint64_t RANK_4 = 0x00000000FF000000ULL;
uint64_t RANK_5 = 0x000000FF00000000ULL;

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

void find_position_hash(Game& game) {
    game.zobrist_hash = 0ULL;
    uint64_t occupied_board = game.bitboards.occupied;
    while (occupied_board) {
        
        int square = __builtin_ctzll(occupied_board);
        //int addition = (game.board[square] >= 8) ? 0 : 6;
        game.zobrist_hash ^= zobrist_table[game.board[square]][square];
        occupied_board &= occupied_board - 1;
    }
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];

    if (game.turn == black) {
        game.zobrist_hash ^= zobrist_black_turn;
    }
}

uint64_t find_rook_attacks(int square, uint64_t& occupied) {
    int directions[4] = {-1, 1, 8, -8};
    int curr = square;
    uint64_t attacks = 0ULL;
    for (int i { 0 }; i < 4; i++) {
        int direction = directions[i];
        curr = square;
        while (determine_square_validity(curr, direction) == true) {
            curr += direction;
            uint64_t mask = 1ULL << curr;
            attacks |= mask;
            if (occupied & mask) {
                break;
            }
        }
    }
    return attacks;
}

uint64_t find_bishop_attacks(int square, uint64_t& occupied) {
    int directions[4] = {7, -7, 9, -9};
    int curr = square;
    uint64_t attacks = 0ULL;
    for (int i { 0 }; i < 4; i++) {
        int direction = directions[i];
        curr = square;
        while (determine_square_validity(curr, direction) == true) {
            curr += direction;
            uint64_t mask = 1ULL << curr;
            attacks |= mask;
            if (occupied & mask) {
                break;
            }
        }
    }
    return attacks;
}

uint64_t get_rook_mask(int square) {
    uint64_t attacks = 0ULL;
    int row = 7 - square / 8;
    int col = square % 8;
    for (int i = row + 1; i < 7; i++) {
        attacks |= (1ULL << (56 - 8 * i + col));
    }
    for (int i = row - 1; i > 0; i--) {
        attacks |= (1ULL << (56 - 8 * i + col));
    }
    for (int i = col + 1; i < 7; i++) {
        attacks |= (1ULL << (56 - 8 * row + i));
    }
    for (int i = col - 1; i > 0; i--) {
        attacks |= (1ULL << (56 - 8 * row + i));
    }
    return attacks;
}

uint64_t get_bishop_mask(int square) {
    uint64_t attacks = 0ULL;
    int row = 7 - square / 8;
    int col = square % 8;
    for (int i = row + 1, j = col + 1; i < 7 && j < 7; i++, j++) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    for (int i = row + 1, j = col - 1; i < 7 && j > 0; i++, j--) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    for (int i = row - 1, j = col + 1; i > 0 && j < 7; i--, j++) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    for (int i = row - 1, j = col - 1; i > 0 && j > 0; i--, j--) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    return attacks;
}

uint64_t set_occupancy(int index, int num_bits, uint64_t attack_mask) {
    uint64_t occupancy = 0ULL;
    for (int i { 0 }; i < num_bits; i++) {
        int square = __builtin_ctzll(attack_mask);
        attack_mask &= attack_mask - 1;  // remove bit

        if (index & (1ULL << i)) {
            occupancy |= (1ULL << square);
        }
    }
    return occupancy;
}

uint64_t get_rook_attacks(int square, uint64_t occupancy, Bitboards& bitboards) {
    occupancy &= bitboards.rook_masks[square];
    occupancy *= bitboards.rook_magic_nums[square];
    occupancy >>= (64 - bitboards.rook_shifts[square]);
    return bitboards.rook_attack_table[square][occupancy];
}

uint64_t get_bishop_attacks(int square, uint64_t occupancy, Bitboards& bitboards) {
    occupancy &= bitboards.bishop_masks[square];
    occupancy *= bitboards.bishop_magic_nums[square];
    occupancy >>= (64 - bitboards.bishop_shifts[square]);
    return bitboards.bishop_attack_table[square][occupancy];
}

bool determine_square_validity(int square, int direction) {
    int rank = square / 8;
    int file = square % 8;
    if ((direction == 9 || direction == -7 || direction == 1) && file == 7) {
        return false;
    } else if ((direction == -9 || direction == 7 || direction == -1) && file == 0) {
        return false;
    } else if ((direction >= 7 && rank == 7) || (direction <= -7 && rank == 0)) {
        return false;
    } 
    return true; 
}

// replace piece with another piece on all 3 representations of the board
void replace_piece(Game& game, uint8_t prev_piece, uint8_t new_piece, int target_square) {
    //int zobrist_offset = (turn == white) ? 0 : 6;
   
    assert(game.board[target_square] != EMPTY_SQUARE);

    // update bitboards
    game.bitboards.bitboards[prev_piece] &= ~(1ULL << target_square);
    game.bitboards.bitboards[new_piece] |= (1ULL << target_square);

    // update 1D array
    game.board[target_square] = new_piece;

    // update zobrist hash
    game.zobrist_hash ^= zobrist_table[prev_piece][target_square];
    game.zobrist_hash ^= zobrist_table[new_piece][target_square];
}

void remove_piece(Game& game, int turn, uint8_t target_piece, int target_square) {
    //int zobrist_offset = (turn == white) ? 0 : 6;

    // update bitboards
    game.bitboards.bitboards[target_piece] &= ~(1ULL << target_square);
    game.bitboards.occupied_tables[turn] &= ~(1ULL << target_square);
    game.bitboards.occupied &= ~(1ULL << target_square);

    // update 1D array
    game.board[target_square] = EMPTY_SQUARE;

    // update zobrist hash
    game.zobrist_hash ^= zobrist_table[target_piece][target_square];
}

void place_piece(Game& game, int turn, uint8_t target_piece, int target_square) {
    //int zobrist_offset = (turn == white) ? 0 : 6;

    // update bitboards
    game.bitboards.bitboards[target_piece] |= (1ULL << target_square);
    game.bitboards.occupied_tables[turn] |= (1ULL << target_square);
    game.bitboards.occupied |= (1ULL << target_square);

    // update 1D array
    game.board[target_square] = target_piece;

    // update zobrist hash
    game.zobrist_hash ^= zobrist_table[target_piece][target_square];
}

void restore_zobrist_en_passant_and_castling(Game& game, Move& prev_move) {
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    game.castling_rights = prev_move.get_castling_rights();
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];

    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
    game.en_passant_index = prev_move.get_en_passant_index();    
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
}

void update_zobrist_en_passant(Game& game, Move& move) {
    move.set_en_passant_index(game.en_passant_index);
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
    assert(game.en_passant_index >= 0 && game.en_passant_index <= 8);

    if (std::abs(move.get_from_square() - move.get_to_square()) == 16 && (move.get_piece() == WHITE_PAWN ||
        move.get_piece() == BLACK_PAWN)) {
        game.en_passant_index = ((move.get_from_square() + move.get_to_square()) / 2) % 8;
    } else {
        game.en_passant_index = 8;
    }
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
}

void undo_move(Game& game, Move& prev_move) {

    //int opposing_turn = ((prev_move.get_turn() == white) ? black : white);
    
    uint8_t original_piece = prev_move.get_piece(); 
    if (prev_move.get_move_type() == PROMOTION) {
        uint8_t piece = (prev_move.get_opposing_turn() == white) ? BLACK_PAWN : WHITE_PAWN;
        uint8_t promotion_piece = convert_promotion_piece(prev_move, prev_move.get_promotion_piece());
        //std::cout << "promoting\n";
        replace_piece(game, promotion_piece, piece, prev_move.get_to_square());
        prev_move.set_another_piece(piece);
    }
    move_piece(game, prev_move.get_piece(),  prev_move.get_to_square(), prev_move.get_from_square(), 
    prev_move.get_turn());
    prev_move.set_another_piece(original_piece);

    if (prev_move.get_move_type() != EN_PASSANT && prev_move.get_captured_piece() != EMPTY_SQUARE) {
        place_piece(game, prev_move.get_opposing_turn(), prev_move.get_captured_piece(), prev_move.get_to_square());
    } else if (prev_move.get_move_type() == EN_PASSANT) {
        //std::cout << "en_passant\n";
        int captured_square = ((prev_move.get_turn() == white) ? prev_move.get_to_square() - 8 : 
        prev_move.get_to_square() + 8);
        uint8_t captured = (prev_move.get_turn() == white) ? BLACK_PAWN : WHITE_PAWN;
        place_piece(game, prev_move.get_opposing_turn(), captured, captured_square);
    }

    int castling_row = ((prev_move.get_turn() == black) ? 0 : 7);
    uint8_t piece = (prev_move.get_turn() == black) ? BLACK_ROOK : WHITE_ROOK;
    if (prev_move.get_move_type() == CASTLING) {
        if (prev_move.get_to_square() - prev_move.get_from_square() == 2) {
            //std::cout << std::bitset<8>(piece) << '\n';
            move_piece(game, piece, 56 - 8 * castling_row + 5, 56 - 8 * castling_row + 7, prev_move.get_turn());
            //std::cout << "undid castling\n";
        } else {
            move_piece(game, piece, 56 - 8 * castling_row + 3, 56 - 8 * castling_row, prev_move.get_turn());
        }
    }
    
}


void undo_game_move(Game& game) {
    if (game.move_record.size() == 0) {
        return;
    }
    //std::cout << "size: " << game.move_record.size() << '\n';
    Move prev_move = game.move_record[game.move_record.size() - 1];
    std::cout << game.move_record.size() - 1 << '\n';

    restore_zobrist_en_passant_and_castling(game, prev_move);

    undo_move(game, prev_move);
    if (prev_move.get_captured_piece() == EMPTY_SQUARE && prev_move.get_piece() != WHITE_PAWN &&
        prev_move.get_piece() != BLACK_PAWN) {
        if (game.plys_to_100 > 0) {
            game.plys_to_100--;
        }
    }
    
    // std::cout << "previous: " << std::bitset<8>(game.castling_rights) << '\n';
    if (game.board_record.size() > 0) {
        game.board_record.pop_back();
    }
    game.move_record.pop_back();
    game.turn = ((game.turn == white) ? black : white);
    game.zobrist_hash ^= zobrist_black_turn;
    evaluate_king_checks(game);
}

void make_game_move(Game& game, int result, Move move) {
    Chessboard& board = game.board;
    
    if (move.get_piece() == WHITE_PAWN || move.get_piece() == BLACK_PAWN || board[move.get_to_square()] != EMPTY_SQUARE) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }

    update_zobrist_en_passant(game, move);

    if ((game.turn == black && board[move.get_from_square()] == BLACK_PAWN 
        && 7 - move.get_from_square() / 8 == 6) || 
        (game.turn == white && board[move.get_from_square()] == WHITE_PAWN 
        && 7 - move.get_from_square() / 8 == 1) &&
        7 <= std::abs(move.get_to_square() - move.get_from_square()) && 
        std::abs(move.get_to_square() - move.get_from_square()) <= 9) {
            game.promoting_pawn = true;
            return;
    }

    if (move.get_from_square() != move.get_to_square()) {
        move_piece(game, move.get_piece(), move.get_from_square(), move.get_to_square(), move.get_turn());
    }   

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == white) { 
            move_piece(game, WHITE_ROOK, 7, 5, white);
        } else if (result == 2 && game.turn == white) {
            move_piece(game, WHITE_ROOK, 0, 3, white);
        } else if (result == 1 && game.turn == black) {
            move_piece(game, BLACK_ROOK, 63, 61, black);
        } else if (result == 2 && game.turn == black) {
            move_piece(game, BLACK_ROOK, 56, 59, black);
        }
        move.set_move_type(CASTLING);
    } else if (result == 3) {
        
        int captured_square = ((game.turn == white) ? move.get_to_square() - 8 : move.get_to_square() + 8);
        int opposing_turn = ((game.turn == white)) ? black : white;
        move.set_move_type(EN_PASSANT);
        move.set_captured(board[captured_square]);
        remove_piece(game, opposing_turn, move.get_captured_piece(), captured_square);
    } 

    game.turn = ((game.turn == white) ? black : white);
    game.zobrist_hash ^= zobrist_black_turn;
    update_castling_flags(game, move);
    game.move_record.push_back(move);
    
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void update_castling_flags(Game& game, Move& move) {
    uint64_t mask = 1ULL;
    Bitboards& bitboards = game.bitboards;
    move.set_castling_flags(game.castling_rights);
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    if (~bitboards.bitboards[BLACK_KING] & mask << 60) {
        game.castling_rights &= ~3;
    }
    if (~bitboards.bitboards[BLACK_ROOK] & mask << 56) {
        game.castling_rights &= ~mask; 
    }
    if (~bitboards.bitboards[BLACK_ROOK] & mask << 63) {
        game.castling_rights &= ~(mask << 1);
    }
    if (~bitboards.bitboards[WHITE_KING] & mask << 4) {
        game.castling_rights &= ~12;
    }
    if (~bitboards.bitboards[WHITE_ROOK] & mask << 0) {
        game.castling_rights &= ~(mask << 2);
    }
    if (~bitboards.bitboards[WHITE_ROOK] & mask << 7) {
        game.castling_rights &= ~(mask << 3); 
    }  
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void is_game_over(Game& game) {
    evaluate_king_checks(game);
    game.board_record.push_back(game.zobrist_hash);
    //record_board(game);
    
    Game game_copy = game;
    Move_list moves = determine_possible_moves(game_copy);
    std::cout << "is game over\n";
    //std::cout << moves.size() << '\n';
    //std::cout << "plys to 100: " << game.plys_to_100 << '\n';
    if (moves.num_moves > 0) {
        std::cout << moves.num_moves << '\n';
    }
    if (moves.num_moves == 0 || determine_repetition(game) || 
        determine_insufficient_material(game) || game.plys_to_100 == 100) {
    
        //std::cout << std::bitset<8>(game.game_status) << '\n';
        //std::cout << game.plys_to_100 << '\n';
        std::cout << "ending game\n";
        std::cout << "moves possible: " << moves.num_moves << '\n';
        end_game(game);
    }
}

bool determine_insufficient_material(Game& game) {
    game.value_black_pieces = 0;
    game.value_white_pieces = 0;
    for (int piece { 1 }; piece < 6; piece++) {
        game.value_white_pieces += __builtin_popcountll(game.bitboards.bitboards[piece]) * piece_values[piece];
    }
    for (int piece { 9 }; piece < 14; piece++) {
        game.value_black_pieces += __builtin_popcountll(game.bitboards.bitboards[piece]) * piece_values[piece];
    }
    bool pawns_on_board = ((game.bitboards.bitboards[WHITE_PAWN] | game.bitboards.bitboards[BLACK_PAWN]) == 0) ? 
    false : true;
    if (game.value_black_pieces <= 300 && game.value_white_pieces <= 300 && !pawns_on_board) {
        game.game_status |= 1UL;
        std::cout << "insufficient material\n";
        return true;
    }
    return false;
}

bool determine_repetition(Game& game) {
    int occurrences { 1 };
    int latest_move { static_cast<int>(game.board_record.size() - 1)};
    std::cout << occurrences << '\n';
    //std::cout << "Latest move: " << latest_move << '\n';
    for (int i { latest_move - 1 }; i >= 0; i--) {
        if (game.board_record[i] == game.board_record[latest_move]) {
            occurrences++;
        }
        if (occurrences == 3) {
            std::cout << "Occurences: " << occurrences << '\n';
            game.game_status |= (1UL << 1);
            std::cout << "I am in here now\n";
            return true;
        }
    }
    return false;
}

void handle_pawn_promotion(Game& game, Move& move) {

    assert(move.get_turn() == game.turn);

    move_piece(game, move.get_piece(), move.get_from_square(), move.get_to_square(), move.get_turn());
    //u_int64_t square = (1ULL << move.get_to_square())
    move.set_move_type(PROMOTION);
    move.set_promotion_piece(game.piece_selected);
    uint8_t promotion_piece = convert_promotion_piece(move, move.get_promotion_piece());

    replace_piece(game, move.get_piece(), promotion_piece, move.get_to_square());

    game.bitboards.update_occupied();
    update_castling_flags(game, move);
    game.zobrist_hash ^= zobrist_black_turn;
    game.turn = ((game.turn == white) ? black : white);
    game.move_record.push_back(move);
    game.piece_selected = -1;
    game.promoting_pawn = false;

}

void evaluate_king_checks(Game& game) {
    uint8_t piece = (game.turn == white) ? WHITE_KING : BLACK_KING;
    int square = __builtin_ctzll(game.bitboards.bitboards[piece]);
    int new_result = is_square_attacked(game.bitboards, square, game.turn);
    if (game.turn == white) {
        game.white_in_check = ((new_result == 1) ? true : false);
    } else {
        game.black_in_check = ((new_result == 1) ? true : false);
    }
}

void end_game(Game& game) {
    //std::cout << "It is over\n";
    game.game_status |= (1UL << 7);
    game.state = Gamestate::Gameover;
    if ((game.game_status & (1UL << 1)) | (game.game_status & 1UL) || game.plys_to_100 == 100) {
        return;
    }
    if (game.turn == black) {
        if (game.black_in_check) {
            game.game_status |= (1UL << 3); // checkmate
            game.winner = white;
        } else {
            game.game_status |= (1UL << 2); // stalemate
        }
    } else {
        if (game.white_in_check) {
            game.game_status |= (1UL << 3);
            game.winner = black;
        } else {
            game.game_status |= (1UL << 2);
        }
    }
}

void move_piece(Game& game, uint8_t target_piece, int from_square, int to_square, int turn) {

    //int piece = move.get_piece();
    uint8_t captured { EMPTY_SQUARE };
    captured = game.board[to_square];

    game.board[to_square] = game.board[from_square];
    game.board[from_square] = EMPTY_SQUARE;

    game.zobrist_hash ^= zobrist_table[target_piece][from_square];
    game.zobrist_hash ^= zobrist_table[target_piece][to_square];

    uint64_t from_bit = 1ULL << from_square; 
    uint64_t to_bit = 1ULL << to_square;
    int opposing_turn = ((turn == white) ? black : white);

    game.bitboards.bitboards[target_piece] ^= (from_bit | to_bit);
    game.bitboards.occupied_tables[turn] ^= (from_bit | to_bit);
 
    if (captured != EMPTY_SQUARE) { 
        game.bitboards.bitboards[captured] &= ~to_bit;
        game.bitboards.occupied_tables[opposing_turn] &= ~to_bit;
        game.zobrist_hash ^= zobrist_table[captured][to_square];
    }
    
    game.bitboards.occupied = game.bitboards.occupied_tables[black] | game.bitboards.occupied_tables[white];
    //bitboards.update_occupied();
    //print_bitboard(bitboards.occupied);
    assert(game.board[to_square] != EMPTY_SQUARE);
    /*
    if (board[move.get_to_square()].piece == none) {
        //print_all_bitboards(bitboards);
        std::cout << move << '\n';
        std::cout << "failed\n";
        return;
    }*/
}

int validate_move(Game& game, Move& move) {
    int result { 0 };
    if (move.get_to_square() < 0 || move.get_to_square() > 63) {
        return -1;
    }

    if ((move.get_from_square() != move.get_to_square()) && 
        (game.board[move.get_to_square()] == EMPTY_SQUARE || 
        get_piece_colour(game.board[move.get_to_square()]) != move.get_turn())) {
        
        if (move.get_piece() == WHITE_PAWN || move.get_piece() == BLACK_PAWN) {
            result = validate_pawn_move(game, move);
        } else if (move.get_piece() == WHITE_KNIGHT || move.get_piece() == BLACK_KNIGHT) {
            result = validate_knight_move(game.bitboards, move);
        } else if (move.get_piece() == WHITE_BISHOP || move.get_piece() == BLACK_BISHOP) {
            result = validate_bishop_move(game.bitboards, move);
        } else if (move.get_piece() == WHITE_ROOK || move.get_piece() == BLACK_ROOK) {
            result = validate_rook_move(game.bitboards, move);
        } else if (move.get_piece() == WHITE_QUEEN || move.get_piece() == BLACK_QUEEN) {
            result = validate_queen_move(game.bitboards, move);
        } else if (move.get_piece() == WHITE_KING || move.get_piece() == BLACK_KING) {
            result = validate_king_move(game, move);
        } else {
            return 0;
        }
        if (result >= 0) {
            Game game_copy = game;
            if (!is_in_check(game_copy, move)) {
                return result;
            }
            return -1;
        } else if (result != 0) {
            return -1;
        } else {
            return 0;
        }
    } else {
        //std::cout << "ruledinvalid\n";
        return -1;
    }
}

int validate_pawn_move(Game& game, Move& move) {
    int square = move.get_to_square();
    Bitboards& bitboards = game.bitboards;
    uint64_t mask = 1ULL;

    if (move.get_turn() == white && 32 <= move.get_from_square() && move.get_from_square() <= 39 
        && std::abs(move.get_to_square() % 8 - move.get_from_square() % 8) == 1 && 
        (std::abs(move.get_to_square() - move.get_from_square()) == 7 || 
        std::abs(move.get_to_square() - move.get_from_square()) == 9) && (~bitboards.occupied & mask << square)) {
        return validate_en_passant(game, move);
    } else if (move.get_turn() == black && 24 <= move.get_from_square() && move.get_from_square() <= 31 
        && std::abs(move.get_to_square() % 8 - move.get_from_square() % 8) == 1 && 
        (std::abs(move.get_to_square() - move.get_from_square()) == 7 || 
        std::abs(move.get_to_square() - move.get_from_square()) == 9)&& (~bitboards.occupied & mask << square)) {
        return validate_en_passant(game, move);  
    }

    int from = move.get_from_square();
    int to = move.get_to_square();
    //std::cout << to - from << '\n';
    if (game.turn == white) {
        if (to - from == 8) {
            if (mask << to & ~bitboards.occupied) {
                return 0;
            }
        } else if (to - from == 16) {
            if ((mask << to & ~bitboards.occupied) && (mask << (to - 8) & ~bitboards.occupied) &&
                mask << from & RANK_2) {
                return 0;
            }
        } else if (to - from == 7 || to - from == 9) {
            if (mask << to & bitboards.occupied_tables[black]) {
                return 0;
            }
        }
        return -1;
    } else {
        if (to - from == -8) {
            if (mask << to & ~bitboards.occupied) {
                return 0;
            }
        } else if (to - from == -16) {
            if ((mask << to & ~bitboards.occupied) && (mask << (to + 8) & ~bitboards.occupied) && 
                mask << from & RANK_7) {
                return 0;
            }
        } else if (to - from == -7 || to - from == -9) {
            if (mask << to & bitboards.occupied_tables[white]) {
                return 0;
            }
        }
        return -1;
    }
}

int validate_en_passant(Game& game, Move& move) {
    //std::cout << "here\n";
    int captured_square = ((move.get_turn() == white) ? move.get_to_square() - 8 : move.get_to_square() + 8);
    uint64_t mask = 1ULL;
    int piece = ((move.get_turn() == white) ? BLACK_PAWN : WHITE_PAWN);
   
    if (game.bitboards.bitboards[piece] & mask << captured_square) {
        Move prev_move = game.move_record[game.move_record.size() - 1];
 
        if ((prev_move.get_piece() == WHITE_PAWN || prev_move.get_piece() == BLACK_PAWN)
         && prev_move.get_to_square() == captured_square && 
            std::abs(prev_move.get_to_square() - prev_move.get_from_square()) == 16) {
            return 3;
        }
        return -1;
    }
    return -1;
}

int validate_knight_move(Bitboards& bitboards, Move& move) {
    u_int64_t mask = 1ULL << move.get_to_square();
    if (bitboards.knight_attacks[move.get_from_square()] & mask) {
        return 0;
    }
    return -1;
}

int validate_bishop_move(Bitboards& bitboards, Move& move) {
    if (std::abs(move.get_to_square() / 8 - move.get_from_square() / 8) ==
        std::abs(move.get_to_square() % 8 - move.get_from_square() % 8)) {
        uint64_t path = bitboards.between_table[move.get_from_square()][move.get_to_square()];
        if (path & bitboards.occupied) {
            return -1;
        }
        return 0;
    }
    return -1;
}

int validate_rook_move(Bitboards& bitboards, Move& move) {
    if ((move.get_to_square() / 8 - move.get_from_square() / 8) == 0 || (move.get_to_square() % 8 - move.get_from_square() % 8) == 0) {
        uint64_t path = bitboards.between_table[move.get_from_square()][move.get_to_square()];
        if (path & bitboards.occupied) {
            return -1;
        }
        return 0;
    }
    return -1;
}

int validate_queen_move(Bitboards& bitboards, Move& move) {
    if (validate_rook_move(bitboards, move) == 0 || validate_bishop_move(bitboards, move) == 0) {
        return 0;
    } 
    return -1;
}

int validate_king_move(Game &game, Move& move) {
    if (game.bitboards.king_moves[move.get_from_square()] & 1ULL << move.get_to_square()) {
        return 0;
    } else if (move.get_from_square() / 8 == move.get_to_square() / 8 && std::abs(move.get_to_square() - move.get_from_square()) == 2) {
        Bitboards bitboard_copy = game.bitboards;
        return validate_castling(game, move);
    }
    return -1;
}

int is_square_attacked(Bitboards& bitboard_copy, int square, int turn) {
    //int opposing_colour = ((turn == white) ? black : white);
    int offset = (turn == white) ? 8 : 0;

    if (bitboard_copy.knight_attacks[square] & bitboard_copy.bitboards[WHITE_KNIGHT + offset]) {
        return 1;
    }
    if (bitboard_copy.pawn_attacks[turn][square] & bitboard_copy.bitboards[WHITE_PAWN + offset]) {
        return 1;
    }
    if (bitboard_copy.king_moves[square] & bitboard_copy.bitboards[WHITE_KING + offset]) {
        return 1;
    }
    uint64_t bishop_attacks = get_bishop_attacks(square, bitboard_copy.occupied, bitboard_copy);
    uint64_t rook_attacks = get_rook_attacks(square, bitboard_copy.occupied, bitboard_copy);
    if ((bishop_attacks & bitboard_copy.bitboards[WHITE_BISHOP + offset]) || 
        (bishop_attacks & bitboard_copy.bitboards[WHITE_QUEEN + offset])) {
        return 1;
    }
    if ((rook_attacks & bitboard_copy.bitboards[WHITE_ROOK + offset]) || 
        (rook_attacks & bitboard_copy.bitboards[WHITE_QUEEN + offset])) {
        return 1;
    }
    
    return 0;
}

int validate_castling(Game &game, Move& move) {

    uint64_t castle_mask_right_w = (1ULL << 5) | (1ULL << 6);
    uint64_t castle_mask_left_w = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
    uint64_t castle_mask_right_b = (1ULL << 62) | (1ULL << 61);
    uint64_t castle_mask_left_b = (1ULL << 59) | (1ULL << 58) | (1ULL << 57);
    uint8_t mask = 1ULL;
    Bitboards& bitboards = game.bitboards;
    int prev_square = move.get_from_square();
    int new_square = move.get_to_square();
    //print_bitboard(bitboard_copy.occupied);
    if (move.get_turn() == white && prev_square == 4 && !is_square_attacked(bitboards, 4, move.get_turn())) {
        if (new_square == 2 && (mask << 2 & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_left_w) == 0) {
                if (!is_square_attacked(bitboards, 3, move.get_turn()) && !is_square_attacked(bitboards, 2, move.get_turn())) {
                    return 2;
                }
            }
        } else if (new_square == 6 && (mask << 3 & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_right_w) == 0) {
                if (!is_square_attacked(bitboards, 5, move.get_turn()) && !is_square_attacked(bitboards, 6, move.get_turn())) {
                    return 1;
                }
            }
        }
    } else if (move.get_turn() == black && prev_square == 60 && !is_square_attacked(bitboards, 60, move.get_turn())) { 
        if (new_square == 58 && (mask & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_left_b) == 0) {
                if (!is_square_attacked(bitboards, 59, move.get_turn()) && !is_square_attacked(bitboards, 58, move.get_turn())) {
                    return 2;
                }
            }
        } else if (new_square == 62 && (mask << 1 & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_right_b) == 0) {
                if (!is_square_attacked(bitboards, 61, move.get_turn()) && !is_square_attacked(bitboards, 62, move.get_turn())) {
                    return 1;
                }
            }
        }
    }
    return -1;
}

int is_in_check(Game& game, Move& move) {
    int king = (move.get_turn() == white) ? WHITE_KING : BLACK_KING;
    make_test_move(game, move); 
    int king_square = __builtin_ctzll(game.bitboards.bitboards[king]);
    if (king_square > 63) {
        print_all_bitboards(game.bitboards);
        exit(2);
    }
    int in_check = is_square_attacked(game.bitboards, king_square, move.get_turn());
    undo_test_move(game, move);
    return in_check;
}

uint8_t convert_promotion_piece(Move& move, const uint8_t& promotion_piece) {
    uint8_t converted = promotion_piece + 2;
    if (move.get_turn() == black) {
        converted |= (1UL << 3);
    }
    //std::cout << "Converted: " << std::bitset<8>(converted) << '\n';
    return converted;
}

int get_piece_colour(uint8_t piece) {
    return (piece >> 3);
}




