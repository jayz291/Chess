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

class Game {
    public:
    //std::vector<Piece> pieces{32}; 
    Gamestate state {};
    Gamemode mode { Gamemode::Twoplayer };
    Chessboard board {};
    Bitboards bitboards {};
    int selected_square;
    int turn;
    bool white_in_check;
    bool black_in_check;
    uint8_t game_status; // 8th bit: is game over, 4th bit: checkmate, 3rd bit: stalemate, 2nd bit: repetition
    int winner;
    bool promoting_pawn;
    int piece_selected;
    std::vector<Move> move_record {};
    std::vector<uint64_t> board_record {};
    std::vector<std::string> notation_history {};
    std::vector<int> plys_to_100_tracking {};
    int move_num;
    int history_scroll_offset;
    int plys_to_100;
    int value_white_pieces;
    int value_black_pieces;
    Move current_move;
    int view { WHITE };
    Move calculated_move;
    bool move_ready;
    uint8_t castling_rights;
    bool is_dragging { false };
    uint8_t dragged_piece;
    sf::String entered_fen;
    std::string final_fen;
    int en_passant_index; // (0 - 7 for col of en passant square, 8 if there is none )
    int en_passant_square;
    int current_ply_num;
    bool invalid_fen_position { false };
    bool rank_ambiguous, file_ambiguous, conflict;
    bool first_move_filler;
    uint64_t zobrist_hash;
    Game();
    void initialise();
};

int handle_fen_string(Game& game);
int fill_board(Game& game, std::string& fen_board_section);
int check_position_validity(Game& game);
int process_en_passant_square(Game& game, std::string& en_passant_square);
std::string to_algebreic_notation(Game& game);
void disambiguate(Game& game, Move& move);

// for debugging
void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
void print_board(std::array<uint8_t, 64> board);
void verify_board_sync(Game& game);
int bit_filled_count(Game& game, std::vector<int>& bitboards_filled, int square);
bool verify_zobrist_sync(Game& game);
void debug_diff(uint64_t diff);