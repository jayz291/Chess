#include "engine.h"
#include "movegen.h"
#include "logic.h"
#include "perft.h"
#include "uci.h"
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
        replace_piece(game, turn, move.get_piece(), promotion_piece, to_square);

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
        computer_turn = thinking_in_progress = false;
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
        verify_board_sync(game);
        verify_zobrist_sync(game);
        is_game_over(game);
    }
    finished = false;
}

Move get_best_move(Game& game, int search_allocated_time_ms, int search_depth) {
    Move current_best_move {};
    Move overall_best_move {};
    search_start_time = std::chrono::steady_clock::now();
    terminate_search = false;
    nodes_searched = 0, positions_searched = 0;
    int overall_best_score, depth_best_score = -50000;
    Move_list possible_moves = determine_possible_moves(game);

    /*for (int i { 0 }; i < possible_moves.num_moves; i++) {
        possible_moves.list[i].eval = sort_moves_by_priority(game, possible_moves.list[i]);
    }*/

    //std::cout << "generating\n";
    for (int depth { 1 }; depth <= search_depth; depth++) {
        int seldepth = 0;
        depth_best_score = -500000;
        int alpha = -500000, beta = 500000;
        int move_eval = 0;
        std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
        [&](Move& move1, Move& move2) {
                //return move1.eval > move2.eval;
            return sort_moves_by_priority(game, move1) > sort_moves_by_priority(game, move2);
        });
        for (int i { 1 }; i < possible_moves.num_moves; i++) {
            if (possible_moves.list[i] == current_best_move) {
                std::swap(possible_moves.list[i], possible_moves.list[0]);
            }
        }
        current_best_move = possible_moves.list[0];

        for (int i { 0 }; i < possible_moves.num_moves; i++) {
            //std::cout << "testing possible moves\n";
            auto& move = possible_moves.list[i];

            make_test_move(game, move);
            move_eval = find_eval(game, i, depth, beta, alpha, search_allocated_time_ms, 1, seldepth);
            undo_test_move(game, move);

            if (terminate_search) {
                break; 
            }
            //move_eval += (std::rand() % 5) - 2;
            //std::cout << "depth:" <<  depth << " e: " << move << ' ' << move_eval << ' ' << game.turn << '\n';
        
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
            auto end = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - search_start_time).count();
            long long total_nodes = nodes_searched + positions_searched;
            
            std::cout << "info depth " << depth 
                      << " seldepth " << seldepth
                      << " score " << format_score(overall_best_score, depth)  
                      << " nodes " << total_nodes 
                      << " time " << elapsed_ms
                      << " pv " << to_chess_notation(overall_best_move) << std::endl;
        } else {
            break;
        }
    }
    assert(overall_best_move.get_to_square() != overall_best_move.get_from_square());
    //std::cout << "Best score: " << overall_best_score << '\n';
    return overall_best_move;
}

// negamax function:
// - alpha: the highest score that the maximising player can guarantee
// - beta: the lowest score that the minimising player can guarantee
int negamax(Game& game, int depth, int alpha, int beta, int search_allocated_time_ms, int ply,
    int& seldepth) { 

    Move stored_move {};
    int stored_eval = probe_transposition_table(game.zobrist_hash, depth, alpha, beta, stored_move);
    if (stored_eval != -999999) {
        return stored_eval;
    }
    if (terminate_search) {
        return 0;
    }

    uint8_t piece = (game.turn == WHITE) ? WHITE_KING : BLACK_KING;
    int king_square = __builtin_ctzll(game.bitboards.bitboards[piece]);
    bool in_check = is_square_attacked(game.bitboards, king_square, game.turn);
    int extension = 0;
    if (in_check && ply <= 50) {
        extension = 1;
    }

    if (depth + extension == 0) {
        return quiescence_search(game, alpha, beta, ply, seldepth);
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
        move_eval = find_eval(game, i, depth + extension, beta, alpha, search_allocated_time_ms, ply, seldepth);
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
inline int find_eval(Game& game, int move_num, int depth, int beta, int alpha, int search_allocated_time_ms,
    int ply, int& seldepth) {
    int move_eval;
    if (move_num == 0) {
        move_eval = -negamax(game, depth - 1, -beta, -alpha, search_allocated_time_ms, ply, seldepth);
    } else {
        move_eval = -negamax(game, depth - 1, -alpha - 1, -alpha, search_allocated_time_ms, ply, seldepth);
        if (move_eval > alpha && move_eval < beta) {
            move_eval = -negamax(game, depth - 1, -beta, -alpha, search_allocated_time_ms, ply, seldepth);
        }
    }
    return move_eval;
}

int evaluate(Game& game) {
    int eval { 0 };
    game.value_white_pieces = game.value_black_pieces = 0;
    for (int piece { 1 }; piece < 6; piece++) {
        int current_material = __builtin_popcountll(game.bitboards.bitboards[piece]) * piece_values[piece];
        game.value_white_pieces += current_material;
        eval += current_material;
    }
    for (int piece { 9 }; piece < 14; piece++) {
        int current_material = __builtin_popcountll(game.bitboards.bitboards[piece]) * piece_values[piece - 8];
        game.value_black_pieces += current_material;
        eval -= current_material;
    }
    for (int piece { 1 }; piece <= 6; piece++) {
        eval += positional_eval(game, game.bitboards.bitboards[piece], piece);
    }
    for (int piece { 9 }; piece <= 14; piece++) {
        eval -= positional_eval(game, game.bitboards.bitboards[piece], piece - 8, true);
    }
    return eval;
}

__attribute__((always_inline)) int positional_eval(Game& game, uint64_t bitboard, uint8_t piece, bool black) {
    int eval { 0 };
    while (bitboard) {
        int square = (!black) ? __builtin_ctzll(bitboard) : __builtin_ctzll(bitboard) ^ 56;
        eval += (start_value_tables[piece - 1][square] + (8000 - game.value_white_pieces - game.value_black_pieces) / 8000.0 *
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

// search captures deeper, until the position is "quiet"
int quiescence_search(Game& game, int alpha, int beta, int ply, int& seldepth) {
    int stand_pat = evaluate(game) * ((game.turn == WHITE) ? 1 : -1);
    positions_searched++;
    if (ply > seldepth) {
        seldepth = ply;
    }
    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    Move_list possible_moves = generate_captures_only(game);
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(game, move1) > sort_moves_by_priority(game, move2);
    });
    //std::cout << possible_moves.num_moves << '\n';
    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        make_test_move(game, possible_moves.list[i]);
        int score = -quiescence_search(game, -beta, -alpha, ply + 1, seldepth);
        undo_test_move(game, possible_moves.list[i]);
        if (score >= beta) {
            return beta;
        } 
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

Move_list determine_possible_moves(Game& game) {
    Move_list moves;
    add_pawn_moves<false>(game, moves);
    add_knight_moves<false>(game, moves);
    add_bishop_moves<false>(game, moves);
    add_rook_moves<false>(game, moves);
    add_queen_moves<false>(game, moves);
    add_king_moves<false>(game, moves);
    return moves;
}

Move_list generate_captures_only(Game& game) {
    Move_list moves;
    add_pawn_moves<true>(game, moves);
    add_knight_moves<true>(game, moves);
    add_bishop_moves<true>(game, moves);
    add_rook_moves<true>(game, moves);
    add_queen_moves<true>(game, moves);
    add_king_moves<true>(game, moves);
    return moves;
}





