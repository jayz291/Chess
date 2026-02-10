#pragma once
#include "game.h"
#include "gui.h"

extern std::atomic<bool> terminate_search;
constexpr int CHECKMATE_THRESHOLD = 300000;
constexpr int NO_ENTRY_FOUND = -999999;

void make_computer_move(Game& game, Assets& assets);
void generate_computer_move(Position& position);
void clear_transposition_table();

struct Engine {
    Position& position;
    int search_allocated_time_ms;
    long long nodes_searched { 0 };
    std::chrono::steady_clock::time_point start_time;
    Engine(Position& position, int search_allocated_time_ms) : position(position), 
    search_allocated_time_ms(search_allocated_time_ms) {
        nodes_searched = 0;
        terminate_search = false;
        start_time = std::chrono::steady_clock::now();
    }

    void record_entry(uint64_t key, int eval, int depth, tt_flag flag, Move best_move, int ply);
    int probe_transposition_table(uint64_t key, int depth, int alpha, int beta, Move& best_move, int ply);

    Move get_best_move(int search_depth = 40);
    int negamax(int depth, int alpha, int beta, int ply, int& seldepth);
    inline int find_eval(int move_num, int depth, int beta, int alpha, int ply, int& seldepth);
    int evaluate();
    int sort_moves_by_priority(Move& move);
    inline int positional_eval(uint64_t bitboard, uint8_t piece, bool black = false);
    int quiescence_search(int alpha, int beta, int ply, int& seldepth);
    int pawn_structure_eval();
    bool determine_repetition();
};



