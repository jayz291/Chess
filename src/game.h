#pragma once
#include "definitions.h"
#include <atomic>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <cstdlib> 
#include <ctime>
#include <cstring>

extern Move calculated_move;
extern std::atomic<bool> computer_turn;
extern std::atomic<bool> thinking_in_progress;
extern std::atomic<bool> finished;
extern int positions_searched;

struct Position {
    Chessboard board;
    Bitboards bitboards;
    bool white_in_check, black_in_check;
    int plys_to_100;
    int turn;
    int en_passant_index; // (0 - 7 for col of en passant square, 8 if there is none )
    int en_passant_square;
    std::vector<int> plys_to_100_tracking {};
    std::vector<Move> move_record {};
    std::vector<uint64_t> board_record {};
    uint8_t castling_rights;
    int value_white_pieces;
    int value_black_pieces;
    uint64_t zobrist_hash;
    Move current_move;
    void initialise() {
        plys_to_100_tracking.clear();
        move_record.clear();
        board_record.clear();
        white_in_check = black_in_check = false;
        board = {};
        bitboards = {};
        plys_to_100 = 0;
        en_passant_square = -1;
        value_white_pieces = value_black_pieces = 0;
        castling_rights = 0b00000000;
        zobrist_hash = 0ULL;
        current_move = {};
    }
};

struct History_log {
    bool rank_ambiguous, file_ambiguous, conflict;
    bool first_move_filler;
    int history_scroll_offset;
    int current_ply_num;
    int move_num;
    std::vector<std::string> notation_history {};
    void initialise() {
        rank_ambiguous = file_ambiguous = conflict = false;
        first_move_filler = false;
        notation_history.clear();
        current_ply_num = 0;
        move_num = 1;
    }
};

struct UI {
    bool invalid_fen_position { false };
    int view { WHITE };
    bool promoting_pawn;
    int selected_square;
    int piece_selected;
    bool is_dragging;
    uint8_t dragged_piece;
    void initialise() {
        invalid_fen_position = false;
        promoting_pawn = false;
        selected_square = -1;
        piece_selected = -1;
        is_dragging = false;
        dragged_piece = EMPTY_SQUARE;
    }
};

class Game {
    public:
    Gamemode mode { Gamemode::Twoplayer };
    Gamestate state {};
    Move calculated_move;
    bool move_ready;
    sf::String entered_fen;
    std::string final_fen;
    uint8_t game_status;
    int winner;
    Position position {};
    UI ui {};
    History_log history_log {};
    Game();
    void initialise();
};

int handle_fen_string(Game& game);
int fill_board(Position& position, std::string& fen_board_section);
int check_position_validity(Game& game);
int process_en_passant_square(Position& position, History_log& history_log, std::string& en_passant_square);
std::string to_algebreic_notation(Game& game);
void disambiguate(Position& position, History_log& history_log, Move& move);

// for debugging
void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
void print_board(std::array<uint8_t, 64> board);
void verify_board_sync(Game& game);
int bit_filled_count(Game& game, std::vector<int>& bitboards_filled, int square);
bool verify_zobrist_sync(Game& game);
void debug_diff(uint64_t diff);