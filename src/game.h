#pragma once
#include "definitions.h"
#include <atomic>

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
        Coords selected {};
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
        std::vector<std::string> board_record {};
        int plys_to_100 {};
        int value_white_pieces {};
        int value_black_pieces {};
        bool pawns_on_board {};
        Move current_move {};
        int view { white };
        std::vector<Move> possible_moves {};
        Move calculated_move {};
        bool move_ready { false };
        uint8_t castling_rights;
        Game();
        void initialise();
};

void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
