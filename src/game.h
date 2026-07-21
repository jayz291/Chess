#pragma once
#include "definitions.h"
#include <atomic>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <cstdlib> 
#include <ctime>
#include <cstring>

extern Move calculated_move; ///< move chosen by the computer 
extern std::atomic<bool> computer_turn;
extern std::atomic<bool> thinking_in_progress;
extern std::atomic<bool> finished;
extern int positions_searched;

struct Position {
    Chessboard board;
    Bitboards bitboards;
    bool white_in_check, black_in_check;
    int plys_to_100; ///< 50 move rule tracking 
    int turn;
    int en_passant_index; ///< 0 - 7 for col of en passant square, 8 if there is none
    int en_passant_square; ///< index of the square where an en passant capture pawn will land on
    std::vector<int> plys_to_100_tracking {};
    std::vector<Move> move_record {};
    std::vector<uint64_t> board_record {};
    uint8_t castling_rights; ///< record of the castling rights for each player
    int value_white_pieces; 
    int value_black_pieces; 
    uint64_t zobrist_hash; ///< an unsigned 64-bit number representing a position
    Move current_move;
    std::deque<Move> premoves {};

    /**
     * @brief initialises all the necessary variables to start the game 
     */
    void initialise();

    /**
     * @brief generates a zobrist hash value from scratch by XORing all relevant values
     */
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
     * @return false if the move is illegal (king captured, or leaves king in check), and true otherwise
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
    sf::Vector2f panel_pos;
    sf::Vector2f panel_size;
    int line_height;
    int max_lines_visible;
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
    Timesetting time_control { Timesetting::Untimed };
    float white_time = 600;
    float black_time = 600;
    std::chrono::time_point<std::chrono::steady_clock> prev_time;
    float time_increment;
    Move calculated_move;
    bool move_ready;
    bool premove;
    sf::String entered_fen;
    std::string final_fen;
    Position position {};
    UI ui {};
    Log log {};
    Result result {};
    Game();

    /**
     * @brief initialises all the necessary components for a new game
     */
    void initialise();

    /**
     * @brief reads the fen string that has been inputted into the textbox, or the fen string for
     * the default position if none is inputted
     * @return -1 (INVALID) if the fen string produced an illegal position, or 0 (VALID) otherwise 
     */
    int handle_fen_string();

    /**
     * @brief fills the chessboard based on the fen string.
     * Updates both the array representation (of length 64) and also the bitboards. 
     * @param fen_board_section the section of the fen string pertaining to the board setup
     * @return -1 (INVALID) if the position is not possible, and 0 (VALID) otherwise
     */
    int fill_board(std::string& fen_board_section);

    /**
     * @brief checks whether a position is legal. 
     * Specifically, it ensures that:
     * there is exactly one king for each side on the board,
     * no pawns on its colour's promotion rank,
     * the king cannot be captured on the next turn 
     * @return -1 (INVALID) if the position is illegal, and 0 (VALID) otherwise 
     */
    int check_position_validity();

    /**
     * @brief automatically sets a previous move, which leads to a valid en passant square
     * @param en_passant_square the index of the en passant square on the board (0-63)
     * @return -1 (INVALID) if the en passant square is impossible, and 0 (VALID) otherwise
     */
    int process_en_passant_square(std::string& en_passant_square);

    /**
     * @brief checks whether two pieces of the same type can move the same square, necessary to
     * provide accurate algebraic chess notation
     * @param move the move to be evaluated
     */
    void disambiguate(Move& move);

    /**
     * @brief undoes a game move. 
     * If the game is over, this function will not update the game log and the 50 move rule count
     * (this function serves to allow the user to look back over the game)
     * @param is_game_over boolean flag, true if the game is over and false otherwise 
     */
    void undo_game_move(bool is_game_over = false);

    /**
     * @brief does a game move. 
     * If the game is over, this function will not update the game log and 50 move rule count
     * (this function serves to allow the user to look back over the game)
     * @param result represents the integer returned from the validate_move function, allowing the
     * function to do special moves
     * @param move the move done
     * @param is_game_over boolean flag, true if the game is over and false otherwise 
     */
    void make_game_move(int result, Move move, bool is_game_over = false);

    /**
     * @brief does pawn promotion 
     * @param piece_already_selected if a piece is already selected, record the promotion piece in the move data
     * @param is_game_over boolean flag, true if the game is over and false otherwise 
     */
    void handle_pawn_promotion(Move& move, bool piece_already_selected = false, bool is_game_over = false);

    /**
     * @brief calls a function to end the game if any of the game over conditions are met
     */
    void is_game_over();

    /**
     * @brief ends the game and updates the game status encoding accordingly
     * @param on_time boolean flag which is true if the game ended because a player ran out of time,
     * and false otherwise
     */
    void end_game(bool on_time = false);

    /**
     * @brief determines if there are enough pieces on the board for a checkmate
     */
    bool determine_insufficient_material();

    /**
     * @brief checks whether there is a draw by threefold repetition
     */
    bool determine_repetition();

    /**
     * @brief translates the move into algebraic chess notation e.g. Qf7+
     */
    std::string to_algebreic_notation();

    /**
     * @brief creates a pgn file of the game
     */
    void create_pgn();

    /**
     * @brief updates the game clock
     */
    void update_time();

    /**
     * @brief adds the time incremenet after each player has made their move
     */
    void add_time_increment();

    /**
     * @brief checks the first move of the queue of premoves (if any) and determines whether it is 
     * legal or not
     */
    void assess_and_make_premove_moves();
};

// DEBUGGING FUNCTIONS

/**
 * @brief prints out the bitboard into an 8x8 array
 * @param bitboard the bitboard to be printed out
 */
void print_bitboard(uint64_t bitboard);

/**
 * @brief prints out all the bitboards for every type of pieces (6 types of pieces x 2 colours)
 * @param bitboards class containing all bitboards
 */
void print_all_bitboards(Bitboards& bitboards);

/**
 * @brief prints out the array representation of the board into an 8x8 array
 * @param board the board to the printed
 */
void print_board(std::array<uint8_t, 64> board);

/**
 * @brief determines whether the array representation and the bitboards of a position are in sync
 * @param position position to be analysed
 */
void verify_board_sync(Position& position);

/**
 * @brief checks how many times a specific index on the bitboard is filled across all bitboards
 * @param position position to be analysed
 * @param bitboards_filled list of bitboards where the bitboard was filled at that index
 * @param square index of the board square evaluated
 * @return the number of bitboards where that bit is filled
 */
int bit_filled_count(Position& position, std::vector<int>& bitboards_filled, int square);

/**
 * @brief determines whether the zobrist hash is correct.
 * Compares the incremental hash to one generated from scratch.
 * @param position position to be analysed
 * @return true if they match and false otherwise
 */
bool verify_zobrist_sync(Position& position);

/**
 * @brief determines the possible reasons for the zobrist hash discrepancy
 * @param diff the XOR between the two hashes
 */
void debug_diff(uint64_t diff);