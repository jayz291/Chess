#pragma once
#include "game.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

int validate_move(Game& game, Chessboard& board, Move& move, bool only_checking_checks = false);
int validate_move_pawn(Game& game, Chessboard& board, Move& move);
int validate_move_knight(Chessboard& board, Move& move);
int validate_move_bishop(Chessboard& board, Move& move);
int validate_move_rook(Chessboard& board, Move& move);
int validate_move_queen(Chessboard& board, Move& move);
int validate_move_king(Game& game, Chessboard& board, Move& move);
int check_checks(Game& game, Chessboard& copy, Move& move);
void evaluate_king_checks(Game& game);
int test_castling(Game& game, Chessboard& copy, Move& move);
int validate_en_passant(Game& game, Chessboard& board, Move& move);

void make_game_move(Game& game, Chessboard& board, int result, Move move);
void make_test_move(Game& game, Chessboard& board, int result, Move& move);
void move_piece(Chessboard& board, int prev_row, int prev_col, int new_row, int new_col, bool undo = false);
void undo_game_move(Game& game);
void undo_test_move(Game& game, Chessboard& board, Move& prev_move);
void undo_move(Game& game, Chessboard& board, Move& prev_move, std::string turn);
void handle_pawn_promotion(Game& game, Chessboard& board, Move& move);

std::vector<Move> determine_possible_moves(Game& game, Chessboard& board, std::string turn, bool CPU = false);
int determine_repetition(Game& game);
int determine_insufficient_material(Game& game);
void end_game(Game& game);
void record_board(Game& game);
void is_game_over(Game& game);
void record_piece_points(Game& game, std::string piece_type, std::string piece_colour);

void generate_computer_move(Game& game);
void update_computer_move(Game& game);
Move get_best_move(Game& game, Chessboard& copy, int depth);
int minimax(Game& game, Chessboard& board, int depth, int alpha, int beta, bool maximising);
int evaluate(Chessboard& board);

