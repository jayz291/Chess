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
    void initialise();
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

    template<bool update_zobrist> void update_castling_flags(Move& move);
    template<bool update_zobrist> void replace_piece(int turn, uint8_t prev_piece, uint8_t new_piece, int target_square);
    template<bool update_zobrist> void remove_piece(int turn, uint8_t target_piece, int target_square);
    template<bool update_zobrist> void place_piece(int turn, uint8_t target_piece, int target_square);
    template<bool update_zobrist> void restore_zobrist_en_passant_and_castling(Move& prev_move);
    template<bool update_zobrist> void update_zobrist_en_passant(Move& move);
    template<bool update_zobrist> void move_piece(uint8_t target_piece, int from_square, int to_square, int turn);
    template<bool update_zobrist> void undo_move(Move& prev_move);
    template<bool update_zobrist> void undo_test_move(Move& prev_move);
    template<bool update_zobrist> bool make_test_move(Move& move);
    void make_null_move(int& stored_ep_square, uint64_t& stored_hash);
    void undo_null_move(int stored_ep_square, uint64_t stored_hash);
};

struct Log {
    bool rank_ambiguous, file_ambiguous, conflict;
    bool first_move_filler;
    int history_scroll_offset;
    int current_ply_num;
    int move_num;
    std::vector<std::string> notation_history {};
    void initialise();
};

struct UI {
    bool invalid_fen_position;
    int view { WHITE };
    bool promoting_pawn;
    int selected_square;
    int piece_selected;
    bool is_dragging;
    uint8_t dragged_piece;
    void initialise();
};

struct Result {
    uint8_t status;
    int winner;
    void initialise();
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
    int handle_fen_string();
    int fill_board(std::string& fen_board_section);
    int check_position_validity();
    int process_en_passant_square(std::string& en_passant_square);
    void disambiguate(Move& move);

    void undo_game_move(bool is_game_over = false);
    void make_game_move(int result, Move move, bool is_game_over = false);
    void handle_pawn_promotion(Move& move, bool piece_already_selected = false, bool is_game_over = false);

    void is_game_over();
    void end_game();
    bool determine_insufficient_material();
    bool determine_repetition();

    std::string to_algebreic_notation();
};

// for debugging
void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
void print_board(std::array<uint8_t, 64> board);
void verify_board_sync(Position& position);
int bit_filled_count(Position& position, std::vector<int>& bitboards_filled, int square);
bool verify_zobrist_sync(Position& position);
void debug_diff(uint64_t diff);