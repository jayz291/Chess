#include "definitions.h"
#include "game.h"
#include "logic.h"

template<int colour, bool generate_captures_only>
__attribute__((always_inline)) void add_pawn_moves(Game& game, Move_list& moves) {
    Bitboards& bitboards = game.bitboards;
    Chessboard& board = game.board;
    
    int local_counter = moves.num_moves;
    constexpr uint8_t target_piece = (colour == WHITE) ? WHITE_PAWN : BLACK_PAWN;
    constexpr int promotion_rank = (colour == WHITE) ? 7 : 0;
    constexpr int opposing_turn = ((colour == WHITE) ? BLACK : WHITE);
    constexpr int left_shift = (colour == WHITE) ? 7 : -9;
    constexpr int right_shift = (colour == WHITE) ? 9 : -7;
    uint64_t wrap_mask_left  = ~FILE_MASKS[7];
    uint64_t wrap_mask_right = ~FILE_MASKS[0];
    uint64_t en_passant_mask = (game.en_passant_square != -1) ? (1ULL << game.en_passant_square) : 0ULL;
    uint64_t enemy_pieces = bitboards.occupied_tables[opposing_turn] | en_passant_mask;
    uint64_t pawns = bitboards.bitboards[target_piece];
    uint64_t left_attacks, right_attacks;
    if constexpr (colour == WHITE) {
        left_attacks = (pawns << 7) & wrap_mask_left & enemy_pieces;
    } else {
        left_attacks = (pawns >> 9) & wrap_mask_left & enemy_pieces;
    }
    while (left_attacks) {
        int to_square = __builtin_ctzll(left_attacks);
        int from_square = (colour == WHITE) ? to_square - 7 : to_square + 9;
        if (to_square == game.en_passant_square) {
            Move move;
            move.set_move(from_square, to_square, target_piece, board[to_square]);
            move.set_move_type(EN_PASSANT);
            moves.list[local_counter++] = move;
        } else if ((to_square >> 3) == promotion_rank) {
            int promotion_choices[4] = { P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN };
            for (int piece: promotion_choices) {
                Move move;
                move.set_move_type(PROMOTION);
                move.set_promotion_piece(piece);
                move.set_move(from_square, to_square, target_piece, board[to_square]);
                moves.list[local_counter++] = move;
            }
        } else {
            Move move;
            move.set_move(from_square, to_square, target_piece, board[to_square]);
            moves.list[local_counter++] = move;
        }
        left_attacks &= left_attacks - 1;
    }
    if constexpr (colour == WHITE) {
        right_attacks = (pawns << 9) & wrap_mask_right & enemy_pieces;
    } else {
        right_attacks = (pawns >> 7) & wrap_mask_right & enemy_pieces;
    }
    while (right_attacks) {
        int to_square = __builtin_ctzll(right_attacks);
        int from_square = (colour == WHITE) ? to_square - 9 : to_square + 7;
        if (to_square == game.en_passant_square) {
            Move move;
            move.set_move(from_square, to_square, target_piece, board[to_square]);
            move.set_move_type(EN_PASSANT);
            moves.list[local_counter++] = move;
        } else if (to_square / 8 == promotion_rank) {
            int promotion_choices[4] = { P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN };
            for (int piece: promotion_choices) {
                Move move;
                move.set_move_type(PROMOTION);
                move.set_promotion_piece(piece);
                move.set_move(from_square, to_square, target_piece, board[to_square]);
                moves.list[local_counter++] = move;
            }
        } else {
            Move move;
            move.set_move(from_square, to_square, target_piece, board[to_square]);
            moves.list[local_counter++] = move;
        }
        right_attacks &= right_attacks - 1;
    }
    if (!generate_captures_only) {
        uint64_t single_pushes, double_pushes;
        if constexpr (colour == WHITE) {
            single_pushes = (bitboards.bitboards[WHITE_PAWN] << 8) & ~game.bitboards.occupied;
            double_pushes = ((single_pushes & RANK_MASKS[2]) << 8) & ~game.bitboards.occupied;
            while (single_pushes) {
                int to_square = __builtin_ctzll(single_pushes);
                int from_square = to_square - 8;
                if (to_square / 8 == promotion_rank) {
                    int promotion_choices[4] = { P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN };
                    for (int piece: promotion_choices) {
                        Move move;
                        move.set_move_type(PROMOTION);
                        move.set_promotion_piece(piece);
                        move.set_move(from_square, to_square, target_piece, EMPTY_SQUARE);
                        moves.list[local_counter++] = move;
                    }
                } else {
                    Move move;
                    move.set_move(from_square, to_square, target_piece, EMPTY_SQUARE);
                    moves.list[local_counter++] = move;
                }
                single_pushes &= single_pushes - 1;
            }
            while (double_pushes) {
                int to_square = __builtin_ctzll(double_pushes);
                int from_square = to_square - 16;
                Move move;
                move.set_move(from_square, to_square, target_piece, EMPTY_SQUARE);
                moves.list[local_counter++] = move;
                double_pushes &= double_pushes - 1;
            }
        } else {
            single_pushes = (bitboards.bitboards[BLACK_PAWN] >> 8) & ~game.bitboards.occupied;
            double_pushes = ((single_pushes & RANK_MASKS[5]) >> 8) & ~game.bitboards.occupied;
            while (single_pushes) {
                int to_square = __builtin_ctzll(single_pushes);
                int from_square = to_square + 8;
                if (to_square / 8 == promotion_rank) {
                    int promotion_choices[4] = { P_KNIGHT, P_BISHOP, P_ROOK, P_QUEEN };
                    for (int piece: promotion_choices) {
                        Move move;
                        move.set_move_type(PROMOTION);
                        move.set_promotion_piece(piece);
                        move.set_move(from_square, to_square, target_piece, EMPTY_SQUARE);
                        moves.list[local_counter++] = move;
                    }
                } else {
                    Move move;
                    move.set_move(from_square, to_square, target_piece, EMPTY_SQUARE);
                    moves.list[local_counter++] = move;
                }
                single_pushes &= single_pushes - 1;
            }
            while (double_pushes) {
                int to_square = __builtin_ctzll(double_pushes);
                int from_square = to_square + 16;
                Move move;
                move.set_move(from_square, to_square, target_piece, EMPTY_SQUARE);
                moves.list[local_counter++] = move;
                double_pushes &= double_pushes - 1;
            } 
        }
    }
    moves.num_moves = local_counter;
}

template<int colour, bool generate_captures_only>
__attribute__((always_inline)) void add_knight_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    constexpr uint8_t piece = (colour == WHITE) ? WHITE_KNIGHT : BLACK_KNIGHT;
    constexpr int opposing_colour = (colour == WHITE) ? BLACK : WHITE;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[opposing_colour]);
    } else {
        targets = ~game.bitboards.occupied_tables[colour];
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
                moves.list[local_counter++] = move;
            }
            knight_moves &= knight_moves - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

template<int colour, bool generate_captures_only>
__attribute__((always_inline)) void add_bishop_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    constexpr uint8_t piece = (colour == WHITE) ? WHITE_BISHOP : BLACK_BISHOP;
    constexpr int opposing_colour = (colour == WHITE) ? BLACK : WHITE;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[opposing_colour]);
    } else {
        targets = ~game.bitboards.occupied_tables[colour];
    }
    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) 
        & targets;
        while (bishop_attacks) {
            int to_square = __builtin_ctzll(bishop_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
            moves.list[moves.num_moves++] = move;
            bishop_attacks &= bishop_attacks - 1;
        }
        current_pieces &= current_pieces - 1;
    }
}

template<int colour, bool generate_captures_only>
__attribute__((always_inline)) void add_rook_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    constexpr uint8_t piece = (colour == WHITE) ? WHITE_ROOK : BLACK_ROOK;
    constexpr int opposing_colour = (colour == WHITE) ? BLACK : WHITE;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[opposing_colour]);
    } else {
        targets = ~game.bitboards.occupied_tables[colour];
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
            moves.list[local_counter++] = move;
            rook_attacks &= rook_attacks - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

template<int colour, bool generate_captures_only>
__attribute__((always_inline)) void add_queen_moves(Game& game, Move_list& moves) {
    uint64_t mask = 1ULL;
    constexpr uint8_t piece = (colour == WHITE) ? WHITE_QUEEN : BLACK_QUEEN;
    constexpr int opposing_colour = (colour == WHITE) ? BLACK : WHITE;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    int local_counter = moves.num_moves;
    int to_square;
    uint64_t targets;
    if constexpr (generate_captures_only) {
        targets = (game.bitboards.occupied_tables[opposing_colour]);
    } else {
        targets = ~game.bitboards.occupied_tables[colour];
    }

    while (current_pieces) {
        int from_square = __builtin_ctzll(current_pieces);
        uint64_t bishop_attacks = get_bishop_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        targets;
        while (bishop_attacks) {
            to_square = __builtin_ctzll(bishop_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
            moves.list[local_counter++] = move;
            bishop_attacks &= bishop_attacks - 1;
        }
        uint64_t rook_attacks = get_rook_attacks(from_square, game.bitboards.occupied, game.bitboards) & 
        targets;
        while (rook_attacks) {
            to_square = __builtin_ctzll(rook_attacks);
            Move move;
            move.set_move(from_square, to_square, piece, game.board[to_square]);
            moves.list[local_counter++] = move;
            rook_attacks &= rook_attacks - 1;
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}

template<int colour, bool generate_captures_only>
__attribute__((always_inline)) void add_king_moves(Game& game, Move_list& moves) {

    uint64_t mask = 1ULL;
    constexpr uint8_t piece = (colour == WHITE) ? WHITE_KING : BLACK_KING;
    uint64_t current_pieces = game.bitboards.bitboards[piece];
    uint64_t targets;
    if constexpr (generate_captures_only) {
        constexpr int opposing_colour = (colour == WHITE) ? BLACK : WHITE;
        targets = (game.bitboards.occupied_tables[opposing_colour]);
    } else {
        targets = ~game.bitboards.occupied_tables[colour];
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
                moves.list[local_counter++] = move;
            }
            king_moves &= king_moves - 1;
        }
        if constexpr (!generate_captures_only) {
            constexpr int original_king_square = (colour == WHITE) ? 4 : 60;
            constexpr int to_kingside_square = (colour == WHITE) ? 6 : 62;
            constexpr int to_queenside_square = (colour == WHITE) ? 2 : 58;
            if (from_square == original_king_square) {
                Move move1, move2;
                move1.set_move(from_square, to_kingside_square, piece, 0);
                move1.set_move_type(CASTLING);
                move2.set_move(from_square, to_queenside_square, piece, 0);
                move2.set_move_type(CASTLING);
                //std::cout << "setting castling move\n";
                if (validate_castling(game, move1) == 1) {
                    moves.list[local_counter++] = move1;
                }
                if (validate_castling(game, move2) == 2) {
                    moves.list[local_counter++] = move2;
                    //std::cout << "here\n";
                }
            } 
        }
        current_pieces &= current_pieces - 1;
    }
    moves.num_moves = local_counter;
}