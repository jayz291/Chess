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
    void find_position_hash();

    int validate_move(Move& move);
    int validate_pawn_move(Move& move);
    int validate_knight_move(Move& move);
    int validate_bishop_move(Move& move);
    int validate_rook_move(Move& move);
    int validate_queen_move(Move& move);
    int validate_king_move(Move& move);
    int validate_castling(Move& move);
    void evaluate_king_checks();
    int is_square_attacked(int square, int turn);
    bool more_moves_available(Move_list moves);
    uint64_t get_rook_attacks(int square, uint64_t occupancy);
    uint64_t get_bishop_attacks(int square, uint64_t occupancy);

    template<bool update_zobrist> void update_castling_flags(Move& move);
    template<bool update_zobrist> void replace_piece(int turn, uint8_t prev_piece, 
        uint8_t new_piece, int target_square);
    template<bool update_zobrist> void remove_piece(int turn, uint8_t target_piece, 
        int target_square);
    template<bool update_zobrist> void place_piece(int turn, uint8_t target_piece, int target_square);
    template<bool update_zobrist> void restore_zobrist_en_passant_and_castling(Move& prev_move);
    template<bool update_zobrist> void update_zobrist_en_passant(Move& move);
    template<bool update_zobrist> void move_piece(uint8_t target_piece, int from_square, int to_square, int turn);
    template<bool update_zobrist> void undo_move(Move& prev_move);

};

struct Log {
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
        history_scroll_offset = 0;
        move_num = 1;
    }
};

struct UI {
    bool invalid_fen_position;
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

struct Result {
    uint8_t status;
    int winner;
    void initialise() {
        status = 0b00000000;
        winner = -1;
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
    Position position {};
    UI ui {};
    Log log {};
    Result result {};
    Game();
    void initialise();
};

int handle_fen_string(Game& game);
int fill_board(Position& position, std::string& fen_board_section);
int check_position_validity(Game& game);
int process_en_passant_square(Position& position, Log& log, std::string& en_passant_square);
std::string to_algebreic_notation(Game& game);
void disambiguate(Position& position, Log& log, Move& move);

// for debugging
void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
void print_board(std::array<uint8_t, 64> board);
void verify_board_sync(Position& position);
int bit_filled_count(Position& position, std::vector<int>& bitboards_filled, int square);
bool verify_zobrist_sync(Position& position);
void debug_diff(uint64_t diff);