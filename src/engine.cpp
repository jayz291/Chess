#include "engine.h"
#include "movegen.h"
#include "logic.h"
#include "perft.h"
#include "uci.h"
#include <iostream>

std::chrono::steady_clock::time_point search_start_time;
std::atomic<bool> terminate_search { false };

void clear_transposition_table() {
    for (int i { 0 }; i < TABLE_SIZE; i++) {
        transposition_table[i].eval = 0;
        transposition_table[i].zobrist_key = 0;
    }
}

void Engine::record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move, int ply) {
    int index = key & (TABLE_SIZE - 1);
    int stored_score = eval;

    // store checkmates with ply adjustments 
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

int Engine::probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move, int ply) {
    int index = key & (TABLE_SIZE - 1);
    table_entry entry = transposition_table[index];
    int return_eval = entry.eval;

    // ensures that the checkmate calculation is relative to the root
    if (return_eval > CHECKMATE_THRESHOLD) {
        return_eval -= ply; 
    } else if (return_eval < -CHECKMATE_THRESHOLD) {
        return_eval += ply;
    }
    if (key == entry.zobrist_key) {
        best_move = entry.best_move;
        // ensures the saved entry is just as accurate, or even more accurate
        if (entry.depth >= depth) {
            if (entry.flag == tt_flag::tt_exact) {
                // the position was searched before and it is not too bad for either player.
                // As the depth is deep enough, return the exact evaluation. 
                return return_eval;
            } else if (entry.flag == tt_flag::tt_alpha && return_eval <= alpha) {
                // the new guaranteed maximum is higher than the previous upper bound stored
                // in the transposition table. Use the new alpha (another move is better for the player)
                return alpha;
            } else if (entry.flag == tt_flag::tt_beta && return_eval >= beta) {
                // the new guaranteed minimum is lower than the previous lower bound stored
                // in the transposition table. The opponent will avoid this position (again)
                return beta;
            }
        }

    }
    return NO_ENTRY_FOUND;
}

template<bool update_zobrist> void Position::undo_test_move(Move& prev_move) {

    undo_move<update_zobrist>(prev_move);
    restore_zobrist_en_passant_and_castling<update_zobrist>(prev_move);
    
    turn = ((turn == BLACK) ? WHITE : BLACK);

    if constexpr (update_zobrist) {
        if (!move_record.empty()) {
            move_record.pop_back();
        }
        zobrist_hash ^= zobrist_black_turn;
        if (!board_record.empty()) {
            board_record.pop_back();
        }
    }
}

template<bool update_zobrist> bool Position::make_test_move(Move& move) {

    assert(board[move.get_from_square()] != EMPTY_SQUARE);
    int to_square = move.get_to_square();
    int from_square = move.get_from_square();
    int current_turn = move.get_turn();
    uint8_t move_type = move.get_move_type();
    int king = (current_turn == WHITE) ? WHITE_KING : BLACK_KING;

    update_zobrist_en_passant<update_zobrist>(move);
    assert(from_square != to_square);
    move_piece<update_zobrist>(move.get_piece(), from_square, to_square, current_turn);

    if (move_type == PROMOTION) {
        uint8_t promotion_piece = convert_promotion_piece(move, move.get_promotion_piece());
        replace_piece<update_zobrist>(current_turn, move.get_piece(), promotion_piece, to_square);
    } else if (move_type == CASTLING) {
        uint8_t piece = (current_turn == WHITE) ? WHITE_ROOK : BLACK_ROOK;
        //int row = 7 - (to_square >> 3);
        int base_square = to_square & 56; // 56 is 111000 in binary
        if (to_square - from_square == 2) {
            move_piece<update_zobrist>(piece, base_square + 7, base_square + 5, current_turn);
        } else if (to_square - from_square == -2) {
            move_piece<update_zobrist>(piece, base_square, base_square + 3, current_turn);
        }
    } else if (move_type == EN_PASSANT) {
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
        move_record.push_back(move);
    }

    if (__builtin_popcountll(bitboards.bitboards[(turn == WHITE) ? WHITE_KING : BLACK_KING]) == 0) {
        undo_test_move<update_zobrist>(move);
        return false;
    }

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
        Engine engine(position_copy, 2000);
        Move chosen_move = engine.get_best_move();
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

void make_computer_move(Game& game, Assets& assets) {
    if (finished) {
        Move chosen_move = calculated_move;
        std::cout << "best move calculated: ";
        std::cout << chosen_move.get_from_square() << " -> " << chosen_move.get_to_square() << '\n';
        if (game.position.board[chosen_move.get_to_square()] != EMPTY_SQUARE) {
            chosen_move.set_captured(game.position.board[chosen_move.get_to_square()]);
        }
        int result = game.position.validate_move(chosen_move);
        game.make_game_move(result, chosen_move);
        if (game.ui.promoting_pawn) {
            chosen_move.set_promotion_piece(game.ui.piece_selected);
            game.handle_pawn_promotion(chosen_move);
        }
        verify_board_sync(game.position);
        verify_zobrist_sync(game.position);
        play_sound(assets, game);
        game.is_game_over();
    }
    finished = false;
}

Move Engine::get_best_move(int search_depth) {
    Move current_best_move {};
    Move overall_best_move {};
    search_start_time = std::chrono::steady_clock::now();
    terminate_search = false;
    nodes_searched = 0, positions_searched = 0;
    int overall_best_score, depth_best_score = -50000;
    MoveGen move_generator(position);
    Move_list possible_moves = move_generator.determine_possible_moves();
    if (possible_moves.num_moves == 0) {
        return current_best_move;
    }
    overall_best_move = possible_moves.list[0];

    for (int depth { 1 }; depth <= search_depth; depth++) {
        int seldepth = 0;
        depth_best_score = -500000;
        int alpha = -500000, beta = 500000;
        int move_eval = 0;
        int move_num = 0;

        // order the moves so that more promising moves are prioritised 
        std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
        [&](Move& move1, Move& move2) {
            return sort_moves_by_priority(move1) > sort_moves_by_priority(move2);
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

            move_eval = find_eval(move_num, depth, beta, alpha, 1, seldepth);
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
        
            // if this occurs, something has gone wrong 
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

int Engine::negamax(int depth, int alpha, int beta, int ply, int& seldepth) { 
    nodes_searched++;
    if (ply > seldepth) {
        seldepth = ply;
    }
    if (determine_repetition()) {
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

    bool in_check = is_in_check();
    int extension = 0;

    // if the king is in check, extend the search 
    if (in_check && ply <= 50) {
        extension = 1;
    }

    if (depth + extension == 0) {
        return quiescence_search(alpha, beta, ply, seldepth);
    }

    if (is_time_over()) {
        terminate_search = true;
        return 0;
    }

    if (depth >= 3 && !in_check && ply > 0 && major_pieces_present()) {
        int stored_ep_square;
        uint64_t stored_hash;
        int reduction = 2;
        position.make_null_move(stored_ep_square, stored_hash);
        int eval = -negamax(depth - 1 - reduction, -beta, -beta + 1, ply + 1, seldepth);
        position.undo_null_move(stored_ep_square, stored_hash);
        if (terminate_search) {
            return 0;
        }
        if (eval >= beta) {
            return beta;
        }
    }
    int num_legal_moves = 0;
    MoveGen move_generator(position);
    Move_list possible_moves = move_generator.determine_possible_moves();
    uint8_t king = (position.turn == WHITE) ? WHITE_KING : BLACK_KING;

    // sorts the moves so that moves likelier to be better are first
    // the move stored from a previous depth is used first
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        if (move1 == stored_move) {
            return true;
        }
        if (move2 == stored_move) {
            return false;
        }
        return sort_moves_by_priority(move1) > sort_moves_by_priority(move2);
    });
 
    int max_eval = -600000;
    int move_eval;
    int original_alpha = alpha;
    Move best_move_this_node {};

    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        auto& possible_move = possible_moves.list[i];

        // if the pseudolegal move is actually illegal, skip to the next move in the list
        if (!position.make_test_move<true>(possible_move)) {
            continue;
        };
        move_eval = find_eval(num_legal_moves, depth + extension, beta, alpha, ply + 1, seldepth);
        num_legal_moves++;
        position.undo_test_move<true>(possible_move);

        if (move_eval > max_eval) {
            max_eval = move_eval;
            best_move_this_node = possible_move;
        }

        // update the alpha to the higher maximum guaranteed score (for the maximising player)
        alpha = std::max(move_eval, alpha);

        // the maximum guaranteed score is higher than the minimum guaranteed score. The
        // minimising player has a better move somewhere else in the tree, so they will not go
        // down this branch - cut off the search here
        if (alpha >= beta) {
            break;
        } 
    }

    if (num_legal_moves == 0) {
        return calculate_checkmate_or_stalemate_eval(king, ply);
    }

    if (!terminate_search) {
        tt_flag flag = set_entry_flag(original_alpha, beta, max_eval);
        record_entry(position.zobrist_hash, max_eval, depth, flag, best_move_this_node, ply);
    }
    return max_eval;
}

inline int Engine::find_eval(int move_num, int depth, int beta, int alpha, int ply, int& seldepth) {
    if (terminate_search) {
        return 0;
    }
    int move_eval;
    if (move_num == 0) {
        // do a full window search on only the first move, to return an exact evaluation score
        move_eval = -negamax(depth - 1, -beta, -alpha, ply, seldepth);
    } else {
        // does a zero-window search
        // beta value: -alpha, alpha value: -alpha - 1
        // if the evaluation is worse than alpha, the move is discarded (not good enough)
        // if it is better, the loop immediately breaks as it is above the beta cutoff
        move_eval = -negamax(depth - 1, -alpha - 1, -alpha, ply, seldepth);

        // re-search condition: if the move evaluation is bigger than alpha and smaller than
        // beta, this means that the move is an improvement for the maximising player (better
        // guaranteed max score), but not so bad that the opposing player will reject it
        // (less than the opposing player's minimum guaranteed score) 
        if (alpha < move_eval && move_eval < beta) {
            move_eval = -negamax(depth - 1, -beta, -alpha, ply, seldepth);
        }
    }
    return move_eval;
}

int Engine::evaluate() {
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
    double material_phase = (8000 - position.value_white_pieces - position.value_black_pieces) / 8000.0;
    for (int piece { 1 }; piece <= 6; piece++) {
        eval += positional_eval(position.bitboards.bitboards[piece], piece, material_phase);
    }
    for (int piece { 9 }; piece <= 14; piece++) {
        eval -= positional_eval(position.bitboards.bitboards[piece], piece - 8, material_phase, true);
    }
    eval += pawn_structure_eval();
    eval += mobility_eval();
    return eval;
}

__attribute__((always_inline)) int Engine::positional_eval(uint64_t bitboard, uint8_t piece, 
    const double& material_phase, bool black) {
    int eval { 0 };
    int idx = piece - 1;
    while (bitboard) {
        int square = (!black) ? __builtin_ctzll(bitboard) : __builtin_ctzll(bitboard) ^ 56;
        eval += (start_value_tables[idx][square] + material_phase *
        (endgame_value_tables[idx][square] - start_value_tables[idx][square]));
        bitboard &= bitboard - 1;
    }
    return eval;
}

int Engine::mobility_eval() {
    int eval { 0 };
    Bitboards& bitboards = position.bitboards;

    uint64_t white_pawns = bitboards.bitboards[WHITE_PAWN];
    uint64_t black_pawns = bitboards.bitboards[BLACK_PAWN];
    uint64_t white_pawn_attacks = 0ULL;
    uint64_t black_pawn_attacks = 0ULL;
    while (white_pawns) {
        white_pawn_attacks |= bitboards.pawn_attacks[WHITE][__builtin_ctzll(white_pawns)];
        white_pawns &= white_pawns - 1;
    }
    while (black_pawns) {
        black_pawn_attacks |= bitboards.pawn_attacks[BLACK][__builtin_ctzll(black_pawns)];
        black_pawns &= black_pawns - 1;
    }

    uint64_t white_knights = bitboards.bitboards[WHITE_KNIGHT];
    uint64_t black_knights = bitboards.bitboards[BLACK_KNIGHT];
    uint64_t white_knight_attacks = 0ULL;
    uint64_t black_knight_attacks = 0ULL;
    while (white_knights) {
        white_knight_attacks |= bitboards.knight_attacks[__builtin_ctzll(white_knights)];
        white_knights &= white_knights - 1;
    }
    while (black_knights) {
        black_knight_attacks |= bitboards.knight_attacks[__builtin_ctzll(black_knights)];
        black_knights &= black_knights - 1;
    }

    uint64_t white_bishops = bitboards.bitboards[WHITE_BISHOP];
    uint64_t black_bishops = bitboards.bitboards[BLACK_BISHOP];
    uint64_t white_bishop_attacks = 0ULL;
    uint64_t black_bishop_attacks = 0ULL;
    while (white_bishops) {
        white_bishop_attacks |= bitboards.get_bishop_attacks(__builtin_ctzll(white_bishops), bitboards.occupied);
        white_bishops &= white_bishops - 1;
    }
    while (black_bishops) {
        black_bishop_attacks |= bitboards.get_bishop_attacks(__builtin_ctzll(black_bishops), bitboards.occupied);
        black_bishops &= black_bishops - 1;
    }

    uint64_t white_rooks = bitboards.bitboards[WHITE_ROOK];
    uint64_t black_rooks = bitboards.bitboards[BLACK_ROOK];
    uint64_t white_rook_attacks = 0ULL;
    uint64_t black_rook_attacks = 0ULL;
    while (white_rooks) {
        white_rook_attacks |= bitboards.get_rook_attacks(__builtin_ctzll(white_rooks), bitboards.occupied); 
        white_rooks &= white_rooks - 1;
    }
    while (black_rooks) {
        black_rook_attacks |= bitboards.get_rook_attacks(__builtin_ctzll(black_rooks), bitboards.occupied); 
        black_rooks &= black_rooks - 1;
    }

    uint64_t white_queens = bitboards.bitboards[WHITE_QUEEN];
    uint64_t black_queens = bitboards.bitboards[BLACK_QUEEN];
    uint64_t white_queen_attacks = 0ULL;
    uint64_t black_queen_attacks = 0ULL;
    while (white_queens) {
        white_queen_attacks |= bitboards.get_bishop_attacks(__builtin_ctzll(white_queens), bitboards.occupied);
        white_queen_attacks |= bitboards.get_rook_attacks(__builtin_ctzll(white_queens), bitboards.occupied); 
        white_queens &= white_queens - 1;
    }
    while (black_queens) {
        black_queen_attacks |= bitboards.get_bishop_attacks(__builtin_ctzll(black_queens), bitboards.occupied);
        black_queen_attacks |= bitboards.get_rook_attacks(__builtin_ctzll(black_queens), bitboards.occupied); 
        black_queens &= black_queens - 1;
    }

    uint64_t white_king_attacks = bitboards.king_moves[__builtin_ctzll(bitboards.bitboards[WHITE_KING])];
    uint64_t black_king_attacks = bitboards.king_moves[__builtin_ctzll(bitboards.bitboards[BLACK_KING])];

    uint64_t white_attacks = white_pawn_attacks | white_knight_attacks | white_bishop_attacks |
    white_rook_attacks | white_queen_attacks | white_king_attacks;
    uint64_t black_attacks = black_pawn_attacks | black_knight_attacks | black_bishop_attacks |
    black_rook_attacks | black_queen_attacks | black_king_attacks;
    eval += 3 * __builtin_popcountll(white_knight_attacks & ~black_attacks);
    eval -= 3 * __builtin_popcountll(black_knight_attacks & ~white_attacks);
    eval += 3 * __builtin_popcountll(white_bishop_attacks & ~black_attacks);
    eval -= 3 * __builtin_popcountll(black_bishop_attacks & ~white_attacks);
    eval += 2 * __builtin_popcountll(white_rook_attacks & ~black_attacks);
    eval -= 2 * __builtin_popcountll(black_rook_attacks & ~white_attacks);
    eval += 2 * __builtin_popcountll(white_queen_attacks & ~black_attacks);
    eval -= 2 * __builtin_popcountll(black_queen_attacks & ~white_attacks);

    // king safety evaluation
    uint64_t white_king_squares = white_king_attacks & bitboards.bitboards[WHITE_KING];
    uint64_t black_king_squares = black_king_attacks & bitboards.bitboards[BLACK_KING];
    eval -= 5 * __builtin_popcountll(white_king_attacks & black_pawn_attacks);
    eval += 5 * __builtin_popcountll(black_king_squares & white_pawn_attacks);
    eval -= 10 * __builtin_popcountll(white_king_squares & 
        (black_knight_attacks | black_bishop_attacks));
    eval += 10 * __builtin_popcountll(black_king_squares & 
        (white_knight_attacks | white_bishop_attacks));
    eval -= 20 * __builtin_popcountll(white_king_squares & black_rook_attacks);
    eval += 20 * __builtin_popcountll(black_king_squares & white_rook_attacks);
    eval -= 40 * __builtin_popcountll(white_king_squares & black_queen_attacks);
    eval += 40 * __builtin_popcountll(black_king_squares & white_queen_attacks);
    return eval;

}

int Engine::pawn_structure_eval() {
    int score = 0;
    uint64_t pawns = position.bitboards.bitboards[WHITE_PAWN];
    uint64_t pawns_copy = pawns;
    while (pawns) {
        int square = __builtin_ctzll(pawns);
        int rank = square / 8;
        int file = square % 8;
        if (((FILE_MASKS[file] | ADJACENT_FILE_MASKS[file]) & WHITE_PASSED_RANK_MASKS[rank] &
            position.bitboards.bitboards[BLACK_PAWN]) == 0) {
            score += RANK_SCORES[rank];
        }
        if (__builtin_popcountll(FILE_MASKS[file] & pawns_copy) > 1) {
            score -= 20;
        }
        if (__builtin_popcountll(ADJACENT_FILE_MASKS[file] & pawns_copy) == 0) {
            score -= 20;
        }
        pawns &= pawns - 1;
    }
    pawns = position.bitboards.bitboards[BLACK_PAWN];
    pawns_copy = pawns;
    while (pawns) {
        int square = __builtin_ctzll(pawns);
        int rank = square / 8;
        int file = square % 8;
        if (((FILE_MASKS[file] | ADJACENT_FILE_MASKS[file] | BLACK_PASSED_RANK_MASKS[rank]) & 
            position.bitboards.bitboards[WHITE_PAWN]) == 0) {
            score -= RANK_SCORES[7 - rank];
        }
        if (__builtin_popcountll(FILE_MASKS[file] & pawns_copy) > 1) {
            score += 20;
        }
        if (__builtin_popcountll(ADJACENT_FILE_MASKS[file] & pawns_copy) == 0) {
            score += 20;
        }
        pawns &= pawns - 1;
    }
    return score;
}

inline bool Engine::major_pieces_present() {
    if (position.turn == WHITE) {
        return (position.bitboards.bitboards[WHITE_KNIGHT] | position.bitboards.bitboards[WHITE_BISHOP] | 
            position.bitboards.bitboards[WHITE_ROOK] | position.bitboards.bitboards[WHITE_QUEEN]);
    } else {
        return (position.bitboards.bitboards[BLACK_KNIGHT] | position.bitboards.bitboards[BLACK_BISHOP] | 
            position.bitboards.bitboards[BLACK_ROOK] | position.bitboards.bitboards[BLACK_QUEEN]);
    }
}

inline bool Engine::is_in_check() {
    uint8_t piece = (position.turn == WHITE) ? WHITE_KING : BLACK_KING;
    int king_square = __builtin_ctzll(position.bitboards.bitboards[piece]);
    return position.is_square_attacked(king_square, position.turn);
}

inline int Engine::calculate_checkmate_or_stalemate_eval(const uint8_t& king, const int& ply) {
    int king_square = __builtin_ctzll(position.bitboards.bitboards[king]);
    if (position.is_square_attacked(king_square, position.turn)) {
        // checkmate in sight: +ply so the computer favours shorter checkmates
        return -400000 + ply;
    } else {
        // stalemate
        return 0;
    }
}

inline tt_flag Engine::set_entry_flag(const int& original_alpha, const int& beta, const int& max_eval) {
    if (max_eval <= original_alpha) {
        // no move improved on the original alpha score from the position. (failed low)
        return tt_flag::tt_alpha;
    } else if (max_eval >= beta) {
        // a move found was too good (evaluation is higher than the opponent's guaranteed min
        // score), so the opponent will avoid this position
        return tt_flag::tt_beta;
    } else {
        // the position found is plausible (not too bad for either player)
        return tt_flag::tt_exact;
    }
}

inline bool Engine::is_time_over() {
    if (((nodes_searched + positions_searched) & 2047) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
        return (elapsed > search_allocated_time_ms);
    } 
    return false;
}

int Engine::sort_moves_by_priority(const Move& move) {
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
        move_score_guess += (1000 * piece_values[captured] - piece_values[piece]) + 1000000;
    }
    if (move.get_move_type() == PROMOTION) {
        move_score_guess += 70 * piece_values[move.get_promotion_piece() + 2];
    } else if (move.get_move_type() == CASTLING) {
        move_score_guess += 100;
    }
    return move_score_guess;
}

int Engine::quiescence_search(int alpha, int beta, int ply, int& seldepth) {

    // score achieved if there are no further changes
    int stand_pat = evaluate() * ((position.turn == WHITE) ? 1 : -1);
    positions_searched++;

    // update the selective depth count
    if (ply > seldepth) {
        seldepth = ply;
    }

    // if the stand pat is above beta, that means that the opponent will avoid this position 
    if (stand_pat >= beta) {
        return beta;
    }

    // if the stand pat is above alpha, set this as the new baseline
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    MoveGen move_generator(position);
    Move_list possible_moves = move_generator.generate_captures_only();
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(move1) > sort_moves_by_priority(move2);
    });
    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        if (!position.make_test_move<true>(possible_moves.list[i])) {
            continue;
        }
        int score = -quiescence_search(-beta, -alpha, ply + 1, seldepth);
        position.undo_test_move<true>(possible_moves.list[i]);
        if (score >= beta) {
            return beta;
        } 
        // raise the baseline if a better score is found
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

bool Engine::determine_repetition() {
    int occurrences { 1 };
    int latest_move { static_cast<int>(position.board_record.size() - 1)};
    for (int i { latest_move - 1 }; i >= 0; i--) {
        if (position.board_record[i] == position.board_record[latest_move]) {
            occurrences++;
        }
        if (occurrences == 2) {
            return true;
        }
    }
    return false;
}

Move_list MoveGen::determine_possible_moves() {
    Move_list moves;
    if (position.turn == WHITE) {
        add_pawn_moves<WHITE, false>(moves);
        add_knight_moves<WHITE, false>(moves);
        add_bishop_moves<WHITE, false>(moves);
        add_rook_moves<WHITE, false>(moves);
        add_queen_moves<WHITE, false>(moves);
        add_king_moves<WHITE, false>(moves);
    } else {
        add_pawn_moves<BLACK, false>(moves);
        add_knight_moves<BLACK, false>(moves);
        add_bishop_moves<BLACK, false>(moves);
        add_rook_moves<BLACK, false>(moves);
        add_queen_moves<BLACK, false>(moves);
        add_king_moves<BLACK, false>(moves);
    }
    return moves;
}

Move_list MoveGen::generate_captures_only() {
    Move_list moves;
    if (position.turn == WHITE) {
        add_pawn_moves<WHITE, true>(moves);
        add_knight_moves<WHITE, true>(moves);
        add_bishop_moves<WHITE, true>(moves);
        add_rook_moves<WHITE, true>(moves);
        add_queen_moves<WHITE, true>(moves);
        add_king_moves<WHITE, true>(moves);
    } else {
        add_pawn_moves<BLACK, true>(moves);
        add_knight_moves<BLACK, true>(moves);
        add_bishop_moves<BLACK, true>(moves);
        add_rook_moves<BLACK, true>(moves);
        add_queen_moves<BLACK, true>(moves);
        add_king_moves<BLACK, true>(moves);
    }
    return moves;
}

template void Position::undo_test_move<false>(Move& prev_move);
template void Position::undo_test_move<true>(Move& prev_move);
template bool Position::make_test_move<false>(Move& move);
template bool Position::make_test_move<true>(Move& move);

