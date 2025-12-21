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

int create_encoded_move(int from, int to, int flags) {
    return (flags << 12) | (to << 6) | from;
}

int get_from(Encoded_move move) {
    return move & MASK_FROM;
}

int get_to(Encoded_move move) {
    return (move >> 6) & MASK_TO;
}

int get_flags(Encoded_move move) {
    return (move >> 12) & MASK_TO;
}

void find_valid_white_pawn_moves(Game& game) {
    uint64_t non_capture_moves = 0ULL;
    uint64_t double_move = 0ULL;
    uint64_t single_move = 0ULL;
    uint64_t capture_moves = 0ULL;
    single_move |= game.bitboards.white_pawns << 8 & ~game.bitboards.occupied;
    double_move |= (single_move << 8 & RANK_4 & ~game.bitboards.occupied);
    capture_moves |= (game.bitboards.white_pawns << 7 & ~FILE_A & game.bitboards.black_occupied);
    capture_moves |= (game.bitboards.white_pawns << 9 & ~FILE_A & game.bitboards.black_occupied);
    non_capture_moves = single_move | double_move;
}

void find_valid_black_pawn_moves(Game& game) {
    uint64_t non_capture_moves = 0ULL;
    uint64_t double_move = 0ULL;
    uint64_t single_move = 0ULL;
    uint64_t capture_moves = 0ULL;
    single_move |= game.bitboards.black_pawns >> 8 & ~game.bitboards.occupied;
    double_move |= (single_move >> 8 & RANK_5 & ~game.bitboards.occupied);
    capture_moves |= (game.bitboards.black_pawns >> 7 & ~FILE_A & game.bitboards.white_occupied);
    capture_moves |= (game.bitboards.black_pawns >> 9 & ~FILE_A & game.bitboards.white_occupied);
    non_capture_moves = single_move | double_move;
}

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
    int opposing_turn = ((turn == white) ? black : white);
    switch_move(prev_move);
    move_piece(game, bitboards, board, prev_move, true);
    switch_move(prev_move);
    if (prev_move.special_move != "en passant" && prev_move.piece_taken != nullptr) {
        board[prev_move.new_row][prev_move.new_col].piece_occupying = prev_move.piece_taken;
        int from_square = (7 - prev_move.new_row) * 8 + (prev_move.new_col);
        uint64_t mask = 1ULL;
        bitboards.bitboards[turn][prev_move.piece_taken->piece_type] |= (mask << from_square);
        bitboards.update_occupied();
    }
    int captured_row = ((turn == black) ? prev_move.new_row + 1 : prev_move.new_row - 1);
    if (prev_move.special_move == "en passant") {
        board[captured_row][prev_move.new_col].piece_occupying = prev_move.piece_taken;
        uint64_t mask = 1ULL;
        int from_square = (7 - captured_row) * 8 + (prev_move.new_col);
        bitboards.bitboards[turn][0] |= (mask << from_square);
    }
    int castling_row = ((prev_move.turn == black) ? 0 : 7);
    if (prev_move.special_move == "castling") {
        if (prev_move.new_col - prev_move.prev_col == 2) {
            Move move { castling_row, 5, castling_row, 7, prev_move.turn, rook };

            move_piece(game, bitboards, board, move);
        } else {
            Move move { castling_row, 3, castling_row, 0, prev_move.turn, rook };
            move_piece(game, bitboards, board, move);
        }
    }
    if (prev_move.special_move == "promotion") {
        uint64_t mask = 1ULL;
        int from_square = (7 - prev_move.prev_row) * 8 + (prev_move.prev_col);
        int to_square = (7 - prev_move.new_row) * 8 + (prev_move.new_col);
        int piece = board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type;
        int player = prev_move.turn;
        bitboards.bitboards[player][piece] &= ~(mask << to_square);
        bitboards.bitboards[player][pawn] |= (mask << from_square);
        board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type = pawn;

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
    if (prev_move.piece_taken == nullptr && prev_move.piece != pawn) {
        if (game.plys_to_100 > 0) {
            game.plys_to_100--;
        }
    }
    if (game.move_record.size() > 2) {
        Move prev_prev_move = game.move_record[game.move_record.size() - 2];
        game.castling_rights = prev_prev_move.castling_rights;
    }
    std::cout << "previous: " << std::bitset<8>(game.castling_rights) << '\n';
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

void undo_test_move(Game& game, Chessboard& board, Move& prev_move) {

    int turn = ((prev_move.turn == black) ? white : black);
   //undo_move(game, board, prev_move, turn);
    if (!game.move_record.empty()) {
        game.move_record.pop_back();
    }
    game.turn = ((game.turn == black) ? white : black);
    //std::cout << game.board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type << '\n';
}

void make_game_move(Game& game, Bitboards& bitboards, Chessboard& board, int result, Move move) {
    
    //std::cout << "moving\n";
    if (move.piece == pawn || board[move.new_row][move.new_col].piece_occupying) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }
    int row = move.prev_row, col = move.prev_col;

    if ((game.turn == black && board[row][col].piece_occupying->piece_type == pawn && row == 6) || 
        (game.turn == white && board[row][col].piece_occupying->piece_type == pawn && row == 1) &&
        std::abs(move.new_row - move.prev_row) == 1) {
    
            game.promoting_pawn = true;
            return;
    }

    if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
        move_piece(game, bitboards, board, move);
    }   

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == white) { 
            Move move { 7, 7, 7, 5, white, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 2 && game.turn == white) {
            Move move { 7, 0, 7, 3, white, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 1 && game.turn == black) {
            Move move { 0, 7, 0, 5, black, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 2 && game.turn == black) {
            Move move { 0, 0, 0, 3, black, rook };
            move_piece(game, bitboards, board, move);
        }
        game.board_record.clear();
        move.special_move = "castling";
    } else if (result == 3) {
        
        int captured_row = ((game.turn == white) ? move.new_row + 1 : move.new_row - 1);
        int opposing_turn = ((game.turn == white)) ? black : white;
        int shift = 56 - (captured_row) * 8 + move.new_col;
        u_int64_t mask = (1ULL << shift);
        move.special_move = "en passant";
        move.piece_taken = board[captured_row][move.new_col].piece_occupying;
        board[captured_row][move.new_col].piece_occupying = nullptr;
        bitboards.bitboards[opposing_turn][0] &= (~mask);
        game.board_record.clear();
    } 
    update_castling_flags(game, game.bitboards, move);
    game.move_record.push_back(move);
    
    //std::cout << std::bitset<8>(game.castling_rights) << '\n';
    if (board[move.new_row][move.new_col].piece_occupying->piece_type == king) {
        if (game.turn == white) {
            game.white_king_position.row = move.new_row;
            game.white_king_position.col = move.new_col;
        } else {
            game.black_king_position.row = move.new_row;
            game.black_king_position.col = move.new_col;
        }
    }
}

void update_castling_flags(Game& game, Bitboards& bitboards, Move& move) {
    uint64_t mask = 1ULL;
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
        game.castling_rights &= ~(mask << 3);
    }
    if (~bitboards.bitboards[white][rook] & mask << 7) {
        game.castling_rights &= ~(mask << 2); 
    }  
    move.castling_rights = game.castling_rights;
    std::cout << std::bitset<8>(game.castling_rights) << '\n';
}

void make_test_move(Game& game, Bitboards& bitboards, Chessboard& board, int result, Move& move) {
    //std::cout << "processing move\n";
    if (board[move.prev_row][move.prev_col].piece_occupying == nullptr) {
        std::cout << "error before\n";
    }
    //std::cout << move.prev_row << ' ' << move.prev_col << ' ' << move.new_row << ' ' << move.new_col << '\n';
 
    int row = move.prev_row, col = move.prev_col;
    if (!board[row][col].piece_occupying) {
        std::cout << "failed here\n";
    }
    if ((move.turn == black && board[row][col].piece_occupying->piece_type == pawn && row == 6) || 
        (move.turn == white && board[row][col].piece_occupying->piece_type == pawn && row == 1) &&
        std::abs(move.new_row - move.prev_row) == 1) {
        if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
            move_piece(game, bitboards, board, move);
            move.special_move = "promotion";
            board[move.new_row][move.new_col].piece_occupying->piece_type = queen;
            game.move_record.push_back(move);
            game.turn = ((game.turn == black) ? white : black);
        }
        return;
    }
    if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
        move_piece(game, bitboards, board, move);
    }  

    if (board[move.new_row][move.new_col].piece_occupying == nullptr) {
        std::cout << "error after\n";
    }
    //std::cout << "reached here\n";

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == white) { 
            Move move { 7, 7, 7, 5, white, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 2 && game.turn == white) {
            Move move { 7, 0, 7, 3, white, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 1 && game.turn == black) {
            Move move { 0, 7, 0, 5, black, rook };
            move_piece(game, bitboards, board, move);
        } else if (result == 2 && game.turn == black) {
            Move move { 0, 0, 0, 3, black, rook };
            move_piece(game, bitboards, board, move);
        }
        move.special_move = "castling";
    } else if (result == 3) {
        int captured_row = ((game.turn == white) ? move.new_row + 1 : move.new_row - 1);
        move.special_move = "en passant";
        move.piece_taken = board[captured_row][move.new_col].piece_occupying;
        board[captured_row][move.new_col].piece_occupying = nullptr;

    } 
    if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
        game.move_record.push_back(move);
    }  
    game.turn = ((game.turn == black) ? white : black);

}

void generate_computer_move(Game& game) {
    game.move_ready = false;
    if (thinking_in_progress) {
        return;
    }
    thinking_in_progress = true;

    auto copy = std::make_shared<Chessboard>();

    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying) {
                (*copy)[i][j].piece_occupying = std::make_shared<Piece>(*game.board[i][j].piece_occupying);
            } else {
                (*copy)[i][j].piece_occupying = nullptr;
            }
        }
    }
    positions_searched = 0;
    Game game_copy = game;
    auto start = std::chrono::steady_clock::now();
    std::thread computer_thread([copy, game_copy, start]() mutable {
        Move chosen_move = get_best_move(game_copy, *copy, 4);
        computer_turn = false;
        thinking_in_progress = false;
        finished = true;
        calculated_move = chosen_move;
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        std::cout << duration.count() << '\n';
    });
    computer_thread.detach();
}

void update_computer_move(Game& game) {
    if (finished) {
        Move chosen_move = calculated_move;
        std::cout << "best move calculated: ";
        std::cout << chosen_move.prev_row << ' ' << chosen_move.prev_col << " -> " 
        << chosen_move.new_row << ' ' << chosen_move.new_col << '\n';
        //std::cout << "nr: " << chosen_move.new_row << "nc: " << chosen_move.new_col << " piece:" << chosen_move.piece << '\n';
        if (game.board[chosen_move.new_row][chosen_move.new_col].piece_occupying) {
            chosen_move.piece_taken = game.board[chosen_move.new_row][chosen_move.new_col].piece_occupying;
        }
        int result = validate_move(game, game.bitboards, game.board, chosen_move);
        make_game_move(game, game.bitboards, game.board, result, chosen_move);

        game.turn = ((game.turn == white) ? black : white);
        is_game_over(game);
    }
    game.move_ready = false;
    finished = false;
}

Move get_best_move(Game& game, Chessboard& copy, int depth) {
    int best_score = (game.turn == white) ? -500000 : 500000;
    Move best_move {};
    int turn = game.turn;
    //std::cout << "generating\n";
    std::vector<Move> possible_moves = determine_possible_moves(game, copy, turn, true);
    //std::cout << "here2\n";

    for (auto move: possible_moves) {
        if (copy[move.new_row][move.new_col].piece_occupying) {
            move.piece_taken = copy[move.new_row][move.new_col].piece_occupying;
        }

        int result = validate_move(game, game.bitboards, copy, move);
        if (result >= 0) {
            //make_test_move(game, copy, result, move);
            //std::cout << "here3\n";
            bool maximising = ((turn == white) ? true : false);
            
            int move_eval = minimax(game, copy, depth - 1, -500000, 500000, !maximising);
            std::cout << "e: " << move_eval << ' ' << turn << '\n';
            if (turn == white) {
                if (move_eval > best_score) {
                    best_score = move_eval;
                    best_move = move;
                }
            } else {
                if (move_eval < best_score) {
                    best_score = move_eval;
                    best_move = move;
                }
            }
            undo_test_move(game, copy, move);
            turn = game.turn;
        }
    }
    std::cout << "Best score: " << best_score << '\n';
    return best_move;
}

int minimax(Game& game, Chessboard& board, int depth, int alpha, int beta, bool maximising) {
    //std::cout << "minimaxing\n";
    if (depth == 0) {
        //std::cout << "reached depth 0\n";
        positions_searched++;
        return evaluate(board);
    }
    //std::cout << "depth: " << depth << '\n';
    int turn = ((maximising == true) ? white : black);

    std::vector<Move> possible_moves = determine_possible_moves(game, board, turn, true);
    //std::cout << "size: " << possible_moves.size() << '\n';
    if (possible_moves.size() == 0) {
        Move default_move { 0, 0, 0, 0, turn, -1 };
        //int in_check = check_checks(game, board, default_move);
        /*if (in_check) {
            return maximising ? -400000 : 400000;
        } else {
            return 0;
        }*/
    }
    if (maximising) {
        if (turn != white) {
            std::cout << "a bug occurred\n";
        }
        int max_eval = -500000;
        
        for (Move possible_move: possible_moves) {
            if (board[possible_move.new_row][possible_move.new_col].piece_occupying) {
                possible_move.piece_taken = board[possible_move.new_row][possible_move.new_col].piece_occupying;
            }
            int result = validate_move(game, game.bitboards, board, possible_move);
            if (result >= 0) {
                //make_test_move(game, board, result, possible_move);
    
                int eval = minimax(game, board, depth - 1, alpha, beta, false);
                undo_test_move(game, board, possible_move);
                alpha = std::max(eval, alpha);
                max_eval = std::max(eval, max_eval);
                if (beta <= alpha) {
                    break;
                }
            }
        }
        return max_eval;
    } else {
        if (turn != black) {
            std::cout << "a bug occurred\n";
        }
        //game.turn = black;
        int min_eval = 500000;
        for (Move possible_move: possible_moves) {
            if (board[possible_move.new_row][possible_move.new_col].piece_occupying) {
                possible_move.piece_taken = board[possible_move.new_row][possible_move.new_col].piece_occupying;
            }
            int result = validate_move(game, game.bitboards, board, possible_move);
            if (result >= 0) {
                //make_test_move(game, board, result, possible_move);

                int eval = minimax(game, board, depth - 1, alpha, beta, true);
                undo_test_move(game, board, possible_move);
                beta = std::min(eval, beta);
                min_eval = std::min(eval, min_eval);
                if (beta <= alpha) {
                    break;
                }
            }
        } 
        return min_eval;    
    } 
}

int evaluate(Chessboard& board) {
    int eval { 0 };
    int points { 0 };
    int pawns, knight_bishops, rooks, queens;
    pawns = knight_bishops = rooks = queens = 0;
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (board[i][j].piece_occupying) {
                if (board[i][j].piece_occupying->piece_type == pawn) {
                    points = 1;
                    pawns++;
                } else if (board[i][j].piece_occupying->piece_type == knight || 
                    board[i][j].piece_occupying->piece_type == bishop) {
                    points = 3;
                    knight_bishops++;
                } else if (board[i][j].piece_occupying->piece_type == rook) {
                    points = 5;
                    rooks++;
                } else if (board[i][j].piece_occupying->piece_type == queen) {
                    points = 9;
                    queens++;
                }
                if (board[i][j].piece_occupying->colour == white) {
                    eval += points;
                } else {
                    eval -= points;
                }
            }
        }
    }
    //std::cout << pawns << ' ' << knight_bishops << ' ' << rooks << ' ' << queens << '\n';
    //std::cout << "eval: " << eval << '\n';
    return eval;
}

void is_game_over(Game& game) {
    evaluate_king_checks(game);
    record_board(game);
    std::vector<Move> moves = determine_possible_moves(game, game.board, game.turn);
    if (moves.size() == 0 || determine_repetition(game) == -1 || 
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
    int shift = 56 - move.new_row * 8 + move.new_col;
    u_int64_t square = mask << shift;
    move.special_move = "promotion";
    
    board[move.new_row][move.new_col].piece_occupying->piece_type = game.piece_selected;
    if (board[move.new_row][move.new_col].piece_occupying == nullptr) {
        std::cout << "something went wrong\n";
    }
    bitboards.bitboards[game.turn][0] &= (~square);
    bitboards.bitboards[game.turn][game.piece_selected] |= (square);
    bitboards.update_occupied();
    update_castling_flags(game, game.bitboards, move);
     game.move_record.push_back(move);
    game.piece_selected = -1;
    game.promoting_pawn = false;

}

void evaluate_king_checks(Game& game) {
    Move move { 0, 0, 0, 0, game.turn, -1, nullptr };
    if (game.turn == white) {
        int next = black;
        int new_result = check_checks(game, game.bitboards, game.board, move);
        if (new_result == 1) {
            game.white_in_check = true;
        } else {
            game.white_in_check = false;
        }
    } else {
        int next = white;
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
    int temp1 = move.prev_col;
    int temp2 = move.prev_row;
    move.prev_col = move.new_col;
    move.prev_row = move.new_row;
    move.new_col = temp1;
    move.new_row = temp2;
}

void move_piece(Game& game, Bitboards& bitboards, Chessboard& board, Move& move, bool undo) {

    int piece = move.piece;
    
    int captured { -1 };
    if (board[move.new_row][move.new_col].piece_occupying) {
        captured = board[move.new_row][move.new_col].piece_occupying->piece_type;
    } 

    board[move.new_row][move.new_col].piece_occupying = nullptr;
    board[move.new_row][move.new_col].piece_occupying = std::move(board[move.prev_row][move.prev_col].piece_occupying);

    int from_square = (7 - move.prev_row) * 8 + (move.prev_col);
    int to_square = (7 - move.new_row) * 8 + (move.new_col);
    uint64_t from_bit = 1ULL << from_square; 
    uint64_t to_bit = 1ULL << to_square;
    int opposing_turn = ((move.turn == white) ? black : white);

    bitboards.bitboards[move.turn][piece] &= ~from_bit;
    bitboards.bitboards[move.turn][piece] |= to_bit;
    if (captured != -1) {
        bitboards.bitboards[opposing_turn][captured] &= ~to_bit;
    }
    
    bitboards.update_occupied();

    if (board[move.new_row][move.new_col].piece_occupying == nullptr) {
        std::cout << "failed\n";
        return;
    }
    board[move.new_row][move.new_col].piece_occupying->row = move.new_row;
    board[move.new_row][move.new_col].piece_occupying->col = move.new_col;
    if (!undo) {
        board[move.new_row][move.new_col].piece_occupying->moves++;
    } else {
        board[move.new_row][move.new_col].piece_occupying->moves--;
    }
}

int validate_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& move, bool only_checking_checks) {
    int result { 0 };
    if (move.new_row > 7 || move.new_col > 7 || move.new_row < 0 || move.new_col < 0) {
        return -1;
    }
    //std::cout << move.prev_row << ' ' << move.prev_col << '\n';
    //std::cout << move.new_row << ' ' << move.new_col << '\n';
    //std::cout << std::boolalpha << (move.prev_row != move.new_row) || (move.prev_col != move.new_col) << '\n';
    if ((move.prev_row != move.new_row || move.prev_col != move.new_col) && 
        (!board[move.new_row][move.new_col].piece_occupying || 
        board[move.new_row][move.new_col].piece_occupying->colour != move.turn)) {
        
        int piece = move.piece;
        if (piece == pawn) {
            result = validate_move_pawn(game, game.bitboards, board, move);
        } else if (piece == knight) {
            result = validate_move_knight(game.bitboards, move);
        } else if (piece == bishop) {
            result = validate_move_bishop(game.bitboards, move);
        } else if (piece == rook) {
            result = validate_move_rook(game.bitboards, move);
        } else if (piece == queen) {
            result = validate_move_queen(game.bitboards, move);
        } else if (piece == king) {
            result = validate_move_king(game, game.bitboards, move);
        } else {
            return 0;
        }
        if (result >= 0 && !only_checking_checks) {
            Chessboard copy {};
            for (int i { 0 }; i < 8; i++) {
                for (int j { 0 }; j < 8; j++) {
                    if (board[i][j].piece_occupying) {
                        copy[i][j].piece_occupying = std::make_shared<Piece>(*board[i][j].piece_occupying);
                    } else {
                        copy[i][j].piece_occupying = nullptr;
                    }
                }
            }
            Bitboards bitboard_copy = game.bitboards;
            if (check_checks(game, bitboard_copy, copy, move) == 0) {
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

int validate_move_pawn(Game& game, Bitboards& bitboards, Chessboard& board, Move& move) {

    if (move.turn == white && move.prev_row == 3 && move.new_row - move.prev_row == -1 && 
        std::abs(move.new_col - move.prev_col) == 1 && !board[move.new_row][move.new_col].piece_occupying) {
        return validate_en_passant(game, board, move);
    } else if (move.turn == black && move.prev_row == 4 && move.new_row - move.prev_row == 1 && 
        std::abs(move.new_col - move.prev_col) == 1 && !board[move.new_row][move.new_col].piece_occupying) {
        return validate_en_passant(game, board, move);  
    }

    int from = (7 - move.prev_row) * 8 + (move.prev_col);
    int to = (7 - move.new_row) * 8 + (move.new_col);
    u_int64_t mask = 1ULL;
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

int validate_en_passant(Game& game, Chessboard& board, Move& move) {
    //std::cout << "here\n";
    int col_position {}, required_prev_row {}, required_new_row {};
    col_position = ((move.new_col - move.prev_col == 1) ? move.prev_col + 1 : move.prev_col - 1);
    required_prev_row = ((move.turn == white) ? 1 : 6);
    required_new_row = ((move.turn == white) ? 3 : 4);
   
    if (board[move.prev_row][col_position].piece_occupying && 
        board[move.prev_row][col_position].piece_occupying->piece_type == pawn) {
        Move prev_move = game.move_record[game.move_record.size() - 1];
        //std::cout << "prev" << prev_move.prev_row << ' ' << prev_move.prev_col << 
        //" Curr" << prev_move.new_row << prev_move.new_col << '\n';
        if (prev_move.piece == pawn && prev_move.prev_row == required_prev_row && 
            prev_move.new_row == required_new_row && prev_move.new_col == col_position) {
            return 3;
        }
        return -1;
    }
    return -1;
}

int validate_move_knight(Bitboards& bitboards, Move& move) {
    int index = 56 - 8 * move.prev_row + move.prev_col;
    int square = 56 - 8 * move.new_row + move.new_col;
    u_int64_t mask = 1ULL << square;
    if (bitboards.knight_attacks[index] & mask) {
        return 0;
    }
    return -1;
}

int validate_move_bishop(Bitboards& bitboards, Move& move) {
    int row_change { move.new_row - move.prev_row };
    int col_change { move.new_col - move.prev_col };
    int from = 56 - 8 * move.prev_row + move.prev_col;
    int to = 56 - 8 * move.new_row + move.new_col;

    if (std::abs(row_change) == std::abs(col_change)) {
        uint64_t path = bitboards.between_table[from][to];
        if (path & bitboards.occupied) {
            return -1;
        }
        return 0;
    }
    return -1;
}

int validate_move_rook(Bitboards& bitboards, Move& move) {
    int row_change { move.new_row - move.prev_row };
    int col_change { move.new_col - move.prev_col };
    int from = 56 - 8 * move.prev_row + move.prev_col;
    int to = 56 - 8 * move.new_row + move.new_col;

    if (row_change == 0 || col_change == 0) {
        uint64_t path = bitboards.between_table[from][to];
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

    int index = 56 - 8 * move.prev_row + move.prev_col;
    int square = 56 - 8 * move.new_row + move.new_col;
    if (game.bitboards.king_moves[index] & 1ULL << square) {
        return 0;
    } else if (move.new_row - move.prev_row == 0 && std::abs(move.new_col - move.prev_col) == 2) {
        Bitboards bitboard_copy = bitboards;
        return test_castling(game, bitboard_copy, move);
    }
    return -1;
}

int check_checks(Game &game, Bitboards& bitboard_copy, Chessboard& copy, Move& move) {
  
    int opposing_colour = ((move.turn == white) ? black : white);
    
    if (move.prev_row != move.new_row || move.prev_col != move.new_col) {
        move_piece(game, bitboard_copy, copy, move);
    }
    
    int square = __builtin_ctzll(bitboard_copy.bitboards[move.turn][king]);
    if (bitboard_copy.knight_attacks[square] & bitboard_copy.bitboards[opposing_colour][knight]) {
        return 1;
    }
    if (bitboard_copy.pawn_attacks[move.turn][square] & bitboard_copy.bitboards[opposing_colour][pawn]) {
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
    int prev_square = 56 - move.prev_row * 8 + move.prev_col;
    int new_square = 56 - move.new_row * 8 + move.new_col;
    //print_bitboard(bitboard_copy.occupied);
    if (move.turn == white && prev_square == 4 && !is_square_attacked(bitboard_copy, 3, move.turn)) {
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

std::vector<Move> determine_possible_moves(Game& game, Chessboard& board, int turn, bool CPU) {
    //std::string board_positions {};
    std::vector<Move> moves {};
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (board[i][j].piece_occupying && board[i][j].piece_occupying->colour == turn) {
                int piece = board[i][j].piece_occupying->piece_type;
                if (board[i][j].piece_occupying->piece_type == pawn && turn == white) {
                    std::vector<std::pair<int, int>> possible_moves { { i - 1, j }, 
                    { i - 2, j }, { i - 1, j + 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece };
                        if (validate_move(game, game.bitboards, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == pawn && turn == black) {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j }, 
                    { i + 2, j }, { i + 1, j + 1 }, { i + 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece};
                        if (validate_move(game, game.bitboards, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }                   
                } else if (board[i][j].piece_occupying->piece_type == knight) {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 2 }, { i + 2, j + 1 }, 
                    { i - 1, j - 2 }, { i - 2, j - 1 }, { i + 1, j - 2 }, 
                    { i - 1, j + 2 }, { i - 2, j + 1 }, { i + 2, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece };
                        if (validate_move(game, game.bitboards, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }  
                } else if (board[i][j].piece_occupying->piece_type == bishop) {
                    std::vector<std::pair<int, int>> directions { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };
                    for (auto pair: directions) {
                        for (int k { 1 }; k < 8; k++) {
                            int new_row { pair.first * k + i };
                            int new_col { pair.second * k + j };
                            if (new_row > 7 || new_row < 0 || new_col > 7 || new_col < 0) {
                                break;
                            }
                            Move move1 { i, j, new_row, new_col, turn, piece };
                            if (validate_move(game, game.bitboards, board, move1) >= 0) {
                                moves.push_back(move1);
                                if (!CPU) {
                                    return moves;
                                }
                            }
                            if (board[new_row][new_col].piece_occupying) {
                                break;
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == rook) {
                    std::vector<std::pair<int, int>> directions { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
                    for (auto pair: directions) {
                        for (int k { 1 }; k < 8; k++) {
                            int new_row { i + pair.first * k };
                            int new_col { j + pair.second * k };
                            if (new_row > 7 || new_row < 0 || new_col > 7 || new_col < 0) {
                                break;
                            }
                            Move move1 { i, j, new_row, new_col, turn, piece };
                            if (validate_move(game, game.bitboards, board, move1) >= 0) {
                                moves.push_back(move1);
                                if (!CPU) {
                                    return moves;
                                }
                            } 
                            if (board[new_row][new_col].piece_occupying) {
                                break;
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == queen) {
                    std::vector<std::pair<int, int>> directions { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }, 
                    { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
                    for (auto pair: directions) {
                        for (int k { 1 }; k < 8; k++) {
                            int new_row { pair.first * k + i };
                            int new_col { pair.second * k + j };
                            if (new_row > 7 || new_row < 0 || new_col > 7 || new_col < 0) {
                                break;
                            }
                            Move move1 { i, j, new_row, new_col, turn, piece };
                            if (validate_move(game, game.bitboards, board, move1) >= 0) {
                                moves.push_back(move1);
                                if (!CPU) {
                                    return moves;
                                }
                            }
                            if (board[new_row][new_col].piece_occupying) {
                                break;
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == king) {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 1 }, { i + 1, j }, 
                    { i, j + 1 }, { i + 1, j - 1 }, { i - 1, j + 1 }, 
                    { i - 1, j }, { i, j - 1 }, { i - 1, j - 1 }, { i, j + 2 }, { i, j - 2 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece };
                        if (validate_move(game, game.bitboards, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }
                }
            }
        }
    }
    return moves;
}

void record_board(Game& game) {
    std::string board_positions {};
    bool black_en_passant { false }, white_en_passant { false };
    bool black_castling { false }, white_castling { false };
    game.value_black_pieces = game.value_white_pieces = 0;
    game.pawns_on_board = false;
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying) {
                board_positions += '[';
                board_positions += std::to_string(i);
                board_positions += std::to_string(j);
                board_positions += std::to_string(game.board[i][j].piece_occupying->piece_type);
                board_positions += std::to_string(game.board[i][j].piece_occupying->colour);
                board_positions += ']';
                record_piece_points(game, game.board[i][j].piece_occupying->piece_type, 
                    game.board[i][j].piece_occupying->colour);
            }   
            if (game.board[i][j].piece_occupying && game.board[i][j].piece_occupying->piece_type == pawn) {
                if (i == 3 && !white_en_passant) {
                    Move move1 { i, j, i - 1, j + 1, white, pawn };
                    Move move2 { i, j, i - 1, j - 1, white, pawn };
                    if (validate_move(game, game.bitboards, game.board, move1) == 3 || 
                        validate_move(game, game.bitboards, game.board, move2) == 3) {
                        white_en_passant = true;
                    }
                }
                if (i == 4 && !black_en_passant) {
                    Move move1 { i, j, i + 1, j + 1, black, pawn };
                    Move move2 { i, j, i + 1, j - 1, black, pawn };
                    if (validate_move(game, game.bitboards, game.board, move1) == 3 || 
                        validate_move(game, game.bitboards, game.board, move2) == 3) {
                        black_en_passant = true;
                    }                   
                }
            }
            if (game.board[i][j].piece_occupying && game.board[i][j].piece_occupying->piece_type == king) {
                if (i == 7 && j == 4 && !white_castling) {
                    Move move1 { i, j, 7, 6, white, king };
                    Move move2 { i, j, 7, 2, white, king };
                    if (validate_move(game, game.bitboards, game.board, move1) == 1 || 
                        validate_move(game, game.bitboards, game.board, move2) == 2) {
                        white_castling = true;
                    }
                }
                if (i == 0 && j == 4 && !black_castling) {
                    Move move1 { i, j, 0, 6, black, king };
                    Move move2 { i, j, 0, 2, black, king };
                    if (validate_move(game, game.bitboards, game.board, move1) == 1 || 
                        validate_move(game, game.bitboards, game.board, move2) == 2) {
                        black_castling = true;
                    }                
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





