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
        int selected_square {};
        int turn {};
        bool white_in_check {};
        bool black_in_check {};
        bool game_over {};
        bool checkmate {};
        bool stalemate {};
        bool repetition {};
        bool insufficient_material {};
        int winner { -1 };
        bool promoting_pawn {};
        int piece_selected {};
        std::vector<Move> move_record {};
        std::vector<uint64_t> board_record {};
        int plys_to_100 {};
        int value_white_pieces {};
        int value_black_pieces {};
        Move current_move {};
        int view { white };
        std::vector<Move> possible_moves {};
        Move calculated_move {};
        bool move_ready { false };
        uint8_t castling_rights;
        bool is_dragging { false };
        int dragged_piece = none;
        sf::Vector2f current_mouse_pos;
        sf::String fen_string;
        int en_passant_index { 8 }; // (0 - 7 for col of en passant square, 8 if there is none )
        bool typing { false };
        std::size_t cursor_index;
        bool default_position { true };
        bool invalid_fen_position { false };
        uint64_t zobrist_hash = 0ULL;
        Game();
        void initialise();
};

int handle_fen_string(Game& game);
int fill_board(Game& proposed_game, std::string& fen_board_section);
int check_position_validity(Game& proposed_game);
int process_en_passant_square(Game& proposed_game, std::string& en_passant_square);
void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
