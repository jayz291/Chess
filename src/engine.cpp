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

void record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move, int ply) {
    int index = key & (TABLE_SIZE - 1);
    int stored_score = eval;
    if (eval > CHECKMATE_THRESHOLD) {
        stored_score = eval + ply;
    } else if (eval < -CHECKMATE_THRESHOLD) {
        stored_score = eval - ply;
    }
    if (transposition_table[index].zobrist_key != 0 && transposition_table[index].depth > depth) {
        return;
    }
    transposition_table[index].best_move = best_move;
    transposition_table[index].eval = stored_score;
    transposition_table[index].zobrist_key = key;
    transposition_table[index].flag = flag;
    transposition_table[index].depth = depth;
}

int probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move, int ply) {
    int index = key & (TABLE_SIZE - 1);
    table_entry entry = transposition_table[index];
    int return_eval = entry.eval;
    if (return_eval > CHECKMATE_THRESHOLD) {
        return_eval -= ply; 
    } else if (return_eval < -CHECKMATE_THRESHOLD) {
        return_eval += ply;
    }
    if (key == entry.zobrist_key) {
        best_move = entry.best_move;
        if (entry.depth >= depth) {
            if (entry.flag == tt_flag::tt_exact) {
                return return_eval;
            } else if (entry.flag == tt_flag::tt_alpha && return_eval <= alpha) {
                return alpha;
            } else if (entry.flag == tt_flag::tt_beta && return_eval >= beta) {
                return beta;
            }
        }

    }
    return NO_ENTRY_FOUND;
}

template<bool update_zobrist> void undo_test_move(Position& position, Move& prev_move) {

    undo_move<update_zobrist>(prev_move);
    restore_zobrist_en_passant_and_castling<update_zobrist>(prev_move);
    
    turn = ((turn == BLACK) ? WHITE : BLACK);
    if (!move_record.empty()) {
        move_record.pop_back();
    }

    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_black_turn;
        if (!position.board_record.empty()) {
            position.board_record.pop_back();
        }
    }
}

template<bool update_zobrist> bool make_test_move(Position& position, Move& move) {

    assert(position.board[move.get_from_square()] != EMPTY_SQUARE);
    int to_square = move.get_to_square();
    int from_square = move.get_from_square();
    int current_turn = move.get_turn();
    uint8_t move_type = move.get_move_type();

    update_zobrist_en_passant<update_zobrist>(move);

    if (move_type == PROMOTION) {
        //std::cout << "promoting pawn\n";
    
        position.move_piece<update_zobrist>(move.get_piece(), from_square, to_square, current_turn);
        uint8_t promotion_piece = convert_promotion_piece(move, move.get_promotion_piece());
        position.replace_piece<update_zobrist>(current_turn, move.get_piece(), promotion_piece, to_square);

        position.update_castling_flags<update_zobrist>(move);
        //game.piece_selected = EMPTY_SQUARE;
        turn = WHITE + BLACK - turn; // flip the turn
        if constexpr (update_zobrist) {
            zobrist_hash ^= zobrist_black_turn;
            board_record.push_back(zobrist_hash);
        }
        move_record.push_back(move);
        uint8_t king_piece = (turn == WHITE) ? WHITE_KING : BLACK_KING;
        if (bitboards.bitboards[king_piece] == 0) {
        position.undo_test_move<update_zobrist>(move);
        return false; // Move is illegal (King is dead)
        }

        if (position.is_square_attacked( 
            __builtin_ctzll(bitboards.bitboards[(turn == WHITE) ? WHITE_KING : BLACK_KING]), current_turn)) {
            position.undo_test_move<update_zobrist>(move); 
            return false; 
        }
        return true;
    }

    //std::cout << "just before moving piece\n";
    assert(from_square != to_square);
    move_piece<update_zobrist>(move.get_piece(), from_square, to_square, current_turn);

    if (move_type == CASTLING) {
        uint8_t piece = (current_turn == WHITE) ? WHITE_ROOK : BLACK_ROOK;
        int row = 7 - (to_square >> 3);
        if (to_square - from_square == 2) {
            move_piece<update_zobrist>(piece, 56 - 8 * row + 7, 56 - 8 * row + 5, current_turn);
        } else if (to_square - from_square == -2) {
            move_piece<update_zobrist>(piece, 56 - 8 * row, 56 - 8 * row + 3, current_turn);
        }
    }

    if (move_type == EN_PASSANT) {
        int captured_square = ((turn == WHITE) ? to_square - 8 : to_square + 8);
        int opposing_turn = ((turn == WHITE)) ? BLACK : WHITE;
        uint8_t captured_piece = (turn == WHITE) ? BLACK_PAWN : WHITE_PAWN;
        remove_piece<update_zobrist>(opposing_turn, captured_piece, captured_square);
    } 
    update_castling_flags<update_zobrist>(move);
    turn = ((turn == WHITE) ? BLACK : WHITE);

    if constexpr (update_zobrist) {
        zobrist_hash ^= zobrist_black_turn;
        board_record.push_back(zobrist_hash);
    }
    move_record.push_back(move);

    if (is_square_attacked(
        __builtin_ctzll(bitboards.bitboards[(current_turn == WHITE) ? WHITE_KING : BLACK_KING]), current_turn)) {
        undo_test_move<update_zobrist>(move); 
        return false; 
    }
    return true;
}

void Position::make_null_move(int& stored_ep_square, uint64_t& stored_hash) {
    stored_ep_square = en_passant_square;
    stored_hash = zobrist_hash;

    turn = (turn == WHITE) ? BLACK : WHITE;
    zobrist_hash ^= zobrist_black_turn;

    if (en_passant_square != -1) {
        int file = en_passant_square % 8;
        zobrist_hash ^= zobrist_en_passant[file];
        en_passant_square = -1;
    }
}

void Position::undo_null_move(int stored_ep_square, uint64_t stored_hash) {
    zobrist_hash = stored_hash;
    en_passant_square = stored_ep_square;
    turn = (turn == WHITE) ? BLACK : WHITE;
}

void generate_computer_move(Position& position) {
    if (thinking_in_progress) {
        return;
    }
    thinking_in_progress = true;
    positions_searched = 0;
    Position position_copy = position;
    auto start = std::chrono::steady_clock::now();
    std::thread computer_thread([position_copy, start]() mutable {
        Move chosen_move = get_best_move(position_copy, 2000);
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
        if (game.position.board[chosen_move.get_to_square()] != EMPTY_SQUARE) {
            chosen_move.set_captured(game.position.board[chosen_move.get_to_square()]);
        }
        int result = game.position.validate_move(chosen_move);
        make_game_move(game, result, chosen_move);
        if (game.ui.promoting_pawn) {
            chosen_move.set_promotion_piece(game.ui.piece_selected);
            handle_pawn_promotion(game, chosen_move);
        }
        verify_board_sync(game.position);
        verify_zobrist_sync(game.position);
        is_game_over(game);
    }
    finished = false;
}

Move get_best_move(Position& position, int search_allocated_time_ms, int search_depth) {
    Move current_best_move {};
    Move overall_best_move {};
    search_start_time = std::chrono::steady_clock::now();
    terminate_search = false;
    nodes_searched = 0, positions_searched = 0;
    int overall_best_score, depth_best_score = -50000;
    Move_list possible_moves = determine_possible_moves(position);

    for (int depth { 1 }; depth <= search_depth; depth++) {
        int seldepth = 0;
        depth_best_score = -500000;
        int alpha = -500000, beta = 500000;
        int move_eval = 0;
        int move_num = 0;
        std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
        [&](Move& move1, Move& move2) {
            return sort_moves_by_priority(position, move1) > sort_moves_by_priority(position, move2);
        });
        for (int i { 1 }; i < possible_moves.num_moves; i++) {
            if (possible_moves.list[i] == current_best_move) {
                std::swap(possible_moves.list[i], possible_moves.list[0]);
            }
        }
        current_best_move = possible_moves.list[0];

        for (int i { 0 }; i < possible_moves.num_moves; i++) {
            auto& move = possible_moves.list[i];

            if (!position.make_test_move<true>(move)) {
                continue;
            };

            move_eval = find_eval(position, move_num, depth, beta, alpha, search_allocated_time_ms, 1, seldepth);
            move_num++;
            position.undo_test_move<true>(move);

            if (terminate_search) {
                break; 
            }
            //std::cout << "depth:" <<  depth << " e: " << move << ' ' << move_eval << ' ' << game.turn << '\n';
        
            alpha = std::max(move_eval, alpha);
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
                      << " score " << format_score(overall_best_score)  
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
int negamax(Position& position, int depth, int alpha, int beta, int search_allocated_time_ms, int ply,
    int& seldepth) { 
    nodes_searched++;
    if (ply > seldepth) {
        seldepth = ply;
    }
    if (determine_repetition(position)) {
        return 0;
    }
    Move stored_move {};
    int stored_eval = probe_transposition_table(position.zobrist_hash, depth, alpha, beta, stored_move, ply);
    if (stored_eval != NO_ENTRY_FOUND) {
        return stored_eval;
    }
    if (terminate_search) {
        return 0;
    }

    uint8_t piece = (position.turn == WHITE) ? WHITE_KING : BLACK_KING;
    int king_square = __builtin_ctzll(position.bitboards.bitboards[piece]);
    bool in_check = position.is_square_attacked(king_square, position.turn);
    int extension = 0;
    if (in_check && ply <= 50) {
        extension = 1;
    }

    if (depth + extension == 0) {
        return quiescence_search(position, alpha, beta, ply, seldepth);
    }
    if ((nodes_searched & 2047) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
        if (elapsed > search_allocated_time_ms) {
            terminate_search = true;
            return 0;
        }
    }
    bool has_major_pieces = false;
    if (position.turn == WHITE) {
        has_major_pieces = (position.bitboards.bitboards[WHITE_KNIGHT] | position.bitboards.bitboards[WHITE_BISHOP] | 
            position.bitboards.bitboards[WHITE_ROOK] | position.bitboards.bitboards[WHITE_QUEEN]);
    } else {
        has_major_pieces = (position.bitboards.bitboards[BLACK_KNIGHT] | position.bitboards.bitboards[BLACK_BISHOP] | 
            position.bitboards.bitboards[BLACK_ROOK] | position.bitboards.bitboards[BLACK_QUEEN]);
    }

    if (depth >= 3 && !in_check && ply > 0 && has_major_pieces) {
        int stored_ep_square;
        uint64_t stored_hash;
        int reduction = 2;
        position.make_null_move(stored_ep_square, stored_hash);
        int eval = -negamax(position, depth - 1 - reduction, -beta, -beta + 1, search_allocated_time_ms, 
            ply + 1, seldepth);
        position.undo_null_move(stored_ep_square, stored_hash);
        if (terminate_search) {
            return 0;
        }
        if (eval >= beta) {
            return beta;
        }
    }
    int num_legal_moves = 0;
    Move_list possible_moves = determine_possible_moves(position);
    uint8_t king = (position.turn == WHITE) ? WHITE_KING : BLACK_KING;

    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        if (move1 == stored_move) {
            return true;
        }
        if (move2 == stored_move) {
            return false;
        }
        return sort_moves_by_priority(position, move1) > sort_moves_by_priority(position, move2);
    });
 
    int max_eval = -600000;
    int move_eval;
    int original_alpha = alpha;
    Move best_move_this_node {};

    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        auto& possible_move = possible_moves.list[i];

        if (!position.make_test_move<true>(possible_move)) {
            continue;
        };
        move_eval = find_eval(position, num_legal_moves, depth + extension, beta, alpha, 
            search_allocated_time_ms, ply + 1, seldepth);
        num_legal_moves++;
        position.undo_test_move<true>(possible_move);

        if (move_eval > max_eval) {
            max_eval = move_eval;
            best_move_this_node = possible_move;
        }

        alpha = std::max(move_eval, alpha);
        if (alpha >= beta) {
            break;
        } 
    }

    if (num_legal_moves == 0) {
        int king_square = __builtin_ctzll(position.bitboards.bitboards[king]);
        if (position.is_square_attacked(king_square, position.turn)) {
            return -400000 + ply;
        } else {
            return 0;
        }
    }

    if (!terminate_search) {
        tt_flag flag;
        if (max_eval <= original_alpha) {
            flag = tt_flag::tt_alpha;
        } else if (max_eval >= beta) {
            flag = tt_flag::tt_beta;
        } else {
            flag = tt_flag::tt_exact;
        }
        record_entry(position.zobrist_hash, max_eval, depth, flag, best_move_this_node, ply);
    }
    return max_eval;
}

// principal variation search
inline int find_eval(Position& position, int move_num, int depth, int beta, int alpha, int search_allocated_time_ms,
    int ply, int& seldepth) {
    if (terminate_search) {
        return 0;
    }
    int move_eval;
    if (move_num == 0) {
        move_eval = -negamax(position, depth - 1, -beta, -alpha, search_allocated_time_ms, ply, seldepth);
    } else {
        move_eval = -negamax(position, depth - 1, -alpha - 1, -alpha, search_allocated_time_ms, ply, seldepth);
        if (move_eval > alpha && move_eval < beta) {
            move_eval = -negamax(position, depth - 1, -beta, -alpha, search_allocated_time_ms, ply, seldepth);
        }
    }
    return move_eval;
}

// evaluation function, based on material and piece square tables
int evaluate(Position& position) {
    int eval { 0 };
    position.value_white_pieces = position.value_black_pieces = 0;
    for (int piece { 1 }; piece < 6; piece++) {
        int current_material = __builtin_popcountll(position.bitboards.bitboards[piece]) * piece_values[piece];
        position.value_white_pieces += current_material;
        eval += current_material;
    }
    for (int piece { 9 }; piece < 14; piece++) {
        int current_material = __builtin_popcountll(position.bitboards.bitboards[piece]) * piece_values[piece - 8];
        position.value_black_pieces += current_material;
        eval -= current_material;
    }
    for (int piece { 1 }; piece <= 6; piece++) {
        eval += positional_eval(position, position.bitboards.bitboards[piece], piece);
    }
    for (int piece { 9 }; piece <= 14; piece++) {
        eval -= positional_eval(position, position.bitboards.bitboards[piece], piece - 8, true);
    }
    return eval;
}

__attribute__((always_inline)) int positional_eval(Position& position, uint64_t bitboard, uint8_t piece, bool black) {
    int eval { 0 };
    double material_phase = (8000 - position.value_white_pieces - position.value_black_pieces) / 8000.0;
    int idx = piece - 1;
    while (bitboard) {
        int square = (!black) ? __builtin_ctzll(bitboard) : __builtin_ctzll(bitboard) ^ 56;
        eval += (start_value_tables[idx][square] + material_phase *
        (endgame_value_tables[idx][square] - start_value_tables[idx][square]));
        bitboard &= bitboard - 1;
    }
    return eval;
}

int sort_moves_by_priority(Position& position, Move& move) {
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
        (8000 - position.value_white_pieces - position.value_black_pieces) / 8000.0 *
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
int quiescence_search(Position& position, int alpha, int beta, int ply, int& seldepth) {
    int stand_pat = evaluate(position) * ((position.turn == WHITE) ? 1 : -1);
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
    Move_list possible_moves = generate_captures_only(position);
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(position, move1) > sort_moves_by_priority(position, move2);
    });
    //std::cout << possible_moves.num_moves << '\n';
    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        if (!position.make_test_move<true>(possible_moves.list[i])) {
            continue;
        }
        int score = -quiescence_search(position, -beta, -alpha, ply + 1, seldepth);
        position.undo_test_move<true>(possible_moves.list[i]);
        if (score >= beta) {
            return beta;
        } 
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

// generates all possible pseudolegal moves (does not care if it leaves king in check)
Move_list determine_possible_moves(Position& position) {
    Move_list moves;
    if (position.turn == WHITE) {
        add_pawn_moves<WHITE, false>(position, moves);
        add_knight_moves<WHITE, false>(position, moves);
        add_bishop_moves<WHITE, false>(position, moves);
        add_rook_moves<WHITE, false>(position, moves);
        add_queen_moves<WHITE, false>(position, moves);
        add_king_moves<WHITE, false>(position, moves);
    } else {
        add_pawn_moves<BLACK, false>(position, moves);
        add_knight_moves<BLACK, false>(position, moves);
        add_bishop_moves<BLACK, false>(position, moves);
        add_rook_moves<BLACK, false>(position, moves);
        add_queen_moves<BLACK, false>(position, moves);
        add_king_moves<BLACK, false>(position, moves);
    }
    return moves;
}

// generates all possible pseudolegal capture moves (does not care if it leaves king in check)
Move_list generate_captures_only(Position& position) {
    Move_list moves;
    if (position.turn == WHITE) {
        add_pawn_moves<WHITE, true>(position, moves);
        add_knight_moves<WHITE, true>(position, moves);
        add_bishop_moves<WHITE, true>(position, moves);
        add_rook_moves<WHITE, true>(position, moves);
        add_queen_moves<WHITE, true>(position, moves);
        add_king_moves<WHITE, true>(position, moves);
    } else {
        add_pawn_moves<BLACK, true>(position, moves);
        add_knight_moves<BLACK, true>(position, moves);
        add_bishop_moves<BLACK, true>(position, moves);
        add_rook_moves<BLACK, true>(position, moves);
        add_queen_moves<BLACK, true>(position, moves);
        add_king_moves<BLACK, true>(position, moves);
    }
    return moves;
}

template void Position::undo_test_move<false>(Move& prev_move);
template void Position::undo_test_move<true>(Move& prev_move);
template bool Position::make_test_move<false>(Move& move);
template bool Position::make_test_move<true>(Move& move);

