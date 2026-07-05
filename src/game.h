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

    /**
     * @brief checks whether a move is legal
     * @param move the move to evaluate
     * @return 
     */
    int validate_move(Move& move);

    /**
     * @brief checks the legality of a pawn move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 0 (VALID NON EN PASSANT MOVE), 3 (VALID EN PASSANT MOVE)
     */
    int validate_pawn_move(Move& move);

    /**
     * @brief checks the legality of a knight move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 0 (VALID move)
     */
    int validate_knight_move(Move& move);

    /**
     * @brief checks the legality of a bishop move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 0 (VALID move)
     */
    int validate_bishop_move(Move& move);

    /**
     * @brief checks the legality of a bishop move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 0 (VALID move)
     */
    int validate_rook_move(Move& move);

    /**
     * @brief checks the legality of a queen move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 0 (VALID move)
     */
    int validate_queen_move(Move& move);

    /**
     * @brief checks the legality of a king move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 0 (VALID non-castling move), 
     * 2 (VALID queenside castling move), 3 (VALID kingside castling move)
     */
    int validate_king_move(Move& move);

    /**
     * @brief helper function to check the legality of a castling move
     * @param move the move to evaluate
     * @return -1 (INVALID move), 2 (VALID queenside castling move), 3 (VALID kingside castling move)
     */
    int validate_castling(Move& move);

    /**
     * @brief evaluates whether the king on each side is in check and modifies the white_in_check/
     * black_in_check attributes
     */
    void evaluate_king_checks();

    /**
     * @brief checks whether a square is attacked by any piece
     * @param square number from 0-63 representing the index of the square on the bitboard
     * @param turn enum value for the current turn
     * @return 1 if the square is attacked, and 0 otherwise
     */
    int is_square_attacked(int square, int turn);

    /**
     * @brief determines whether there is a legal move in the list of pseudolegal moves
     * @param moves 
     * @return true if a valid move is found, and false otherwise
     */
    bool more_moves_available(Move_list moves);

    /**
     * @brief update castling rights in the Game class/zobrist hash. Store the castling rights in the Move
     * struct so that it can be restored later (when an undo occurs)
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param move the move done
     */
    template<bool update_zobrist> void update_castling_flags(Move& move);

    /** 
     * @brief replace piece with another piece on all 3 representations of the board
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param turn colour of the pieces
     * @param prev_piece piece to be replaced
     * @param new_piece piece to replace it with
     * @param target_square number from 0-63 representing the index of the square on the bitboard
     */
    template<bool update_zobrist> void replace_piece(int turn, uint8_t prev_piece, uint8_t new_piece, int target_square);

    /**
     * @brief remove piece from all representations of the board
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param turn colour of the piece
     * @param target_piece piece to be removed
     * @param target_square number from 0-63 representing the index of the square on the bitboard
     */
    template<bool update_zobrist> void remove_piece(int turn, uint8_t target_piece, int target_square);

    /**
     * @brief place piece on all representations of the board
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param turn colour of the piece
     * @param target_piece piece to be removed
     * @param target_square number from 0-63 representing the index of the square on the bitboard
     */
    template<bool update_zobrist> void place_piece(int turn, uint8_t target_piece, int target_square);

    /**
     * @brief restores the previous en passant square and castling rights (and the zobrist hash, if applicable)
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param prev_move the previous move, containing info pertaining to castling/en passant rights
     */
    template<bool update_zobrist> void restore_zobrist_en_passant_and_castling(Move& prev_move);

    /**
     * @brief sets the column and the index of the en passant square (and the zobrist hash), if applicable
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param move the move done
     */
    template<bool update_zobrist> void update_zobrist_en_passant(Move& move);

    /**
     * @brief moves a piece from one square to another. 
     * Equivalent to removing the piece from its original square,
     * placing the piece at its new square, and also removing any captured pieces at the new square.
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param target_piece the piece being moved
     * @param from_square index of the previous square
     * @param to_square index of the new square
     * @param turn colour of the piece
     */
    template<bool update_zobrist> void move_piece(uint8_t target_piece, int from_square, int to_square, int turn);

    /**
     * @brief undoes a move
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param prev_move move to be undone
     */
    template<bool update_zobrist> void undo_move(Move& prev_move);

    /**
     * @brief undoes a trial move 
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param prev_move move to be undone
     */
    template<bool update_zobrist> void undo_test_move(Move& prev_move);

    /**
     * @brief does a trial move
     * @tparam update_zobrist true if the zobrist hash is to be updated, and false otherwise
     * @param move move to be done
     */
    template<bool update_zobrist> bool make_test_move(Move& move);

    /**
     * @brief switches the turn to give a free move to a colour (for evaluation purposes)
     * @param stored_ep_square reference to store the current en passant square of the position
     * @param stored_hash reference to store the current zobrist hash of the position
     */
    void make_null_move(int& stored_ep_square, uint64_t& stored_hash);

    /**
     * @brief undoes the turn switch
     * @param stored_ep_square en passant square of the previous position
     * @param stored_hash zobrist hash of the previous position 
     */
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
    void create_pgn();
};

// for debugging
void print_bitboard(uint64_t bitboard);
void print_all_bitboards(Bitboards& bitboards);
void print_board(std::array<uint8_t, 64> board);
void verify_board_sync(Position& position);
int bit_filled_count(Position& position, std::vector<int>& bitboards_filled, int square);
bool verify_zobrist_sync(Position& position);
void debug_diff(uint64_t diff);