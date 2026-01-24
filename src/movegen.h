#include "definitions.h"
#include "game.h"

template<bool generate_captures_only>
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
        if constexpr (!generate_captures_only) {
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
                                (mask << (to_square - 8) & ~bitboards.occupied) && mask << from_square & RANK_MASKS[1]) {
                                    moves.list[local_counter++] = move;
                                } else if ((turn == BLACK && mask << to_square & ~bitboards.occupied) && 
                                (mask << (to_square + 8) & ~bitboards.occupied) && mask << from_square & RANK_MASKS[6]) {
                                    moves.list[local_counter++] = move;
                                }
                            }
                        }
                    }
                }
                non_captures &= non_captures - 1;
            }
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

template<bool generate_captures_only>
__attribute__((always_inline)) void add_knight_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint8_t piece = (game.turn == WHITE) ? WHITE_KNIGHT : BLACK_KNIGHT;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[(game.turn == WHITE) ? BLACK : WHITE]);
    } else {
        targets = ~game.bitboards.occupied_tables[game.turn];
    }
    int local_counter = moves.num_moves;
    while (current_pieces) {
        //std::cout << "inside here\n";
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t knight_moves = game.bitboards.knight_attacks[from_square];
        while (knight_moves) {
            int to_square = __builtin_ctzll(knight_moves);
            if (mask << to_square & targets) {
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

template<bool generate_captures_only>
__attribute__((always_inline)) void add_bishop_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint8_t piece = (game.turn == WHITE) ? WHITE_BISHOP : BLACK_BISHOP;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[(game.turn == WHITE) ? BLACK : WHITE]);
    } else {
        targets = ~game.bitboards.occupied_tables[game.turn];
    }
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) 
        & targets;
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

template<bool generate_captures_only>
__attribute__((always_inline)) void add_rook_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint8_t piece = (game.turn == WHITE) ? WHITE_ROOK : BLACK_ROOK;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[(game.turn == WHITE) ? BLACK : WHITE]);
    } else {
        targets = ~game.bitboards.occupied_tables[game.turn];
    }
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t rook_attacks = get_rook_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        targets;
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

template<bool generate_captures_only>
__attribute__((always_inline)) void add_queen_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    uint8_t piece = (game.turn == WHITE) ? WHITE_QUEEN : BLACK_QUEEN;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    int to_square;
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[(game.turn == WHITE) ? BLACK : WHITE]);
    } else {
        targets = ~game.bitboards.occupied_tables[game.turn];
    }

    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        targets;
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
        targets;
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

template<bool generate_captures_only>
__attribute__((always_inline)) void add_king_moves(Game& game, Move_list& moves) {

    uint64_t mask = 1ULL;
    uint8_t piece = (game.turn == WHITE) ? WHITE_KING : BLACK_KING;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[(game.turn == WHITE) ? BLACK : WHITE]);
    } else {
        targets = ~game.bitboards.occupied_tables[game.turn];
    }
    int local_counter = moves.num_moves;
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t king_moves = game.bitboards.king_moves[from_square];
        while (king_moves) {
            int to_square = __builtin_ctzll(king_moves);
            if (mask << to_square & targets) {
                //print_bitboard(bitboards.occupied_tables[turn]);
                Move move;
                move.set_move(from_square, to_square, piece, game.board[to_square]);
                if (!is_in_check(game, move)) {
                    moves.list[local_counter++] = move;
                }
            }
            king_moves &= king_moves - 1;
        }
        if constexpr (!generate_captures_only) {
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
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}