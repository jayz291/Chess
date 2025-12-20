#pragma once
#include "game.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

void find_valid_white_pawn_moves(Game& game);
void find_valid_black_pawn_moves(Game& game);
uint64_t find_rook_attacks(int square, Bitboards& bitboards);
uint64_t find_bishop_attacks(int square, Bitboards& bitboards);
bool determine_square_validity(int square, int direction);

int create_encoded_move(int from, int to, int flags);
int get_flags(Encoded_move move);
int get_from(Encoded_move move);
int get_to(Encoded_move move);

int validate_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& move, bool only_checking_checks = false);
int validate_move_pawn(Game& game, Bitboards& bitboards, Chessboard& board, Move& move);
int validate_move_knight(Bitboards& bitboards, Chessboard& board, Move& move);
int validate_move_bishop(Bitboards& bitboards, Chessboard& board, Move& move);
int validate_move_rook(Bitboards& bitboards, Chessboard& board, Move& move);
int validate_move_queen(Bitboards& bitboards, Chessboard& board, Move& move);
int validate_move_king(Game& game, Bitboards& bitboards, Chessboard& board, Move& move);
int check_checks(Game& game, Bitboards& bitboard_copy, Chessboard& copy, Move& move);
void evaluate_king_checks(Game& game);
int test_castling(Game& game, Bitboards& bitboard_copy, Chessboard& copy, Move& move);
int validate_en_passant(Game& game, Chessboard& board, Move& move);
int is_square_attacked(Bitboards& bitboard_copy, int square, int turn);

void make_game_move(Game& game, Bitboards& bitboards, Chessboard& board, int result, Move move);
void make_test_move(Game& game, Bitboards& bitboards, Chessboard& board, int result, Move& move);
void move_piece(Game& game, Bitboards& bitboards, Chessboard& board, Move& move, bool undo = false);
void undo_game_move(Game& game);
void undo_test_move(Game& game, Chessboard& board, Move& prev_move);
void undo_move(Game& game, Bitboards& bitboards, Chessboard& board, Move& prev_move, int turn);
void handle_pawn_promotion(Game& game, Bitboards& bitboards, Chessboard& board, Move& move);
void switch_move(Move& move);

std::vector<Move> determine_possible_moves(Game& game, Chessboard& board, int turn, bool CPU = false);
int determine_repetition(Game& game);
int determine_insufficient_material(Game& game);
void end_game(Game& game);
void record_board(Game& game);
void is_game_over(Game& game);
void record_piece_points(Game& game, int piece_type, int piece_colour);

void generate_computer_move(Game& game);
void update_computer_move(Game& game);
Move get_best_move(Game& game, Chessboard& copy, int depth);
int minimax(Game& game, Chessboard& board, int depth, int alpha, int beta, bool maximising);
int evaluate(Chessboard& board);

