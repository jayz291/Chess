#include "logic.h"
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

uint64_t zobrist_table[12][64];
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
    for (int piece { 0 }; piece < 12; piece++) {
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
    uint64_t occupied_board = game.bitboards.occupied;
    while (occupied_board) {
        int square = __builtin_ctzll(occupied_board);
        int addition = (game.board[square].piece_occupying.colour == white) ? 0 : 6;
        game.zobrist_hash ^= zobrist_table[game.board[square].piece_occupying.piece_type + addition][square];
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
void replace_piece(Game& game, int turn, int prev_piece, int new_piece, int target_square) {
    int zobrist_offset = (turn == white) ? 0 : 6;
   
    assert(game.board[target_square].piece_occupying.piece_type != none);

    // update bitboards
    game.bitboards.bitboards[turn][prev_piece] &= ~(1ULL << target_square);
    game.bitboards.bitboards[turn][new_piece] |= (1ULL << target_square);

    // update 1D array
    game.board[target_square].piece_occupying.piece_type = new_piece;

    // update zobrist hash
    game.zobrist_hash ^= zobrist_table[prev_piece + zobrist_offset][target_square];
    game.zobrist_hash ^= zobrist_table[new_piece + zobrist_offset][target_square];
}

void undo_move(Game& game, Move& prev_move, int turn) {
    Bitboards& bitboards = game.bitboards;
    Chessboard& board = game.board;
    int opposing_turn = ((prev_move.turn == white) ? black : white);
    int current = (prev_move.turn == white) ? 0 : 6;
    int opposing = (prev_move.turn == white) ? 6 : 0;
    switch_move(prev_move);
    
    //int promoted_piece = queen;
    int original_piece = prev_move.piece; 
    if (prev_move.special_move == promotion) {

        replace_piece(game, prev_move.turn, prev_move.promoted_piece, pawn, prev_move.prev_square);
        prev_move.piece = pawn;
    }

    move_piece(game, prev_move, true);
    prev_move.piece = original_piece;

    if (prev_move.special_move != en_passant && prev_move.piece_taken.piece_type != none) {
        board[prev_move.prev_square].piece_occupying = prev_move.piece_taken;
        int from_square = prev_move.prev_square;
        uint64_t mask = (1ULL << from_square);
        bitboards.bitboards[opposing_turn][prev_move.piece_taken.piece_type] |= mask;
        bitboards.occupied_tables[opposing_turn] |= mask;
        bitboards.occupied |= mask;

        game.zobrist_hash ^= zobrist_table[prev_move.piece_taken.piece_type + opposing][from_square];
        //bitboards.update_occupied();
    }
 
    if (prev_move.special_move == en_passant) {
        int captured_square = ((prev_move.turn == white) ? prev_move.prev_square - 8 : prev_move.prev_square + 8);
        board[captured_square].piece_occupying = prev_move.piece_taken;
        assert(prev_move.piece_taken.piece_type == pawn);
        uint64_t mask = 1ULL;

        bitboards.bitboards[opposing_turn][pawn] |= (mask << captured_square);
        bitboards.occupied_tables[opposing_turn] |= (mask << captured_square);
        bitboards.occupied |= (mask << captured_square);
        game.zobrist_hash ^= zobrist_table[pawn + opposing][captured_square];
        //bitboards.update_occupied();
    }
    int castling_row = ((prev_move.turn == black) ? 0 : 7);
    if (prev_move.special_move == castling) {
        if (prev_move.prev_square - prev_move.new_square == 2) {
            Move move { 56 - 8 * castling_row + 5, 56 - 8 * castling_row + 7, prev_move.turn, rook };
            move_piece(game, move, true);
        } else {
            Move move { 56 - 8 * castling_row + 3, 56 - 8 * castling_row, prev_move.turn, rook };
            move_piece(game, move, true);
        }
    }
    
    switch_move(prev_move);
}


void undo_game_move(Game& game) {
    if (game.move_record.size() == 0) {
        return;
    }
    //std::cout << "size: " << game.move_record.size() << '\n';
    Move prev_move = game.move_record[game.move_record.size() - 1];
    std::cout << game.move_record.size() - 1 << '\n';
    int turn = (((game.move_record.size() - 1) % 2 == 0) ? black : white); 
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    game.castling_rights = prev_move.castling_rights;
    // std::cout << std::bitset<8>(prev_move.castling_rights) << '\n';
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];

    undo_move(game, prev_move, turn);
    if (prev_move.piece_taken.piece_type == none && prev_move.piece != pawn) {
        if (game.plys_to_100 > 0) {
            game.plys_to_100--;
        }
    }
  
    //std::cout << "Prev index: " << game.en_passant_index << '\n';
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
    game.en_passant_index = prev_move.en_passant_index;    
    assert(game.en_passant_index >= 0 && game.en_passant_index <= 8);
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
    //std::cout << "New index: " << game.en_passant_index << '\n';  
    
    // std::cout << "previous: " << std::bitset<8>(game.castling_rights) << '\n';
    if (game.board_record.size() > 0) {
        game.board_record.pop_back();
    }
    game.move_record.pop_back();
    game.turn = ((game.turn == white) ? black : white);
    game.zobrist_hash ^= zobrist_black_turn;
    evaluate_king_checks(game);
}

void undo_test_move(Game& game, Move& prev_move) {

    undo_move(game, prev_move, prev_move.turn);
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    game.castling_rights = prev_move.castling_rights;
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];

    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
    game.en_passant_index = prev_move.en_passant_index;
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];

    if (!game.move_record.empty()) {
        game.move_record.pop_back();
    }

    game.turn = ((game.turn == black) ? white : black);
    game.zobrist_hash ^= zobrist_black_turn;
}

void make_game_move(Game& game, int result, Move move) {
    Chessboard& board = game.board;
    Bitboards& bitboards = game.bitboards;
    
    int opposing = (game.turn == white) ? 6 : 0;
    //std::cout << "moving\n";
    if (move.piece == pawn || board[move.new_square].piece_occupying.piece_type != none) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }

    move.en_passant_index = game.en_passant_index;
    //int old_en_passant_index = (game.en_passant_square != -1) ? (game.en_passant_square % 8) : 8;
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];
    assert(game.en_passant_index >= 0 && game.en_passant_index <= 8);

    if (std::abs(move.prev_square - move.new_square) == 16 && move.piece == pawn) {
        game.en_passant_index = ((move.prev_square + move.new_square) / 2) % 8;
    } else {
        game.en_passant_index = 8;
    }
    int new_en_passant_index = game.en_passant_index;
    game.zobrist_hash ^= zobrist_en_passant[new_en_passant_index];

    if ((game.turn == black && board[move.prev_square].piece_occupying.piece_type == pawn 
        && 7 - move.prev_square / 8 == 6) || 
        (game.turn == white && board[move.prev_square].piece_occupying.piece_type == pawn 
        && 7 - move.prev_square / 8 == 1) &&
        7 <= std::abs(move.new_square - move.prev_square) && std::abs(move.new_square - move.prev_square) <= 9) {
            game.promoting_pawn = true;
            return;
    }

    if (move.prev_square != move.new_square) {
        move_piece(game, move);
    }   

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == white) { 
            Move move { 7, 5, white, rook };
            move_piece(game, move);
        } else if (result == 2 && game.turn == white) {
            Move move { 0, 3, white, rook };
            move_piece(game, move);
        } else if (result == 1 && game.turn == black) {
            Move move { 63, 61, black, rook };
            move_piece(game, move);
        } else if (result == 2 && game.turn == black) {
            Move move { 56, 59, black, rook };
            move_piece(game, move);
        }
        game.board_record.clear();
        move.special_move = castling;
    } else if (result == 3) {
        
        int captured_square = ((game.turn == white) ? move.new_square - 8 : move.new_square + 8);
        int opposing_turn = ((game.turn == white)) ? black : white;
   
        u_int64_t mask = (1ULL << captured_square);
        move.special_move = en_passant;
        move.piece_taken = board[captured_square].piece_occupying;
        board[captured_square].piece_occupying = { none, none};
        bitboards.bitboards[opposing_turn][pawn] &= (~mask);
        game.zobrist_hash ^= zobrist_table[pawn + opposing][captured_square];
        //Square: " << captured_square << '\n';
        bitboards.update_occupied();
        game.board_record.clear();
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
    move.castling_rights = game.castling_rights;
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    if (~bitboards.bitboards[black][king] & mask << 60) {
        game.castling_rights &= ~3;
    }
    if (~bitboards.bitboards[black][rook] & mask << 56) {
        game.castling_rights &= ~mask; 
    }
    if (~bitboards.bitboards[black][rook] & mask << 63) {
        game.castling_rights &= ~(mask << 1);
    }
    if (~bitboards.bitboards[white][king] & mask << 4) {
        game.castling_rights &= ~12;
    }
    if (~bitboards.bitboards[white][rook] & mask << 0) {
        game.castling_rights &= ~(mask << 2);
    }
    if (~bitboards.bitboards[white][rook] & mask << 7) {
        game.castling_rights &= ~(mask << 3); 
    }  
    game.zobrist_hash ^= zobrist_castling[game.castling_rights];
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void make_test_move(Game& game, Move& move) {
    Chessboard& board = game.board;
    Bitboards& bitboards = game.bitboards;
    
    int current = (game.turn == white) ? 0 : 6;
    int opposing = (game.turn == white) ? 6 : 0;

    assert(board[move.prev_square].piece_occupying.piece_type != none);
    //std::cout << "processing move\n";

    /*if (move.piece == pawn || board[move.new_square].piece_occupying.piece_type != none) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }*/
    move.en_passant_index = game.en_passant_index;
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];

    if (std::abs(move.prev_square - move.new_square) == 16 && move.piece == pawn) {
        game.en_passant_index = ((move.prev_square + move.new_square) / 2) % 8;
    } else {
        game.en_passant_index = 8;
    }
    game.zobrist_hash ^= zobrist_en_passant[game.en_passant_index];

    if (move.special_move == promotion) {
        //std::cout << "promoting pawn\n";
        game.promoting_pawn = true;
        move_piece(game, move);
        replace_piece(game, move.turn, pawn, move.promoted_piece, move.new_square);

        update_castling_flags(game, move);
        game.move_record.push_back(move);
        game.piece_selected = -1;
        game.promoting_pawn = false;
        game.turn = white + black - game.turn; // flip the turn
        game.zobrist_hash ^= zobrist_black_turn;
        return;
    }

    //std::cout << "just before moving piece\n";
    assert(move.prev_square != move.new_square);
    move_piece(game, move);

    if (move.special_move == castling) {
        int row = 7 - move.new_square / 8;
        if (move.new_square - move.prev_square == 2) {
            Move rook_move { 56 - 8 * row + 7, 56 - 8 * row + 5, move.turn, rook };
            move_piece(game, rook_move);
        } else if (move.new_square - move.prev_square == -2) {
            Move rook_move { 56 - 8 * row, 56 - 8 * row + 3, move.turn, rook };
            move_piece(game, rook_move);
        }
    }

    if (move.special_move == en_passant) {

        int captured_square = ((game.turn == white) ? move.new_square - 8 : move.new_square + 8);
        int opposing_turn = ((game.turn == white)) ? black : white;
        u_int64_t mask = (1ULL << captured_square);
        move.piece_taken = board[captured_square].piece_occupying;
        board[captured_square].piece_occupying = { none, none};
        bitboards.bitboards[opposing_turn][pawn] &= (~mask);
        bitboards.occupied_tables[opposing_turn] &= (~mask);
        bitboards.occupied &= (~mask);
        
        game.zobrist_hash ^= zobrist_table[pawn + opposing][captured_square];
    } 
    update_castling_flags(game, move);
    game.move_record.push_back(move);
    game.turn = ((game.turn == white) ? black : white);
    game.zobrist_hash ^= zobrist_black_turn;
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void generate_computer_move(Game& game) {
    if (thinking_in_progress) {
        return;
    }
    thinking_in_progress = true;
    
    positions_searched = 0;
    Game game_copy = game;
    auto start = std::chrono::steady_clock::now();
    std::thread computer_thread([game_copy, start]() mutable {
        Move chosen_move = get_best_move(game_copy, 6);
        computer_turn = false;
        thinking_in_progress = false;
        finished = true;
        calculated_move = chosen_move;
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        std::cout << duration.count() << '\n';
        std::cout << "Avg nodes per sec: " << positions_searched / duration.count() * 1000 << '\n';
    });
    computer_thread.detach();
}

void make_computer_move(Game& game) {
    if (finished) {
        Move chosen_move = calculated_move;
        std::cout << "best move calculated: ";
        std::cout << chosen_move.prev_square << " -> " << chosen_move.new_square << '\n';
        if (game.board[chosen_move.new_square].piece_occupying.piece_type != none) {
            chosen_move.piece_taken = game.board[chosen_move.new_square].piece_occupying;
        }
        int result = validate_move(game, chosen_move);
        make_game_move(game, result, chosen_move);
        if (game.promoting_pawn) {
            game.piece_selected = chosen_move.promoted_piece;
            handle_pawn_promotion(game, chosen_move);
        }

        is_game_over(game);
    }
    finished = false;
}

Move get_best_move(Game& game, int depth) {
    int best_score = -500000;
    Move best_move {};
    //int turn = game.turn;
    //std::cout << "generating\n";
    Move_list possible_moves = determine_possible_moves(game);
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(game, move1) > sort_moves_by_priority(game, move2);
    });

    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        //std::cout << "testing possible moves\n";
        auto& move = possible_moves.list[i];
        make_test_move(game, move);
        
        int move_eval = -negamax(game, depth - 1, -500000, 500000);
        move_eval += (std::rand() % 5) - 2;

        std::cout << "e: " << move << ' ' << move_eval << ' ' << game.turn << '\n';
       
        if (move_eval > best_score) {
            best_score = move_eval;
            best_move = move;
        }
    
        if (best_move.new_square == best_move.prev_square) {
            best_move = move;
        }
        undo_test_move(game, move);
        //turn = game.turn;
        
    }
    std::cout << "Best score: " << best_score << '\n';
    return best_move;
}

int negamax(Game& game, int depth, int alpha, int beta) { 

    if (depth == 0) {
        //std::cout << "reached depth 0\n";
        positions_searched++;
        int perspective = (game.turn == white) ? 1 : -1;
        return perspective * evaluate(game, game.bitboards);
    }
    
    Move_list possible_moves = determine_possible_moves(game);
    if (possible_moves.num_moves == 0) {
        int king_square = __builtin_ctzll(game.bitboards.bitboards[game.turn][king]);
        if (is_square_attacked(game.bitboards, king_square, game.turn)) {
            return -400000 - depth * 50;
        } else {
            return 0;
        }
    }
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(game, move1) > sort_moves_by_priority(game, move2);
    });
    //std::cout << "size: " << possible_moves.size() << '\n';
 
    int max_eval = -600000;

    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        auto& possible_move = possible_moves.list[i];
        
        make_test_move(game, possible_move);
        int eval = -negamax(game, depth - 1, -beta, -alpha);
        undo_test_move(game, possible_move);

        alpha = std::max(eval, alpha);
        max_eval = std::max(eval, max_eval);
        if (beta <= alpha) {
            break;
        } 
    }
    return max_eval;
}

int evaluate(Game& game, Bitboards& bitboards) {
    int eval { 0 };
    
    for (int piece { 0 }; piece < 5; piece++) {
        eval += __builtin_popcountll(bitboards.bitboards[white][piece]) * piece_values[piece];
        eval -= __builtin_popcountll(bitboards.bitboards[black][piece]) * piece_values[piece];
    }

    for (int piece { 0 }; piece <= 5; piece++) {
        eval += positional_eval(game, bitboards.bitboards[white][piece], piece, white);
        eval -= positional_eval(game, bitboards.bitboards[black][piece], piece, black);
    }
    return eval;
}

__attribute__((always_inline)) int positional_eval(Game& game, uint64_t bitboard, int piece, int colour) {
    int eval { 0 };
    while (bitboard) {
        int square = (piece == white) ? __builtin_ctzll(bitboard) : __builtin_ctzll(bitboard) ^ 56;
        eval += start_value_tables[piece][square] + (7800 - game.value_white_pieces - game.value_black_pieces) / 7800.0 *
        (start_value_tables[piece][square] - endgame_value_tables[piece][square]);
        bitboard &= bitboard - 1;
    }
    return eval;
}

int sort_moves_by_priority(Game& game, Move& move) {
    int move_score_guess = 0;
    int square = (move.turn == white) ? move.new_square : move.new_square ^ 56;
    move_score_guess += 30 * (start_value_tables[move.piece][square] + 
        (7800 - game.value_white_pieces - game.value_black_pieces) / 7800.0 *
        (start_value_tables[move.piece][square] - endgame_value_tables[move.piece][square]));
    if (move.piece_taken.piece_type != none) {
        move_score_guess += (10000 * piece_values[move.piece_taken.piece_type] - piece_values[move.piece]);
    }
    if (move.special_move == promotion) {
        move_score_guess += 70 * piece_values[move.promoted_piece];
    }
    if (move.special_move == castling) {
        move_score_guess += 100;
    }
    return move_score_guess;
}

void is_game_over(Game& game) {
    evaluate_king_checks(game);
    game.board_record.push_back(game.zobrist_hash);
    //record_board(game);
    
    Game game_copy = game;
    Move_list moves = determine_possible_moves(game_copy);
    //std::cout << moves.size() << '\n';
    //std::cout << "plys to 100: " << game.plys_to_100 << '\n';
    if (moves.num_moves > 0) {
        std::cout << moves.num_moves << '\n';
    }
    if (moves.num_moves == 0 || determine_repetition(game) || 
        determine_insufficient_material(game) || game.plys_to_100 == 100) {
        end_game(game);
    }
}

bool determine_insufficient_material(Game& game) {
    game.value_black_pieces = 0;
    game.value_white_pieces = 0;
    for (int piece { 0 }; piece < 5; piece++) {
        game.value_white_pieces += __builtin_popcountll(game.bitboards.bitboards[white][piece]) * piece_values[piece];
        game.value_black_pieces += __builtin_popcountll(game.bitboards.bitboards[black][piece]) * piece_values[piece];
    }
    bool pawns_on_board = ((game.bitboards.bitboards[white][pawn] | game.bitboards.bitboards[black][pawn]) == 0) ? 
    false : true;
    if (game.value_black_pieces <= 300 && game.value_white_pieces <= 300 && !pawns_on_board) {
        game.insufficient_material = true;
        return true;
    }
    return false;
}

bool determine_repetition(Game& game) {
    int occurrences { 1 };
    int latest_move { static_cast<int>(game.board_record.size() - 1)};
    for (int i { latest_move - 1 }; i >= 0; i--) {
        if (game.board_record[i] == game.board_record[latest_move]) {
            occurrences++;
        }
        if (occurrences == 3) {
            game.repetition = true;
            return true;
        }
    }
    return false;
}

void handle_pawn_promotion(Game& game, Move& move) {

    int current_offset = (move.turn == white) ? 0 : 6;
    assert(move.turn == game.turn);

    Chessboard& board = game.board;
    Bitboards& bitboards = game.bitboards;
    move_piece(game, move);
    //u_int64_t square = (1ULL << move.new_square);
    move.special_move = promotion;
    move.promoted_piece = game.piece_selected;
    
    replace_piece(game, move.turn, pawn, move.promoted_piece, move.new_square);

    bitboards.update_occupied();
    update_castling_flags(game, move);
    game.zobrist_hash ^= zobrist_black_turn;
    game.turn = ((game.turn == white) ? black : white);
    game.move_record.push_back(move);
    game.piece_selected = none;
    game.promoting_pawn = false;

}

void evaluate_king_checks(Game& game) {
    int square = __builtin_ctzll(game.bitboards.bitboards[game.turn][king]);
    int new_result = is_square_attacked(game.bitboards, square, game.turn);
    if (game.turn == white) {
        game.white_in_check = ((new_result == 1) ? true : false);
    } else {
        game.black_in_check = ((new_result == 1) ? true : false);
    }
}

void end_game(Game& game) {
    //std::cout << "It is over\n";
    game.game_over = true;
    game.state = Gamestate::Gameover;
    if (game.repetition || game.insufficient_material || game.plys_to_100 == 100) {
        return;
    }
    if (game.turn == black) {
        if (game.black_in_check) {
            game.checkmate = true;
            game.winner = white;
        } else {
            game.stalemate = true;
        }
    } else {
        if (game.white_in_check) {
            game.checkmate = true;
            game.winner = black;
        } else {
            game.stalemate = true;
        }
    }
}

void switch_move(Move& move) {
    int temp = move.prev_square;
    move.prev_square = move.new_square;
    move.new_square = temp;
}

void move_piece(Game& game, Move& move, bool undo) {

    //int piece = move.piece;
    int captured { -1 };
    captured = game.board[move.new_square].piece_occupying.piece_type;

    game.board[move.new_square].piece_occupying = game.board[move.prev_square].piece_occupying;
    game.board[move.prev_square].piece_occupying = { none, none };

    int current = (move.turn == white) ? 0 : 6;
    int opposing = (move.turn == white) ? 6 : 0;
    game.zobrist_hash ^= zobrist_table[move.piece + current][move.prev_square];
    game.zobrist_hash ^= zobrist_table[move.piece + current][move.new_square];

    uint64_t from_bit = 1ULL << move.prev_square; 
    uint64_t to_bit = 1ULL << move.new_square;
    int opposing_turn = ((move.turn == white) ? black : white);

    game.bitboards.bitboards[move.turn][move.piece] ^= (from_bit | to_bit);
    game.bitboards.occupied_tables[move.turn] ^= (from_bit | to_bit);
 
    if (captured != -1) {
        game.bitboards.bitboards[opposing_turn][captured] &= ~to_bit;
        game.bitboards.occupied_tables[opposing_turn] &= ~to_bit;
        game.zobrist_hash ^= zobrist_table[captured + opposing][move.new_square];
    }
    
    game.bitboards.occupied = game.bitboards.occupied_tables[black] | game.bitboards.occupied_tables[white];
    //bitboards.update_occupied();
    //print_bitboard(bitboards.occupied);
    assert(game.board[move.new_square].piece_occupying.piece_type != none);
    /*
    if (board[move.new_square].piece_occupying.piece_type == none) {
        //print_all_bitboards(bitboards);
        std::cout << move << '\n';
        std::cout << "failed\n";
        return;
    }*/
}

int validate_move(Game& game, Move& move, bool only_checking_checks) {
    int result { 0 };
    if (move.new_square < 0 || move.new_square > 63) {
        return -1;
    }

    if ((move.prev_square != move.new_square) && 
        (game.board[move.new_square].piece_occupying.piece_type == none || 
        game.board[move.new_square].piece_occupying.colour != move.turn)) {
        
        if (move.piece == pawn) {
            result = validate_move_pawn(game, move);
        } else if (move.piece == knight) {
            result = validate_move_knight(game.bitboards, move);
        } else if (move.piece == bishop) {
            result = validate_move_bishop(game.bitboards, move);
        } else if (move.piece == rook) {
            result = validate_move_rook(game.bitboards, move);
        } else if (move.piece == queen) {
            result = validate_move_queen(game.bitboards, move);
        } else if (move.piece == king) {
            result = validate_move_king(game, move);
        } else {
            return 0;
        }
        if (result >= 0 && !only_checking_checks) {
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

int validate_move_pawn(Game& game, Move& move) {
    int square = move.new_square;
    Bitboards& bitboards = game.bitboards;
    uint64_t mask = 1ULL;

    if (move.turn == white && 32 <= move.prev_square && move.prev_square <= 39 
        && std::abs(move.new_square % 8 - move.prev_square % 8) == 1 && 
        (std::abs(move.new_square - move.prev_square) == 7 || 
        std::abs(move.new_square - move.prev_square) == 9) && (~bitboards.occupied & mask << square)) {
        return validate_en_passant(game, move);
    } else if (move.turn == black && 24 <= move.prev_square && move.prev_square <= 31 
        && std::abs(move.new_square % 8 - move.prev_square % 8) == 1 && 
        (std::abs(move.new_square - move.prev_square) == 7 || 
        std::abs(move.new_square - move.prev_square) == 9)&& (~bitboards.occupied & mask << square)) {
        return validate_en_passant(game, move);  
    }

    int from = move.prev_square;
    int to = move.new_square;
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
    Bitboards& bitboards = game.bitboards;
    int captured_square = ((move.turn == white) ? move.new_square - 8 : move.new_square + 8);
    uint64_t mask = 1ULL;
    int opposing_turn = ((move.turn == white) ? black : white);
   
    if (bitboards.bitboards[opposing_turn][pawn] & mask << captured_square) {
        Move prev_move = game.move_record[game.move_record.size() - 1];
        //std::cout << "prev" << prev_move.prev_row << ' ' << prev_move.prev_col << 
        //" Curr" << prev_move.new_row << prev_move.new_col << '\n';
        if (prev_move.piece == pawn && prev_move.new_square == captured_square && 
            std::abs(prev_move.new_square - prev_move.prev_square) == 16) {
            return 3;
        }
        return -1;
    }
    return -1;
}

int validate_move_knight(Bitboards& bitboards, Move& move) {
    u_int64_t mask = 1ULL << move.new_square;
    if (bitboards.knight_attacks[move.prev_square] & mask) {
        return 0;
    }
    return -1;
}

int validate_move_bishop(Bitboards& bitboards, Move& move) {
    if (std::abs(move.new_square - move.prev_square) % 9 == 0 || 
        std::abs(move.new_square - move.prev_square) % 7 == 0) {
        uint64_t path = bitboards.between_table[move.prev_square][move.new_square];
        if (path & bitboards.occupied) {
            return -1;
        }
        return 0;
    }
    return -1;
}

int validate_move_rook(Bitboards& bitboards, Move& move) {
    if ((move.new_square - move.prev_square) % 8 == 0 || std::abs(move.new_square - move.prev_square) <= 7) {
        uint64_t path = bitboards.between_table[move.prev_square][move.new_square];
        if (path & bitboards.occupied) {
            return -1;
        }
        return 0;
    }
    return -1;
}

int validate_move_queen(Bitboards& bitboards, Move& move) {
    if (validate_move_rook(bitboards, move) == 0 || validate_move_bishop(bitboards, move) == 0) {
        return 0;
    } 
    return -1;
}

int validate_move_king(Game &game, Move& move) {
    if (game.bitboards.king_moves[move.prev_square] & 1ULL << move.new_square) {
        return 0;
    } else if (move.prev_square / 8 == move.new_square / 8 && std::abs(move.new_square - move.prev_square) == 2) {
        Bitboards bitboard_copy = game.bitboards;
        return test_castling(game, move);
    }
    return -1;
}

int is_square_attacked(Bitboards& bitboard_copy, int square, int turn) {
    int opposing_colour = ((turn == white) ? black : white);

    if (bitboard_copy.knight_attacks[square] & bitboard_copy.bitboards[opposing_colour][knight]) {
        return 1;
    }
    if (bitboard_copy.pawn_attacks[turn][square] & bitboard_copy.bitboards[opposing_colour][pawn]) {
        return 1;
    }
    if (bitboard_copy.king_moves[square] & bitboard_copy.bitboards[opposing_colour][king]) {
        return 1;
    }
    uint64_t bishop_attacks = get_bishop_attacks(square, bitboard_copy.occupied, bitboard_copy);
    uint64_t rook_attacks = get_rook_attacks(square, bitboard_copy.occupied, bitboard_copy);
    if ((bishop_attacks & bitboard_copy.bitboards[opposing_colour][bishop]) || 
        (bishop_attacks & bitboard_copy.bitboards[opposing_colour][queen])) {
        return 1;
    }
    if ((rook_attacks & bitboard_copy.bitboards[opposing_colour][rook]) || 
        (rook_attacks & bitboard_copy.bitboards[opposing_colour][queen])) {
        return 1;
    }
    
    return 0;
}

int test_castling(Game &game, Move& move) {

    uint64_t castle_mask_right_w = (1ULL << 5) | (1ULL << 6);
    uint64_t castle_mask_left_w = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
    uint64_t castle_mask_right_b = (1ULL << 62) | (1ULL << 61);
    uint64_t castle_mask_left_b = (1ULL << 59) | (1ULL << 58) | (1ULL << 57);
    uint8_t mask = 1ULL;
    Bitboards& bitboards = game.bitboards;
    int prev_square = move.prev_square;
    int new_square = move.new_square;
    //print_bitboard(bitboard_copy.occupied);
    if (move.turn == white && prev_square == 4 && !is_square_attacked(bitboards, 4, move.turn)) {
        if (new_square == 2 && (mask << 2 & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_left_w) == 0) {
                if (!is_square_attacked(bitboards, 3, move.turn) && !is_square_attacked(bitboards, 2, move.turn)) {
                    return 2;
                }
            }
        } else if (new_square == 6 && (mask << 3 & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_right_w) == 0) {
                if (!is_square_attacked(bitboards, 5, move.turn) && !is_square_attacked(bitboards, 6, move.turn)) {
                    return 1;
                }
            }
        }
    } else if (move.turn == black && prev_square == 60 && !is_square_attacked(bitboards, 60, move.turn)) { 
        if (new_square == 58 && (mask & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_left_b) == 0) {
                if (!is_square_attacked(bitboards, 59, move.turn) && !is_square_attacked(bitboards, 58, move.turn)) {
                    return 2;
                }
            }
        } else if (new_square == 62 && (mask << 1 & game.castling_rights)) {
            if ((bitboards.occupied & castle_mask_right_b) == 0) {
                if (!is_square_attacked(bitboards, 61, move.turn) && !is_square_attacked(bitboards, 62, move.turn)) {
                    return 1;
                }
            }
        }
    }
    return -1;
}

int is_in_check(Game& game, Move& move) {
    int mover = game.turn;
    make_test_move(game, move); 
    int king_square = __builtin_ctzll(game.bitboards.bitboards[mover][king]);
    int in_check = is_square_attacked(game.bitboards, king_square, mover);
    undo_test_move(game, move);
    return in_check;
}

Move_list determine_possible_moves(Game& game) {
    Move_list moves;
    add_pawn_moves(game, moves);
    add_knight_moves(game, moves);
    add_bishop_moves(game, moves);
    add_rook_moves(game, moves);
    add_queen_moves(game, moves);
    add_king_moves(game, moves);
    return moves;
}

__attribute__((always_inline)) void add_pawn_moves(Game& game, Move_list& moves) {
    Bitboards& bitboards = game.bitboards;
    Chessboard& board = game.board;
    int& turn = game.turn;
    uint64_t mask = 1ULL;
    uint64_t current_pieces = bitboards.bitboards[turn][pawn];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        int opposing_turn = ((turn == white) ? black : white);
        uint64_t captures = bitboards.pawn_attacks[turn][from_square];
        uint64_t non_captures = bitboards.pawn_moves[turn][from_square];
        while (captures) {
            int to_square = __builtin_ctzll(captures);
            if (mask << to_square & bitboards.occupied_tables[opposing_turn]) {
                int row = 7 - to_square / 8;
                if ((row == 7 && turn == black) || (row == 0 && turn == white)) {
                    int promotion_choices[4] = { knight, bishop, rook, queen };
                    for (int piece: promotion_choices) {
                        Move move { from_square, to_square, turn, pawn, board[to_square].piece_occupying };
                        move.special_move = promotion;
                        move.promoted_piece = piece;

                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    }
                } else {
                    Move move { from_square, to_square, turn, pawn, board[to_square].piece_occupying };
                    if (!is_in_check(game, move)) {
                        moves.list[local_counter++] = move;
                    }
                }
            } else if (7 - from_square / 8 == 3 && turn == white && game.move_record.size() > 0) {
                Move prev_move = game.move_record.back();
                if (prev_move.new_square - prev_move.prev_square == -16 && prev_move.piece == pawn && 
                    std::abs(prev_move.new_square % 8 - from_square % 8) == 1) {
                    //std::cout << to_square % 8 - prev_move.new_col << '\n';
                    if (to_square - prev_move.new_square == 8) {
                        //std::cout << "in en passant\n";
                        Move move { from_square, to_square, turn, pawn, board[to_square - 8].piece_occupying };
                        assert(board[to_square - 8].piece_occupying.piece_type != none);
                        move.special_move = en_passant;
                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    } 
                }
            } else if (7 - from_square / 8 == 4 && turn == black && game.move_record.size() > 0){
                Move prev_move = game.move_record.back();
                if (prev_move.new_square - prev_move.prev_square == 16 && prev_move.piece == pawn && 
                    std::abs(prev_move.new_square % 8 - from_square % 8) == 1) {
                    if (to_square - prev_move.new_square == -8) {
                        Move move { from_square, to_square, turn, pawn, board[to_square + 8].piece_occupying };
                        move.special_move = en_passant;
                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    } 
                }
            }
            captures &= captures - 1;
        }
        while (non_captures) {
            int to_square = __builtin_ctzll(non_captures);
            if (mask << to_square & ~bitboards.occupied) {
                int row = 7 - to_square / 8;
                if ((row == 7 && turn == black) || (row == 0 && turn == white)) {
                    int promotion_choices[4] = { knight, bishop, rook, queen };
                    for (int piece: promotion_choices) {
                        Move move { from_square, to_square, turn, pawn, board[to_square].piece_occupying };
                        move.special_move = promotion;
                        move.promoted_piece = piece;
                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    }
                } else {
                    Move move { from_square, to_square, turn, pawn, board[to_square].piece_occupying  };
                    
                    if (!is_in_check(game, move)) {
                        if (std::abs(from_square - to_square) == 8) {
                            moves.list[local_counter++] = move;
                        } else if (std::abs(from_square - to_square) == 16) {
                            if ((turn == white && mask << to_square & ~bitboards.occupied) && 
                            (mask << (to_square - 8) & ~bitboards.occupied) && mask << from_square & RANK_2) {
                                moves.list[local_counter++] = move;
                            } else if ((turn == black && mask << to_square & ~bitboards.occupied) && 
                            (mask << (to_square + 8) & ~bitboards.occupied) && mask << from_square & RANK_7) {
                                moves.list[local_counter++] = move;
                            }
                        }
                    }
                }
            }
            non_captures &= non_captures - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

__attribute__((always_inline)) void add_knight_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint64_t current_pieces = game.bitboards.bitboards[game.turn][knight];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t knight_moves = game.bitboards.knight_attacks[from_square];
        while (knight_moves) {
            int to_square = __builtin_ctzll(knight_moves);
            if (mask << to_square & ~game.bitboards.occupied_tables[game.turn]) {
                Move move { from_square, to_square, game.turn, knight, 
                    game.board[to_square].piece_occupying };
                if (!is_in_check(game, move)) {
                    moves.list[local_counter++] = move;
                }
            }
            knight_moves &= knight_moves - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

__attribute__((always_inline)) void add_bishop_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint64_t current_pieces = game.bitboards.bitboards[game.turn][bishop];
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) 
        & ~game.bitboards.occupied_tables[game.turn];
        while (bishop_attacks) {
            int to_square = __builtin_ctzll(bishop_attacks);
            Move move { from_square, to_square, game.turn, bishop, game.board[to_square].piece_occupying };
            if (!is_in_check(game, move)) {
                moves.list[moves.num_moves++] = move;
            }
            bishop_attacks &= bishop_attacks - 1;
        }
        current_pieces &= current_pieces - 1;
    }
}

__attribute__((always_inline)) void add_rook_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint64_t current_pieces = game.bitboards.bitboards[game.turn][rook];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t rook_attacks = get_rook_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        ~game.bitboards.occupied_tables[game.turn];
        while (rook_attacks) {
            int to_square = __builtin_ctzll(rook_attacks);
            Move move { from_square, to_square, game.turn, rook, game.board[to_square].piece_occupying };
            if (!is_in_check(game, move)) {
                moves.list[local_counter++] = move;
            }
            rook_attacks &= rook_attacks - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

__attribute__((always_inline)) void add_queen_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint64_t current_pieces = game.bitboards.bitboards[game.turn][queen];
    int local_counter = moves.num_moves;
    int to_square;

    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        ~game.bitboards.occupied_tables[game.turn];
        while (bishop_attacks) {
            to_square = __builtin_ctzll(bishop_attacks);
            Move move { from_square, to_square, game.turn, queen, game.board[to_square].piece_occupying };
            if (!is_in_check(game, move)) {
                moves.list[local_counter++] = move;
            }
            bishop_attacks &= bishop_attacks - 1;
        }
        uint64_t rook_attacks = get_rook_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        ~game.bitboards.occupied_tables[game.turn];
        while (rook_attacks) {
            to_square = __builtin_ctzll(rook_attacks);
            Move move { from_square, to_square, game.turn, queen, game.board[to_square].piece_occupying };
            if (!is_in_check(game, move)) {
                moves.list[local_counter++] = move;
            }
            rook_attacks &= rook_attacks - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

__attribute__((always_inline)) void add_king_moves(Game& game, Move_list& moves) {

    uint64_t mask = 1ULL;
    uint64_t current_pieces = game.bitboards.bitboards[game.turn][king];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t king_moves = game.bitboards.king_moves[from_square];
        while (king_moves) {
            int to_square = __builtin_ctzll(king_moves);
            if (mask << to_square & ~game.bitboards.occupied_tables[game.turn]) {
                //print_bitboard(bitboards.occupied_tables[turn]);
                Move move { from_square, to_square, game.turn, king, game.board[to_square].piece_occupying };
                if (!is_in_check(game, move)) {
                    moves.list[local_counter++] = move;
                }
            }
            king_moves &= king_moves - 1;
        }
        if (from_square == 4 && game.turn == white) {
            Move move1 { from_square, 6, game.turn, king, { none, none }, castling };
            Move move2 { from_square, 2, game.turn, king, { none, none }, castling };
            if (test_castling(game, move1) == 1) {
                moves.list[local_counter++] = move1;
            }
            if (test_castling(game, move2) == 2) {
                moves.list[local_counter++] = move2;
                //std::cout << "here\n";
            }
        } else if (from_square == 60 && game.turn == black) {
            Move move1 { from_square, 62, game.turn, king, { none, none }, castling };
            Move move2 { from_square, 58, game.turn, king, { none, none }, castling };
            if (test_castling(game, move1) == 1) {
                moves.list[local_counter++] = move1;
            }
            if (test_castling(game, move2) == 2) {
                moves.list[local_counter++] = move2;
            }
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}



