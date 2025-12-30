#include "logic.h"
#include <iostream>

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

uint64_t find_rook_attacks(int square, Bitboards& bitboards) {
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
            if (bitboards.occupied & mask) {
                break;
            }
        }
    }
    return attacks;
}

uint64_t find_bishop_attacks(int square, Bitboards& bitboards) {
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
            if (bitboards.occupied & mask) {
                break;
            }
        }
    }
    return attacks;
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

void undo_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& prev_move, int turn) {
    int mover = prev_move.turn;
    int opposing_turn = ((mover == white) ? black : white);
    switch_move(prev_move);
    int promoted_piece = queen;
    int original_piece = prev_move.piece; 
    if (prev_move.special_move == promotion) {
        if (board[prev_move.prev_square].piece_occupying.piece_type != none) {
            promoted_piece = board[prev_move.prev_square].piece_occupying.piece_type;
            prev_move.piece = promoted_piece;
        } else {
            std::cout << "an error occurred\n";
        }
    }
    move_piece(game, bitboards, board, prev_move, true);
    prev_move.piece = original_piece;

    if (prev_move.special_move != en_passant && prev_move.piece_taken.piece_type != none) {
        board[prev_move.prev_square].piece_occupying = prev_move.piece_taken;
        int from_square = prev_move.prev_square;
        uint64_t mask = 1ULL;
        bitboards.bitboards[opposing_turn][prev_move.piece_taken.piece_type] |= (mask << from_square);
        bitboards.update_occupied();
    }
    //int captured_row = ((turn == black) ? prev_move.prev_row + 1 : prev_move.prev_row - 1);
    if (prev_move.special_move == en_passant) {
        int captured_square = ((prev_move.turn == white) ? prev_move.prev_square - 8 : prev_move.prev_square + 8);
        board[captured_square].piece_occupying = prev_move.piece_taken;
        assert(prev_move.piece_taken.piece_type == pawn);
        uint64_t mask = 1ULL;
        int from_square = captured_square;
        bitboards.bitboards[opposing_turn][pawn] |= (mask << from_square);
        bitboards.update_occupied();
    }
    int castling_row = ((prev_move.turn == black) ? 0 : 7);
    if (prev_move.special_move == castling) {
        if (prev_move.prev_square - prev_move.new_square == 2) {
            Move move { 56 - 8 * castling_row + 5, 56 - 8 * castling_row + 7, prev_move.turn, rook };
            move_piece(game, bitboards, board, move, true);
        } else {
            Move move { 56 - 8 * castling_row + 3, 56 - 8 * castling_row, prev_move.turn, rook };
            move_piece(game, bitboards, board, move, true);
        }
    }
    if (prev_move.special_move == promotion) {
        uint64_t mask = 1ULL;
        int to_square = prev_move.new_square;
        int piece = board[to_square].piece_occupying.piece_type;
        int colour = prev_move.turn;
        bitboards.bitboards[colour][promoted_piece] &= ~(mask << to_square);
        bitboards.bitboards[colour][pawn] |= (mask << to_square);
        board[prev_move.new_square].piece_occupying.piece_type = pawn;

        bitboards.update_occupied();
    }
}


void undo_game_move(Game& game) {
    if (game.move_record.size() == 0) {
        return;
    }
    //std::cout << "size: " << game.move_record.size() << '\n';
    Move prev_move = game.move_record[game.move_record.size() - 1];
    std::cout << game.move_record.size() - 1 << '\n';
    int turn = (((game.move_record.size() - 1) % 2 == 0) ? black : white); 

    undo_move(game, game.bitboards, game.board, prev_move, turn);
    if (prev_move.piece_taken.piece_type == none && prev_move.piece != pawn) {
        if (game.plys_to_100 > 0) {
            game.plys_to_100--;
        }
    }
  
    game.castling_rights = prev_move.castling_rights;
    
    // std::cout << "previous: " << std::bitset<8>(game.castling_rights) << '\n';
    //std::cout << "here now\n";
    //std::cout << game.board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type << '\n';
    if (game.board_record.size() > 0) {
        game.board_record.pop_back();
    }
    game.move_record.pop_back();
    game.turn = ((game.turn == white) ? black : white);
    evaluate_king_checks(game);
    //std::cout << "undo done\n";
}

void undo_test_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& prev_move) {
    int turn = prev_move.turn;
    undo_move(game, bitboards, board, prev_move, turn);
    game.castling_rights = prev_move.castling_rights;
    if (!game.move_record.empty()) {
        game.move_record.pop_back();
    }

    game.turn = ((game.turn == black) ? white : black);
    //std::cout << game.board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type << '\n';
}

void make_game_move(Game& game, Bitboards& bitboards, Chessboard& board, int result, Move move) {
    
    //std::cout << "moving\n";
    if (move.piece == pawn || board[move.new_square].piece_occupying.piece_type != none) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }

    if ((game.turn == black && board[move.prev_square].piece_occupying.piece_type == pawn 
        && 7 - move.prev_square / 8 == 6) || 
        (game.turn == white && board[move.prev_square].piece_occupying.piece_type == pawn 
        && 7 - move.prev_square / 8 == 1) &&
        7 <= std::abs(move.new_square - move.prev_square) && std::abs(move.new_square - move.prev_square) <= 9) {
            game.promoting_pawn = true;
            return;
    }

    if (move.prev_square != move.new_square) {
        move_piece(game, bitboards, board, move);
    }   

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == white) { 
            Move move { 7, 5, white, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 2 && game.turn == white) {
            Move move { 0, 3, white, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 1 && game.turn == black) {
            Move move { 63, 61, black, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 2 && game.turn == black) {
            Move move { 56, 59, black, rook };
            move_piece(game, bitboards, board, move);
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
        bitboards.update_occupied();
        game.board_record.clear();
    } 
    update_castling_flags(game, bitboards, move);
    game.move_record.push_back(move);
    
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void update_castling_flags(Game& game, Bitboards& bitboards, Move& move) {
    uint64_t mask = 1ULL;
    move.castling_rights = game.castling_rights;
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
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void make_test_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& move) {

    if (board[move.prev_square].piece_occupying.piece_type == none) {
        std::cout << move << '\n';
    }
    //std::cout << "processing move\n";
    //std::cout << "moving\n";
    //std::cout << "moving\n";
    if (move.piece == pawn || board[move.new_square].piece_occupying.piece_type != none) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }

    if (move.special_move == promotion) {
        //std::cout << "promoting pawn\n";
        game.promoting_pawn = true;
        move_piece(game, bitboards, board, move);
        u_int64_t mask = 1ULL;
        int shift = move.new_square;
        u_int64_t square = mask << shift;
        //move.special_move = promotion;

        board[move.new_square].piece_occupying.piece_type = move.promoted_piece;
        if (board[move.new_square].piece_occupying.piece_type == none) {
            std::cout << "something went wrong\n";
        }
        bitboards.bitboards[move.turn][pawn] &= (~square);
        bitboards.bitboards[move.turn][move.promoted_piece] |= (square);
        bitboards.update_occupied();
        update_castling_flags(game, bitboards, move);
        game.move_record.push_back(move);
        game.piece_selected = -1;
        game.promoting_pawn = false;
        game.turn = ((game.turn == white) ? black : white);
        return;
    }

    //std::cout << "just before moving piece\n";
    if (move.prev_square != move.new_square) {
        move_piece(game, bitboards, board, move);
    }   
    if (move.special_move == castling) {
        int row = 7 - move.new_square / 8;
        if (move.new_square - move.prev_square == 2) {
            Move rook_move { 56 - 8 * row + 7, 56 - 8 * row + 5, move.turn, rook };
            move_piece(game, bitboards, board, rook_move);
        } else if (move.new_square - move.prev_square == -2) {
            Move rook_move { 56 - 8 * row, 56 - 8 * row + 3, move.turn, rook };
            move_piece(game, bitboards, board, rook_move);
        }
    }

    if (move.special_move == en_passant) {
        //int captured_row = ((move.turn == white) ? move.new_row + 1 : move.new_row - 1);
        int captured_square = ((game.turn == white) ? move.new_square - 8 : move.new_square + 8);
        int opposing_turn = ((game.turn == white)) ? black : white;
        u_int64_t mask = (1ULL << captured_square);
        move.piece_taken = board[captured_square].piece_occupying;
        board[captured_square].piece_occupying = { none, none};
        bitboards.bitboards[opposing_turn][pawn] &= (~mask);
        bitboards.update_occupied();
    } 
    update_castling_flags(game, bitboards, move);
    game.move_record.push_back(move);
    game.turn = ((game.turn == white) ? black : white);
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void generate_computer_move(Game& game) {
    if (thinking_in_progress) {
        return;
    }
    thinking_in_progress = true;
    
    Bitboards bitboards_copy = game.bitboards;
    Chessboard copy = game.board;
    positions_searched = 0;
    Game game_copy = game;
    auto start = std::chrono::steady_clock::now();
    std::thread computer_thread([copy, bitboards_copy, game_copy, start]() mutable {
        Move chosen_move = get_best_move(game_copy, bitboards_copy, copy, 6);
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

void update_computer_move(Game& game) {
    if (finished) {
        Move chosen_move = calculated_move;
        std::cout << "best move calculated: ";
        std::cout << chosen_move.prev_square << " -> " << chosen_move.new_square << '\n';
        //std::cout << "nr: " << chosen_move.new_row << "nc: " << chosen_move.new_col << " piece:" << chosen_move.piece << '\n';
        if (game.board[chosen_move.new_square].piece_occupying.piece_type != none) {
            chosen_move.piece_taken = game.board[chosen_move.new_square].piece_occupying;
        }
        int result = validate_move(game, game.bitboards, game.board, chosen_move);
        make_game_move(game, game.bitboards, game.board, result, chosen_move);
        if (game.promoting_pawn) {
            game.piece_selected = chosen_move.promoted_piece;
            handle_pawn_promotion(game, game.bitboards, game.board, chosen_move);
        }

        game.turn = ((chosen_move.turn == white) ? black : white);
        is_game_over(game);
    }
    finished = false;
}

Move get_best_move(Game& game, Bitboards& bitboards, Chessboard& copy, int depth) {
    int best_score = -500000;
    Move best_move {};
    int turn = game.turn;
    //std::cout << "generating\n";
    Move_list possible_moves = determine_possible_moves(game, bitboards, copy, turn, true);
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(move1) > sort_moves_by_priority(move2);
    });

    //std::cout << possible_moves.size() << '\n';
    //std::cout << "here2\n";

    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        //std::cout << "testing possible moves\n";
        auto& move = possible_moves.list[i];
        make_test_move(game, bitboards, copy, move);
        int mover = (game.turn == white) ? black : white;
        int king_square = __builtin_ctzll(bitboards.bitboards[mover][king]);
        if (is_square_attacked(bitboards, king_square, mover)) {
            undo_test_move(game, bitboards, copy, move);
            continue;
        }
        //std::cout << "here3\n";
        
        int move_eval = -negamax(game, bitboards, copy, depth - 1, -500000, 500000);
        move_eval += (std::rand() % 5) - 2;

        std::cout << "e: " << move_eval << ' ' << turn << '\n';
       
        if (move_eval > best_score) {
            best_score = move_eval;
            best_move = move;
        }
    
        if (best_move.new_square == best_move.prev_square) {
            best_move = move;
        }
        undo_test_move(game, bitboards, copy, move);
        turn = game.turn;
        
    }
    std::cout << "Best score: " << best_score << '\n';
    return best_move;
}

int negamax(Game& game, Bitboards& bitboards, Chessboard& board, int depth, int alpha, int beta) { 

    //std::cout << "minimaxing\n";
    if (depth == 0) {
        //std::cout << "reached depth 0\n";
        positions_searched++;
        int perspective = (game.turn == white) ? 1 : -1;
        return perspective * evaluate(bitboards);
    }
    
    Move_list possible_moves = determine_possible_moves(game, bitboards, board, game.turn, true);
    std::sort(possible_moves.list.begin(), possible_moves.list.begin() + possible_moves.num_moves, 
    [&](Move& move1, Move& move2) {
        return sort_moves_by_priority(move1) > sort_moves_by_priority(move2);
    });
    //std::cout << "size: " << possible_moves.size() << '\n';
    if (possible_moves.num_moves == 0) {
        Move default_move { 0, 0, game.turn, -1 };
        int in_check = check_checks(game, bitboards, board, default_move);
        if (in_check) {
            return -400000 + depth * 50;
        } else {
            return 0;
        }
    }
 
    int max_eval = -600000;
    for (int i { 0 }; i < possible_moves.num_moves; i++) {
        auto& possible_move = possible_moves.list[i];
        make_test_move(game, bitboards, board, possible_move);
        int mover = (game.turn == white) ? black : white;
        int king_square = __builtin_ctzll(bitboards.bitboards[mover][king]);
        if (is_square_attacked(bitboards, king_square, mover)) {
            undo_test_move(game, bitboards, board, possible_move);
            continue;
        }

        int eval = -negamax(game, bitboards, board, depth - 1, -beta, -alpha);
        undo_test_move(game, bitboards, board, possible_move);
        alpha = std::max(eval, alpha);
        //compare_bitboards(saved_bitboards, bitboards);
        //verify_board_sync(game);
        max_eval = std::max(eval, max_eval);
        if (beta <= alpha) {
            break;
        } 
    }
    return max_eval;
}

int evaluate(Bitboards& bitboards) {
    int eval { 0 };
    
    eval += __builtin_popcountll(bitboards.bitboards[white][pawn]) * piece_values[pawn];
    eval += __builtin_popcountll(bitboards.bitboards[white][knight]) * piece_values[knight];
    eval += __builtin_popcountll(bitboards.bitboards[white][bishop]) * piece_values[bishop];
    eval += __builtin_popcountll(bitboards.bitboards[white][rook]) * piece_values[rook];
    eval += __builtin_popcountll(bitboards.bitboards[white][queen]) * piece_values[queen];
    eval -= __builtin_popcountll(bitboards.bitboards[black][pawn]) * piece_values[pawn];
    eval -= __builtin_popcountll(bitboards.bitboards[black][knight]) * piece_values[knight];
    eval -= __builtin_popcountll(bitboards.bitboards[black][bishop]) * piece_values[bishop];
    eval -= __builtin_popcountll(bitboards.bitboards[black][rook]) * piece_values[rook];
    eval -= __builtin_popcountll(bitboards.bitboards[black][queen]) * piece_values[queen];
    eval += positional_eval(bitboards.bitboards[white][pawn], white_pawn_square_table);
    eval += positional_eval(bitboards.bitboards[white][knight], knights_table);
    eval += positional_eval(bitboards.bitboards[white][bishop], bishop_table);
    eval += positional_eval(bitboards.bitboards[white][rook], rook_table);
    eval += positional_eval(bitboards.bitboards[white][queen], queen_table);
    eval -= positional_eval(bitboards.bitboards[black][pawn], black_pawn_square_table);
    eval -= positional_eval(bitboards.bitboards[black][knight], knights_table);
    eval -= positional_eval(bitboards.bitboards[black][bishop], bishop_table);
    eval -= positional_eval(bitboards.bitboards[black][rook], rook_table);
    eval -= positional_eval(bitboards.bitboards[black][queen], queen_table);
    if (bitboards.bitboards[black][queen] != 0 && bitboards.bitboards[white][queen] != 0) {
        eval += positional_eval(bitboards.bitboards[white][king], king_table_beginning_white);
        eval -= positional_eval(bitboards.bitboards[black][king], king_table_beginning_black);
    } else {
        eval += positional_eval(bitboards.bitboards[white][king], king_table_endgame_white);
        eval -= positional_eval(bitboards.bitboards[black][king], king_table_endgame_black);
    }
    return eval;
}

int positional_eval(uint64_t bitboard, const int table[]) {
    int eval { 0 };
    while (bitboard) {
        int square = __builtin_ctzll(bitboard);
        eval += table[square];
        bitboard &= bitboard - 1;
    }
    return eval;
}

int sort_moves_by_priority(Move& move) {
    if (move.piece_taken.piece_type != none) {
        return 10000 + piece_values[move.piece_taken.piece_type] - piece_values[move.piece];
    }
    if (move.special_move == promotion) {
        return 9000;
    }
    if (move.special_move == castling) {
        return 4000;
    }
    return 0;
}

void is_game_over(Game& game) {
    evaluate_king_checks(game);
    record_board(game);
    
    Bitboards bitboards_copy = game.bitboards;
    Chessboard copy = game.board;
    Game game_copy = game;
    Move_list moves = determine_possible_moves(game_copy, bitboards_copy, copy, game.turn);
    //std::cout << moves.size() << '\n';
    //std::cout << "plys to 100: " << game.plys_to_100 << '\n';
    if (moves.num_moves > 0) {
        std::cout << moves.num_moves << '\n';
    }
    if (moves.num_moves == 0 || determine_repetition(game) == -1 || 
        determine_insufficient_material(game) == -1 || game.plys_to_100 == 100) {
        end_game(game);
    }
}

int determine_insufficient_material(Game& game) {
    if (game.value_black_pieces <= 3 && game.value_white_pieces <= 3 && !game.pawns_on_board) {
        game.insufficient_material = true;
        return -1;
    }
    return 0;
}

int determine_repetition(Game& game) {
    int occurrences { 1 };
    int latest_move { static_cast<int>(game.board_record.size() - 1)};
    for (int i { latest_move - 1 }; i >= 0; i--) {
        if (game.board_record[i] == game.board_record[latest_move]) {
            occurrences++;
        }
        if (occurrences == 3) {
            game.repetition = true;
            return -1;
        }
    }
    return 0;
}

void handle_pawn_promotion(Game& game, Bitboards& bitboards, Chessboard& board, Move& move) {
    move_piece(game, bitboards, board, move);
    u_int64_t mask = 1ULL;
    int shift = move.new_square;
    u_int64_t square = mask << shift;
    move.special_move = promotion;
    
    board[move.new_square].piece_occupying.piece_type = game.piece_selected;
    if (board[move.new_square].piece_occupying.piece_type == none) {
        std::cout << "something went wrong\n";
    }
    bitboards.bitboards[game.turn][pawn] &= (~square);
    bitboards.bitboards[game.turn][game.piece_selected] |= (square);
    bitboards.update_occupied();
    update_castling_flags(game, game.bitboards, move);
    game.move_record.push_back(move);
    game.piece_selected = none;
    game.promoting_pawn = false;

}

void evaluate_king_checks(Game& game) {
    Move move { 0, 0, game.turn, -1, { none, none } };
    if (game.turn == white) {
        int new_result = check_checks(game, game.bitboards, game.board, move);
        if (new_result == 1) {
            game.white_in_check = true;
        } else {
            game.white_in_check = false;
        }
    } else {
        int new_result = check_checks(game, game.bitboards, game.board, move);
        if (new_result == 1) {
            game.black_in_check = true;
        } else {
            game.black_in_check = false;
        }
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

void move_piece(Game& game, Bitboards& bitboards, Chessboard& board, Move& move, bool undo) {

    int piece = move.piece;
    int captured { -1 };
    captured = board[move.new_square].piece_occupying.piece_type;

    board[move.new_square].piece_occupying = board[move.prev_square].piece_occupying;
    board[move.prev_square].piece_occupying = { none, none };

    uint64_t from_bit = 1ULL << move.prev_square; 
    uint64_t to_bit = 1ULL << move.new_square;
    int opposing_turn = ((move.turn == white) ? black : white);

    bitboards.bitboards[move.turn][piece] &= ~from_bit;
    bitboards.bitboards[move.turn][piece] |= to_bit;
    if (captured != -1) {
        bitboards.bitboards[opposing_turn][captured] &= ~to_bit;
    }
    
    bitboards.update_occupied();
    //print_bitboard(bitboards.occupied);

    if (board[move.new_square].piece_occupying.piece_type == none) {
        //print_all_bitboards(bitboards);
        std::cout << move << '\n';
        std::cout << "failed\n";
        return;
    }
}



int validate_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& move, bool only_checking_checks) {
    int result { 0 };
    if (move.new_square < 0 || move.new_square > 63) {
        return -1;
    }
    //std::cout << move.prev_row << ' ' << move.prev_col << '\n';
    //std::cout << move.new_row << ' ' << move.new_col << '\n';
    //std::cout << std::boolalpha << (move.prev_row != move.new_row) || (move.prev_col != move.new_col) << '\n';
    if ((move.prev_square != move.new_square) && 
        (board[move.new_square].piece_occupying.piece_type == none || 
        board[move.new_square].piece_occupying.colour != move.turn)) {
        
        if (move.piece == pawn) {
            result = validate_move_pawn(game, bitboards, move);
        } else if (move.piece == knight) {
            result = validate_move_knight(bitboards, move);
        } else if (move.piece == bishop) {
            result = validate_move_bishop(bitboards, move);
        } else if (move.piece == rook) {
            result = validate_move_rook(bitboards, move);
        } else if (move.piece == queen) {
            result = validate_move_queen(bitboards, move);
        } else if (move.piece == king) {
            result = validate_move_king(game, bitboards, move);
        } else {
            return 0;
        }
        if (result >= 0 && !only_checking_checks) {
            if (check_checks(game, bitboards, board, move) == 0) {
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

int validate_move_pawn(Game& game, Bitboards& bitboards, Move& move) {
    int square = move.new_square;
    uint64_t mask = 1ULL;

    if (move.turn == white && 32 <= move.prev_square && move.prev_square <= 39 
        && std::abs(move.new_square % 8 - move.prev_square % 8) == 1 && 
        (std::abs(move.new_square - move.prev_square) == 7 || 
        std::abs(move.new_square - move.prev_square) == 9) && (~bitboards.occupied & mask << square)) {
        return validate_en_passant(game, bitboards, move);
    } else if (move.turn == black && 24 <= move.prev_square && move.prev_square <= 31 
        && std::abs(move.new_square % 8 - move.prev_square % 8) == 1 && 
        (std::abs(move.new_square - move.prev_square) == 7 || 
        std::abs(move.new_square - move.prev_square) == 9)&& (~bitboards.occupied & mask << square)) {
        return validate_en_passant(game, bitboards, move);  
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
            if (mask << to & bitboards.black_occupied) {
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
            if (mask << to & bitboards.white_occupied) {
                return 0;
            }
        }
        return -1;
    }
}

int validate_en_passant(Game& game, Bitboards& bitboards, Move& move) {
    //std::cout << "here\n";
    
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

int validate_move_king(Game &game, Bitboards& bitboards, Move& move) {
    if (game.bitboards.king_moves[move.prev_square] & 1ULL << move.new_square) {
        return 0;
    } else if (move.prev_square / 8 == move.new_square / 8 && std::abs(move.new_square - move.prev_square) == 2) {
        Bitboards bitboard_copy = bitboards;
        return test_castling(game, bitboard_copy, move);
    }
    return -1;
}

int check_checks(Game& game, Bitboards& bitboards, Chessboard& board_copy, Move& move) {

    Chessboard copy = board_copy;
    Bitboards bitboard_copy = bitboards;
    
    int opposing_colour = ((move.turn == white) ? black : white);
    
    if (move.prev_square != move.new_square) {
        move_piece(game, bitboard_copy, copy, move);
    }
    
    int square = __builtin_ctzll(bitboard_copy.bitboards[move.turn][king]);
    return is_square_attacked(bitboard_copy, square, move.turn);
    
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
    uint64_t bishop_attacks = find_bishop_attacks(square, bitboard_copy);
    uint64_t rook_attacks = find_rook_attacks(square, bitboard_copy);
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

int test_castling(Game &game, Bitboards& bitboard_copy, Move& move) {

    uint64_t castle_mask_right_w = (1ULL << 5) | (1ULL << 6);
    uint64_t castle_mask_left_w = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
    uint64_t castle_mask_right_b = (1ULL << 62) | (1ULL << 61);
    uint64_t castle_mask_left_b = (1ULL << 59) | (1ULL << 58) | (1ULL << 57);
    uint8_t mask = 1ULL;
    int prev_square = move.prev_square;
    int new_square = move.new_square;
    //print_bitboard(bitboard_copy.occupied);
    if (move.turn == white && prev_square == 4 && !is_square_attacked(bitboard_copy, 4, move.turn)) {
        if (new_square == 2 && (mask << 2 & game.castling_rights)) {
            if ((bitboard_copy.occupied & castle_mask_left_w) == 0) {
                if (!is_square_attacked(bitboard_copy, 3, move.turn) && !is_square_attacked(bitboard_copy, 2, move.turn)) {
                    return 2;
                }
            }
        } else if (new_square == 6 && (mask << 3 & game.castling_rights)) {
            if ((bitboard_copy.occupied & castle_mask_right_w) == 0) {
                if (!is_square_attacked(bitboard_copy, 5, move.turn) && !is_square_attacked(bitboard_copy, 6, move.turn)) {
                    return 1;
                }
            }
        }
    } else if (move.turn == black && prev_square == 60 && !is_square_attacked(bitboard_copy, 60, move.turn)) { 
        if (new_square == 58 && (mask & game.castling_rights)) {
            if ((bitboard_copy.occupied & castle_mask_left_b) == 0) {
                if (!is_square_attacked(bitboard_copy, 59, move.turn) && !is_square_attacked(bitboard_copy, 58, move.turn)) {
                    return 2;
                }
            }
        } else if (new_square == 62 && (mask << 1 & game.castling_rights)) {
            if ((bitboard_copy.occupied & castle_mask_right_b) == 0) {
                if (!is_square_attacked(bitboard_copy, 61, move.turn) && !is_square_attacked(bitboard_copy, 62, move.turn)) {
                    return 1;
                }
            }
        }
    }
    return -1;
}

Move_list determine_possible_moves(Game& game, Bitboards& bitboards, Chessboard& board, int turn, bool CPU) {
    //std::string board_positions {};
    Move_list moves {};
    uint64_t mask = 1ULL;
    uint64_t current_pieces = bitboards.occupied_tables[turn];
    
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        int opposing_turn = ((turn == white) ? black : white);
        if (mask << from_square & bitboards.bitboards[turn][pawn]) {
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
                            if (!CPU) {
                                if (check_checks(game, bitboards, board, move) != 1) {
                                    moves.list[moves.num_moves++] = move;
                                    return moves;
                                }
                            } else {
                                moves.list[moves.num_moves++] = move;
                            }
                        }
                    } else {
                        Move move { from_square, to_square, turn, pawn, board[to_square].piece_occupying };
                        if (!CPU) {
                            if (check_checks(game, bitboards, board, move) != 1) {
                                moves.list[moves.num_moves++] = move;
                                return moves;
                            }
                        } else {
                            moves.list[moves.num_moves++] = move;
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
                            if (!CPU) {
                                if (check_checks(game, bitboards, board, move) != 1) {
                                    moves.list[moves.num_moves++] = move;
                                    return moves;
                                }
                            } else {
                                //std::cout << "move pushed\n";
                                moves.list[moves.num_moves++] = move;
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
                            if (!CPU) {
                                if (check_checks(game, bitboards, board, move) != 1) {
                                    moves.list[moves.num_moves++] = move;
                                    return moves;
                                }
                            } else {
                                moves.list[moves.num_moves++] = move;
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
                            if (!CPU) {
                                if (check_checks(game, bitboards, board, move) != 1) {
                                    moves.list[moves.num_moves++] = move;
                                    return moves;
                                }
                            } else {
                                moves.list[moves.num_moves++] = move;
                            }
                        }
                    } else {
                        Move move { from_square, to_square, turn, pawn, board[to_square].piece_occupying  };
                        if (!CPU) {
                            if (check_checks(game, bitboards, board, move) != 1) {
                                moves.list[moves.num_moves++] = move;
                                return moves;
                            }
                        } else if (std::abs(from_square - to_square) == 8) {
                            moves.list[moves.num_moves++] = move;
                        } else if (std::abs(from_square - to_square) == 16) {
                            if ((turn == white && mask << to_square & ~bitboards.occupied) && 
                            (mask << (to_square - 8) & ~bitboards.occupied) && mask << from_square & RANK_2) {
                                moves.list[moves.num_moves++] = move;
                            } else if ((turn == black && mask << to_square & ~bitboards.occupied) && 
                            (mask << (to_square + 8) & ~bitboards.occupied) && mask << from_square & RANK_7) {
                                moves.list[moves.num_moves++] = move;
                            }
                        }
                    }
                }
                non_captures &= non_captures - 1;
            }
        }
        if (mask << from_square & bitboards.bitboards[turn][knight]) {
            uint64_t knight_moves = bitboards.knight_attacks[from_square];
            while (knight_moves) {
                int to_square = __builtin_ctzll(knight_moves);
                if (mask << to_square & ~bitboards.occupied_tables[turn]) {
                    Move move { from_square, to_square, turn, knight, board[to_square].piece_occupying };
                    if (!CPU) {
                        if (check_checks(game, bitboards, board, move) != 1) {
                            moves.list[moves.num_moves++] = move;
                            return moves;
                        }
                    } else {
                        moves.list[moves.num_moves++] = move;
                    }
                }
                knight_moves &= knight_moves - 1;
            }
        }
        if (mask << from_square & bitboards.bitboards[turn][bishop]) {
            uint64_t bishop_attacks = find_bishop_attacks(from_square, bitboards) & ~bitboards.occupied_tables[turn];
            while (bishop_attacks) {
                int to_square = __builtin_ctzll(bishop_attacks);
                Move move { from_square, to_square, turn, bishop, board[to_square].piece_occupying };
                if (!CPU) {
                    if (check_checks(game, bitboards, board, move) != 1) {
                        moves.list[moves.num_moves++] = move;
                        return moves;
                    }
                } else {
                    moves.list[moves.num_moves++] = move;
                }
                bishop_attacks &= bishop_attacks - 1;
            }
        }
        if (mask << from_square & bitboards.bitboards[turn][rook]) {
            uint64_t rook_attacks = find_rook_attacks(from_square, bitboards) & ~bitboards.occupied_tables[turn];
            while (rook_attacks) {
                int to_square = __builtin_ctzll(rook_attacks);
                Move move { from_square, to_square, turn, rook, board[to_square].piece_occupying };
                if (!CPU) {
                    if (check_checks(game, bitboards, board, move) != 1) {
                        moves.list[moves.num_moves++] = move;
                        return moves;
                    }
                } else {
                    moves.list[moves.num_moves++] = move;
                }
                rook_attacks &= rook_attacks - 1;
            }
        }
        if (mask << from_square & bitboards.bitboards[turn][queen]) {
            uint64_t bishop_attacks = find_bishop_attacks(from_square, bitboards) & ~bitboards.occupied_tables[turn];
            while (bishop_attacks) {
                int to_square = __builtin_ctzll(bishop_attacks);
                Move move { from_square, to_square, turn, queen, board[to_square].piece_occupying };
                if (!CPU) {
                    if (check_checks(game, bitboards, board, move) != 1) {
                        moves.list[moves.num_moves++] = move;
                        return moves;
                    }
                } else {
                    moves.list[moves.num_moves++] = move;
                }
                bishop_attacks &= bishop_attacks - 1;
            }
            uint64_t rook_attacks = find_rook_attacks(from_square, bitboards) & ~bitboards.occupied_tables[turn];
            while (rook_attacks) {
                int to_square = __builtin_ctzll(rook_attacks);
                Move move { from_square, to_square, turn, queen, board[to_square].piece_occupying };
                if (!CPU) {
                    if (check_checks(game, bitboards, board, move) != 1) {
                        std::cout << "here\n";
                        moves.list[moves.num_moves++] = move;
                        return moves;
                    }
                } else {
                    moves.list[moves.num_moves++] = move;
                }
                
                rook_attacks &= rook_attacks - 1;
            }
        }
        if (mask << from_square & bitboards.bitboards[turn][king]) {
            uint64_t king_moves = bitboards.king_moves[from_square];
            while (king_moves) {
                int to_square = __builtin_ctzll(king_moves);
                if (mask << to_square & ~bitboards.occupied_tables[turn]) {
                    //print_bitboard(bitboards.occupied_tables[turn]);
                    Move move { from_square, to_square, turn, king, board[to_square].piece_occupying };
                    if (!CPU) {
                        if (check_checks(game, bitboards, board, move) != 1) {
                            moves.list[moves.num_moves++] = move;
                            return moves;
                        }
                    } else {
                        moves.list[moves.num_moves++] = move;
                    }
                }
                king_moves &= king_moves - 1;
            }
            if (from_square == 4 && turn == white) {
                Move move1 { from_square, 6, turn, king, { none, none }, castling };
                Move move2 { from_square, 2, turn, king, { none, none }, castling };
                if (test_castling(game, bitboards, move1) == 1) {
                    moves.list[moves.num_moves++] = move1;
                }
                if (test_castling(game, bitboards, move2) == 2) {
                    moves.list[moves.num_moves++] = move2;
                    //std::cout << "here\n";
                }
            } else if (from_square == 60 && turn == black) {
                Move move1 { from_square, 62, turn, king, { none, none }, castling };
                Move move2 { from_square, 58, turn, king, { none, none }, castling };
                if (test_castling(game, bitboards, move1) == 1) {
                    moves.list[moves.num_moves++] = move1;
                }
                if (test_castling(game, bitboards, move2) == 2) {
                    moves.list[moves.num_moves++] = move2;
                }
            }
        }
        current_pieces &= current_pieces - 1;
    }
    //std::cout << "here now\n";
    return moves;
}

void record_board(Game& game) {
    std::string board_positions {};
    bool black_en_passant { false }, white_en_passant { false };
    bool black_castling { false }, white_castling { false };
    game.value_black_pieces = game.value_white_pieces = 0;
    game.pawns_on_board = false;
    for (int i { 0 }; i < 64; i++) {
        
            if (game.board[i].piece_occupying.piece_type != none) {
                board_positions += '[';
                board_positions += std::to_string(i);
                board_positions += std::to_string(game.board[i].piece_occupying.piece_type);
                board_positions += std::to_string(game.board[i].piece_occupying.colour);
                board_positions += ']';
                record_piece_points(game, game.board[i].piece_occupying.piece_type, 
                    game.board[i].piece_occupying.colour);
            }   
            if (/*game.board[i][j].piece_occupying &&*/ game.board[i].piece_occupying.piece_type == pawn) {
                if (56 - i / 8 == 3 && !white_en_passant) {
                    Move move1 { i, i + 7, white, pawn };
                    Move move2 { i, i + 9, white, pawn };
                    if (validate_move(game, game.bitboards, game.board, move1) == 3 || 
                        validate_move(game, game.bitboards, game.board, move2) == 3) {
                        white_en_passant = true;
                    }
                }
                if (i == 4 && !black_en_passant) {
                    Move move1 { i, i - 7, black, pawn };
                    Move move2 { i, i - 9, black, pawn };
                    if (validate_move(game, game.bitboards, game.board, move1) == 3 || 
                        validate_move(game, game.bitboards, game.board, move2) == 3) {
                        black_en_passant = true;
                    }                   
                }
            }
            if (/*game.board[i][j].piece_occupying &&*/ game.board[i].piece_occupying.piece_type == king) {
                if (i == 4 && !white_castling) {
                    Move move1 { i, 6, white, king };
                    Move move2 { i, 2, white, king };
                    if (validate_move(game, game.bitboards, game.board, move1) == 1 || 
                        validate_move(game, game.bitboards, game.board, move2) == 2) {
                        white_castling = true;
                    }
                }
                if (i == 60 && !black_castling) {
                    Move move1 { i, 62, black, king };
                    Move move2 { i, 58, black, king };
                    if (validate_move(game, game.bitboards, game.board, move1) == 1 || 
                        validate_move(game, game.bitboards, game.board, move2) == 2) {
                        black_castling = true;
                    }                
                }
            }
  
    }
    if (white_castling) {
        board_positions += "[wc=t]";
    }
    if (black_castling) {
        board_positions += "[bc=t]";
    }
    if (white_en_passant) {
        board_positions += "[wep=t]";
    }
    if (black_en_passant) {
        board_positions += "[bep=t]";
    }
    //std::cout << board_positions << '\n';
    game.board_record.push_back(board_positions);
}

void record_piece_points(Game& game, int piece_type, int piece_colour) {
    int points { 0 };
    if (piece_type == knight || piece_type == bishop) {
        points = 3;
    } else if (piece_type == pawn) {
        points = 1;
        game.pawns_on_board = true;
    } else if (piece_type == rook) {
        points = 5;
    } else if (piece_type == queen) {
        points = 9;
    }
    if (piece_colour == white) {
        game.value_white_pieces += points;
    } else {
        game.value_black_pieces += points;
    }
}





