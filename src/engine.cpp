#include "engine.h"
#include "logic.h"
#include "perft.h"
#include <iostream>

std::chrono::steady_clock::time_point search_start_time;
int nodes_searched = 0;
int terminate_search { false };

void clear_transposition_table() {
    for (int i { 0 }; i < TABLE_SIZE; i++) {
        transposition_table[i].eval = 0;
        transposition_table[i].zobrist_key = 0;
    }
}

void record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move) {
    int index = key & (TABLE_SIZE - 1);

    if (transposition_table[index].zobrist_key != 0 && transposition_table[index].depth > depth) {
        return;
    }
    transposition_table[index].best_move = best_move;
    transposition_table[index].eval = eval;
    transposition_table[index].zobrist_key = key;
    transposition_table[index].flag = flag;
    transposition_table[index].depth = depth;
}

int probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move) {
    int index = key & (TABLE_SIZE - 1);
    table_entry entry = transposition_table[index];
    if (key == entry.zobrist_key) {
        best_move = entry.best_move;
        if (entry.depth >= depth) {
            if (entry.flag == tt_flag::tt_exact) {
                return entry.eval;
            } else if (entry.flag == tt_flag::tt_alpha && entry.eval <= alpha) {
                return alpha;
            } else if (entry.flag == tt_flag::tt_beta && entry.eval >= beta) {
                return beta;
            }
        }

    }
    return -999999;
}

void undo_test_move(Game& game, Move& prev_move) {

    undo_move(game, prev_move);
    restore_zobrist_en_passant_and_castling(game, prev_move);

    if (!game.move_record.empty()) {
        game.move_record.pop_back();
    }

    game.turn = ((game.turn == BLACK) ? WHITE : BLACK);
    game.zobrist_hash ^= zobrist_black_turn;
}

void make_test_move(Game& game, Move& move) {

    assert(game.board[move.get_from_square()] != EMPTY_SQUARE);
    int to_square = move.get_to_square();
    int from_square = move.get_from_square();
    int turn = move.get_turn();
    uint8_t move_type = move.get_move_type();

    update_zobrist_en_passant(game, move);

    if (move_type == PROMOTION) {
        //std::cout << "promoting pawn\n";
    
        move_piece(game, move.get_piece(), from_square, to_square, turn);
        uint8_t promotion_piece = convert_promotion_piece(move, move.get_promotion_piece());
        replace_piece(game, move.get_piece(), promotion_piece, to_square);

        update_castling_flags(game, move);
        game.move_record.push_back(move);
        game.piece_selected = EMPTY_SQUARE;
        game.turn = WHITE + BLACK - game.turn; // flip the turn
        game.zobrist_hash ^= zobrist_black_turn;
        return;
    }

    //std::cout << "just before moving piece\n";
    assert(from_square != to_square);
    move_piece(game, move.get_piece(), from_square, to_square, turn);

    if (move_type == CASTLING) {
        uint8_t piece = (turn == WHITE) ? WHITE_ROOK : BLACK_ROOK;
        int row = 7 - to_square / 8;
        if (to_square - from_square == 2) {
            move_piece(game, piece, 56 - 8 * row + 7, 56 - 8 * row + 5, turn);
        } else if (to_square - from_square == -2) {
            move_piece(game, piece, 56 - 8 * row, 56 - 8 * row + 3, turn);
        }
    }

    if (move_type == EN_PASSANT) {
        int captured_square = ((game.turn == WHITE) ? to_square - 8 : to_square + 8);
        int opposing_turn = ((game.turn == WHITE)) ? BLACK : WHITE;
        uint8_t captured_piece = (game.turn == WHITE) ? BLACK_PAWN : WHITE_PAWN;
        remove_piece(game, opposing_turn, captured_piece, captured_square);
    } 
    update_castling_flags(game, move);
    game.move_record.push_back(move);
    game.turn = ((game.turn == WHITE) ? BLACK : WHITE);
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
        Move chosen_move = get_best_move(game_copy, 2000);
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
        std::cout << chosen_move.get_from_square() << " -> " << chosen_move.get_to_square() << '\n';
        if (game.board[chosen_move.get_to_square()] != EMPTY_SQUARE) {
            chosen_move.set_captured(game.board[chosen_move.get_to_square()]);
        }
        int result = validate_move(game, chosen_move);
        make_game_move(game, result, chosen_move);
        if (game.promoting_pawn) {
            chosen_move.set_promotion_piece(game.piece_selected);
            handle_pawn_promotion(game, chosen_move);
        }

        is_game_over(game);
    }
    finished = false;
}

Move get_best_move(Game& game, int search_allocated_time_ms) {
    Move current_best_move {};
    Move overall_best_move {};
    search_start_time = std::chrono::steady_clock::now();
    terminate_search = false;
    nodes_searched = 0;
    int overall_best_score, depth_best_score = -50000;
    Move_list possible_moves = determine_possible_moves(game);

    /*for (int i { 0 }; i < possible_moves.num_moves; i++) {
        possible_moves.list[i].eval = sort_moves_by_priority(game, possible_moves.list[i]);
    }*/

    //std::cout << "generating\n";
    for (int depth { 1 }; depth < 40; depth++) {
        depth_best_score = -500000;
        int alpha = -500000, beta = 500000;
        int move_eval = 0;
        std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
        [&](Move& move1, Move& move2) {
            //return move1.eval > move2.eval;
            return 0;
        });
        current_best_move = possible_moves.list[0];

        for (int i { 0 }; i < possible_moves.num_moves; i++) {
            //std::cout << "testing possible moves\n";
            auto& move = possible_moves.list[i];

            make_test_move(game, move);
            move_eval = find_eval(game, i, depth, beta, alpha, search_allocated_time_ms);
            undo_test_move(game, move);

            if (terminate_search) {
                break; 
            }
            //move_eval += (std::rand() % 5) - 2;
            //move.eval = move_eval;

            std::cout << "depth:" <<  depth << " e: " << move << ' ' << move_eval << ' ' << game.turn << '\n';
        
            //alpha = std::max(move_eval, alpha);
            if (move_eval > depth_best_score) {
                depth_best_score = move_eval;
                current_best_move = move;
            }
        
            if (current_best_move.get_to_square() == current_best_move.get_from_square()) {
                current_best_move = move;
            }
        }
        
        if (!terminate_search) {
            overall_best_move = current_best_move;
            overall_best_score = depth_best_score;
        } else {
            break;
        }
    }
    assert(overall_best_move.get_to_square() != overall_best_move.get_from_square());
    std::cout << "Best score: " << overall_best_score << '\n';
    return overall_best_move;
}

int negamax(Game& game, int depth, int alpha, int beta, int search_allocated_time_ms) { 

    Move stored_move {};
    int stored_eval = probe_transposition_table(game.zobrist_hash, depth, alpha, beta, stored_move);
    if (stored_eval != -999999) {
        return stored_eval;
    }
    if (terminate_search) {
        return 0;
    }

    if (depth == 0) {
        positions_searched++;
        int perspective = (game.turn == WHITE) ? 1 : -1;
        return perspective * evaluate(game);
    }
    nodes_searched++;
    if ((nodes_searched & 2047) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
        if (elapsed > search_allocated_time_ms) {
            terminate_search = true;
            return 0;
        }
    }
    
    Move_list possible_moves = determine_possible_moves(game);
    uint8_t king = (game.turn == WHITE) ? WHITE_KING : BLACK_KING;
    if (possible_moves.num_moves == 0) {
        int king_square = __builtin_ctzll(game.bitboards.bitboards[king]);
        if (is_square_attacked(game.bitboards, king_square, game.turn)) {
            return -400000 - depth * 50;   // favour quick checkmates
        } else {
            return 0;
        }
    }
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        if (move1 == stored_move) {
            return true;
        }
        if (move2 == stored_move) {
            return false;
        }
        return sort_moves_by_priority(game, move1) > sort_moves_by_priority(game, move2);
    });
 
    int max_eval = -600000;
    int move_eval;
    int original_alpha = alpha;
    Move best_move_this_node {};

    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        auto& possible_move = possible_moves.list[i];
        
        make_test_move(game, possible_move);
        move_eval = find_eval(game, i, depth, beta, alpha, search_allocated_time_ms);
        undo_test_move(game, possible_move);

        if (move_eval > max_eval) {
            max_eval = move_eval;
            best_move_this_node = possible_move;
        }

        alpha = std::max(move_eval, alpha);
        if (alpha >= beta) {
            break;
        } 
    }
    tt_flag flag;
    if (max_eval <= original_alpha) {
        flag = tt_flag::tt_alpha;
    } else if (max_eval >= beta) {
        flag = tt_flag::tt_beta;
    } else {
        flag = tt_flag::tt_exact;
    }
    if (max_eval <= 20000) {
        record_entry(game.zobrist_hash, max_eval, depth, flag, best_move_this_node);
    }
    return max_eval;
}

// principal variation search
inline int find_eval(Game& game, int move_num, int depth, int beta, int alpha, int search_allocated_time_ms) {
    int move_eval;
    if (move_num == 0) {
        move_eval = -negamax(game, depth - 1, -beta, -alpha, search_allocated_time_ms);
    } else {
        move_eval = -negamax(game, depth - 1, -alpha - 1, -alpha, search_allocated_time_ms);
        if (move_eval > alpha && move_eval < beta) {
            move_eval = -negamax(game, depth - 1, -beta, -alpha, search_allocated_time_ms);
        }
    }
    return move_eval;
}

int evaluate(Game& game) {
    int eval { 0 };
    
    for (int piece { 1 }; piece <= 6; piece++) {
        eval += __builtin_popcountll(game.bitboards.bitboards[piece]) * piece_values[piece];
        eval += positional_eval(game, game.bitboards.bitboards[piece], piece);
    }
    for (int piece { 9 }; piece <= 14; piece++) {
        eval -= __builtin_popcountll(game.bitboards.bitboards[piece]) * piece_values[piece];
        eval -= positional_eval(game, game.bitboards.bitboards[piece], piece);
    }

    return eval;
}

__attribute__((always_inline)) int positional_eval(Game& game, uint64_t bitboard, uint8_t piece) {
    int eval { 0 };
    if ((piece >> 3) & 1) {
        piece -= 8;
    }
    while (bitboard) {
        int square = ((piece >> 3) == WHITE) ? __builtin_ctzll(bitboard) : __builtin_ctzll(bitboard) ^ 56;
        eval += (start_value_tables[piece - 1][square] + (7800 - game.value_white_pieces + game.value_black_pieces) / 7800.0 *
        (endgame_value_tables[piece - 1][square] - start_value_tables[piece - 1][square]));
        bitboard &= bitboard - 1;
    }
    return eval;
}

int sort_moves_by_priority(Game& game, Move& move) {
    int move_score_guess = 0;
    int square = (move.get_turn() == WHITE) ? move.get_to_square() : move.get_to_square() ^ 56;
    uint8_t piece = move.get_piece();
    uint8_t captured = move.get_captured_piece();
    if ((piece >> 3) == BLACK) {
        piece -= 8;
    }
    if ((captured >> 3) == BLACK) {
        captured -= 8;
    }
    move_score_guess += 30 * (start_value_tables[piece - 1][square] + 
        (7800 - game.value_white_pieces - game.value_black_pieces) / 7800.0 *
        (start_value_tables[piece - 1][square] - endgame_value_tables[piece - 1][square]));
    if (captured != EMPTY_SQUARE) {
        move_score_guess += (10000 * piece_values[captured] - piece_values[piece]);
    }
    if (move.get_move_type() == PROMOTION) {
        move_score_guess += 70 * piece_values[move.get_promotion_piece() + 2];
    }
    if (move.get_move_type() == CASTLING) {
        move_score_guess += 100;
    }
    return move_score_guess;
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
    uint8_t target_piece = (game.turn == WHITE) ? WHITE_PAWN : BLACK_PAWN;
    uint64_t current_pieces = bitboards.bitboards[target_piece];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        int opposing_turn = ((turn == WHITE) ? BLACK : WHITE);
        uint64_t captures = bitboards.pawn_attacks[turn][from_square];
        uint64_t non_captures = bitboards.pawn_moves[turn][from_square];
        while (captures) {
            int to_square = __builtin_ctzll(captures);
            if (mask << to_square & bitboards.occupied_tables[opposing_turn]) {
                int row = 7 - to_square / 8;
                if ((row == 7 && turn == BLACK) || (row == 0 && turn == WHITE)) {
                    int promotion_choices[4] = { P_ROOK, P_KNIGHT, P_BISHOP, P_QUEEN };
                    for (int piece: promotion_choices) {
                        Move move;
                        move.set_move(from_square, to_square, target_piece, board[to_square]);
                        move.set_move_type(PROMOTION);
                        move.set_promotion_piece(piece);
                        //move.get_promotion_piece() = piece;

                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    }
                } else {
                    Move move;
                    move.set_move(from_square, to_square, target_piece, board[to_square]);
                    if (!is_in_check(game, move)) {
                        moves.list[local_counter++] = move;
                    }
                }
            } else if (7 - from_square / 8 == 3 && turn == WHITE && game.move_record.size() > 0) {
                Move prev_move = game.move_record.back();
                if (prev_move.get_to_square() - prev_move.get_from_square() == -16 && prev_move.get_piece() == BLACK_PAWN && 
                    std::abs(prev_move.get_to_square() % 8 - from_square % 8) == 1) {
                    //std::cout << to_square % 8 - prev_move.new_col << '\n';
                    if (to_square - prev_move.get_to_square() == 8) {
                        //std::cout << "in en passant\n";
                        Move move;
                        assert(board[to_square - 8] != EMPTY_SQUARE);
                        move.set_move_type(EN_PASSANT);
                        move.set_move(from_square, to_square, target_piece, board[to_square - 8]);
                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    } 
                }
            } else if (7 - from_square / 8 == 4 && turn == BLACK && game.move_record.size() > 0){
                Move prev_move = game.move_record.back();
                if (prev_move.get_to_square() - prev_move.get_from_square() == 16 && prev_move.get_piece() == WHITE_PAWN && 
                    std::abs(prev_move.get_to_square() % 8 - from_square % 8) == 1) {
                    if (to_square - prev_move.get_to_square() == -8) {
                        Move move;
                        move.set_move_type(EN_PASSANT);
                        move.set_move(from_square, to_square, target_piece, board[to_square + 8]);
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
                if ((row == 7 && turn == BLACK) || (row == 0 && turn == WHITE)) {
                    int promotion_choices[4] = { P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN };
                    for (int piece: promotion_choices) {
                        Move move;
                        move.set_move_type(PROMOTION);
                        move.set_promotion_piece(piece);
                        move.set_move(from_square, to_square, target_piece, board[to_square]);
                        if (!is_in_check(game, move)) {
                            moves.list[local_counter++] = move;
                        }
                    }
                } else {
                    Move move;
                    move.set_move(from_square, to_square, target_piece, board[to_square]);
                    if (!is_in_check(game, move)) {
                        if (std::abs(from_square - to_square) == 8) {
                            moves.list[local_counter++] = move;
                        } else if (std::abs(from_square - to_square) == 16) {
                            if ((turn == WHITE && mask << to_square & ~bitboards.occupied) && 
                            (mask << (to_square - 8) & ~bitboards.occupied) && mask << from_square & RANK_2) {
                                moves.list[local_counter++] = move;
                            } else if ((turn == BLACK && mask << to_square & ~bitboards.occupied) && 
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
    uint8_t piece = (game.turn == WHITE) ? WHITE_KNIGHT : BLACK_KNIGHT;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        //std::cout << "inside here\n";
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t knight_moves = game.bitboards.knight_attacks[from_square];
        while (knight_moves) {
            int to_square = __builtin_ctzll(knight_moves);
            if (mask << to_square & ~game.bitboards.occupied_tables[game.turn]) {
                Move move;
                move.set_move(from_square, to_square, piece, game.board[to_square]);
                //std::cout << std::bitset<32>(move.data);
                if (!is_in_check(game, move)) {
                    //std::cout << "adding move\n";
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
    uint8_t piece = (game.turn == WHITE) ? WHITE_BISHOP : BLACK_BISHOP;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) 
        & ~game.bitboards.occupied_tables[game.turn];
        while (bishop_attacks) {
            int to_square = __builtin_ctzll(bishop_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
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
    uint8_t piece = (game.turn == WHITE) ? WHITE_ROOK : BLACK_ROOK;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t rook_attacks = get_rook_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        ~game.bitboards.occupied_tables[game.turn];
        //std::cout << "finding rook attacks\n";
        while (rook_attacks) {
            int to_square = __builtin_ctzll(rook_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
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
    uint8_t piece = (game.turn == WHITE) ? WHITE_QUEEN : BLACK_QUEEN;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    int to_square;

    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        ~game.bitboards.occupied_tables[game.turn];
        while (bishop_attacks) {
            to_square = __builtin_ctzll(bishop_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
            if (!is_in_check(game, move)) {
                moves.list[local_counter++] = move;
            }
            bishop_attacks &= bishop_attacks - 1;
        }
        uint64_t rook_attacks = get_rook_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        ~game.bitboards.occupied_tables[game.turn];
        while (rook_attacks) {
            to_square = __builtin_ctzll(rook_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
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
    uint8_t piece = (game.turn == WHITE) ? WHITE_KING : BLACK_KING;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t king_moves = game.bitboards.king_moves[from_square];
        while (king_moves) {
            int to_square = __builtin_ctzll(king_moves);
            if (mask << to_square & ~game.bitboards.occupied_tables[game.turn]) {
                //print_bitboard(bitboards.occupied_tables[turn]);
                Move move;
                move.set_move(from_square, to_square, piece, game.board[to_square]);
                if (!is_in_check(game, move)) {
                    moves.list[local_counter++] = move;
                }
            }
            king_moves &= king_moves - 1;
        }
        if (from_square == 4 && game.turn == WHITE) {
            Move move1, move2;
            move1.set_move(from_square, 6, WHITE_KING, 0);
            move1.set_move_type(CASTLING);
            move2.set_move(from_square, 2, WHITE_KING, 0);
            move2.set_move_type(CASTLING);
            //std::cout << "setting castling move\n";
            if (validate_castling(game, move1) == 1) {
                moves.list[local_counter++] = move1;
            }
            if (validate_castling(game, move2) == 2) {
                moves.list[local_counter++] = move2;
                //std::cout << "here\n";
            }
        } else if (from_square == 60 && game.turn == BLACK) {
            Move move1, move2;
            move1.set_move(from_square, 62, BLACK_KING, 0);
            move1.set_move_type(CASTLING);
            move2.set_move(from_square, 58, BLACK_KING, 0);
            move2.set_move_type(CASTLING);
            if (validate_castling(game, move1) == 1) {
                moves.list[local_counter++] = move1;
            }
            if (validate_castling(game, move2) == 2) {
                moves.list[local_counter++] = move2;
            }
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}



