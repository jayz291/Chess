#pragma once
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <bitset>
#include <algorithm>
#include <assert.h>

constexpr int SQUARE_SIZE = 95;
constexpr int INVALID = -1;
constexpr int VALID = 0;
constexpr int KINGSIDE_CASTLING_MOVE = 1;
constexpr int QUEENSIDE_CASTLING_MOVE = 2;
constexpr int EN_PASSANT_MOVE = 3;

enum {
    WHITE = 0,
    BLACK = 1
};

enum tt_flag {
    tt_exact,
    tt_alpha, 
    tt_beta,
};

using Chessboard = std::array<uint8_t, 64>;

enum : uint8_t {
    EMPTY_SQUARE = 0b0000,
    WHITE_PAWN = 0b0001,
    WHITE_KNIGHT = 0b0010,
    WHITE_BISHOP = 0b0011,
    WHITE_ROOK = 0b0100,
    WHITE_QUEEN = 0b0101,
    WHITE_KING = 0b0110,
    BLACK_PAWN = 0b1001,
    BLACK_KNIGHT = 0b1010,
    BLACK_BISHOP = 0b1011,
    BLACK_ROOK = 0b1100,
    BLACK_QUEEN = 0b1101,
    BLACK_KING = 0b1110,
};

enum : uint8_t {
    TYPE_MASK = 0b0111,
    COLOUR_MASK = 0b1000,
    PAWN = 0b0001,
    KNIGHT = 0b0010,
    BISHOP = 0b0011,
    ROOK = 0b0100,
    QUEEN = 0b0101,
    KING = 0b0110,
};

inline const uint8_t piece_array[2][7] = {
    { EMPTY_SQUARE, WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING },
    { EMPTY_SQUARE, BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING }
};

enum : uint8_t {
    P_KNIGHT = 0b00,
    P_BISHOP = 0b01,
    P_ROOK = 0b10,
    P_QUEEN = 0b11,
};

enum : uint8_t {
    QUIET = 0b00,
    CASTLING = 0b01,
    EN_PASSANT = 0b10,
    PROMOTION = 0b11
};

class Move {
    private:
    uint32_t data {};
    static constexpr int FROM_SHIFT = 26;
    static constexpr int TO_SHIFT = 20;
    static constexpr int PIECE_SHIFT = 16;
    static constexpr int CAPTURED_SHIFT = 12;
    static constexpr int SPECIAL_MOVE_SHIFT = 10;
    static constexpr int PROMOTION_PIECE_SHIFT = 8;
    static constexpr int CASTLING_RIGHTS_SHIFT = 4;
    static constexpr uint32_t SQUARE_MASK = 0x3F;
    static constexpr uint32_t MASK = 0xF;
    static constexpr uint32_t TWO_BIT_MASK = 0x3;
    public:
    int get_from_square() const {
        return (data >> FROM_SHIFT) & SQUARE_MASK;
    }
    int get_to_square() const {
        return (data >> TO_SHIFT) & SQUARE_MASK;
    }
    int get_piece() const {
        return (data >> PIECE_SHIFT) & MASK;
    }
    int get_turn() const {
        return (data >> 19) & 1;
    }
    int get_opposing_turn() const {
        return ~(data >> 19) & 1;
    }
    int get_captured_piece() const {
        return (data >> CAPTURED_SHIFT) & MASK;
    }
    int get_move_type() const {
        return (data >> SPECIAL_MOVE_SHIFT) & TWO_BIT_MASK;
    }
    int get_promotion_piece() const {
        return (data >> PROMOTION_PIECE_SHIFT) & TWO_BIT_MASK;
    }
    int get_castling_rights() const {
        return (data >> CASTLING_RIGHTS_SHIFT) & MASK;
    }
    int get_en_passant_index() const {
        return data & MASK;
    }
    void set_from_square(int from_square) {
        data |= (from_square & SQUARE_MASK) << FROM_SHIFT;
    }
    void set_to_square(int to_square) {
        data |= (to_square & SQUARE_MASK) << TO_SHIFT;
    }
    void set_piece(uint8_t piece) {
        data |= (piece & MASK) << PIECE_SHIFT;
    }
    void set_captured(uint8_t captured) {
        data &= ~(MASK << CAPTURED_SHIFT);
        data |= (captured & MASK) << CAPTURED_SHIFT;
    }
    void set_move_type(uint8_t move_type) {
        data |= (move_type & TWO_BIT_MASK) << SPECIAL_MOVE_SHIFT;
    }
    void set_promotion_piece(uint8_t promotion_piece) {
        data |= (promotion_piece & TWO_BIT_MASK) << PROMOTION_PIECE_SHIFT;
    }
    void set_castling_flags(uint8_t castling_rights) {
        data &= ~(MASK << CASTLING_RIGHTS_SHIFT);
        data |= (castling_rights & MASK) << CASTLING_RIGHTS_SHIFT;
    }
    void set_en_passant_index(uint8_t index) {
        data &= ~(MASK);
        data |= (index & MASK);
    }
    void set_move(int from_square, int to_square, uint8_t piece, uint8_t captured) {
        set_from_square(from_square);
        set_to_square(to_square);
        set_piece(piece);
        set_captured(captured);
    }
};

struct Move_list {
    std::array<Move, 300> list;
    int num_moves {};
};

struct table_entry {
    uint64_t zobrist_key;
    int eval;
    int depth;
    tt_flag flag;
    Move best_move;
};

constexpr int TABLE_SIZE = 1048576;
inline table_entry transposition_table[TABLE_SIZE];

const int piece_values[8] = { 0, 100, 320, 330, 500, 900, 20000, 0 };

const int start_value_tables[6][64] = {
    // pawn 
    { 0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, -20, -20, 10, 10, 5,
    5, -5, -10, 0, 0, -10, -5, 5, 
    -10, 0, 0, 20, 20, 0, 0, -10,
    -10, 5, 10, 25, 25, 10, 5, -10,
    10, 10, 20, 30, 30, 20, 10, 10, 
    50, 50, 50, 50, 50, 50, 50, 50,
    0, 0, 0, 0, 0, 0, 0, 0  },
    // knight
    { -50, -40, -30, -30, -30, -30, -40, -50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50 },
    // bishop
    { -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20 }, 
    // rook
    { 0,  0,  0,  0,  0,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  -5,  0,  0,  0,  0,  -5, -5,
    -5,  -5,  0,  0,  0,  0,  -5, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    5, 10, 10, 10, 10, 10, 10,  5,
    0,  0,  0,  5,  5,  0,  0,  0 },
    // queen
    {  -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20 },
    // king
    { 20, 60, 40,  0,  0, 10, 60, 20,
    20, 20,  0,  0,  0,  0, 20, 20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10 }
};

const int endgame_value_tables[6][64] = {
    // pawn 
    { 0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, -5, -5, -5, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    10, 10, 10, 10, 10, 10, 10, 10,
    20, 20, 10, 10, 10, 10, 20, 20,
    30, 30, 20, 20, 20, 20, 30, 30,
    40, 40, 30, 30, 30, 30, 40, 40,
    50, 50, 40, 40, 40, 40, 50, 50 },
    // knight
    { -50, -40, -30, -30, -30, -30, -40, -50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50 },
    //bishop
    { -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10,  10,  10, 10, 10,  10,  10,-10,
    -10,  10,  10, 10, 10,  10,  10,-10,
    -10,  10, 10, 10, 10, 10,  10,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20 },
    // rook
    {  0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 10, 10, 10, 10, 10, 0,
    0, 20, 20, 30, 30, 20, 20, 0,
    0, 20, 20, 30, 30, 30, 20, 0,
    0, 20, 20, 30, 30, 20, 20, 0,
    0, 20, 20, 30, 30, 20, 20, 0,
    20, 40, 40, 40, 40, 40, 40, 20,
    30, 30, 40, 40, 40, 40, 30, 30  },
    // queen
    {  0, -5, -10, -10, -10, -5, 0,
    0, 0, 10, 10, 10, 10, 0, 0,
    0, 20, 20, 30, 30, 20, 20, 0,
    0, 20, 30, 30, 30, 30, 20, 0,
    0, 30, 40, 40, 40, 40, 30, 0,
    0, 30, 30, 30, 30, 30, 30, 0,
    0, 10, 20, 20, 20, 10, 10, 0,
    10, 10, 10, 10, 10, 10, 10, 10  },
    // king
    { -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50 },
};

extern const int RANK_SCORES[8];

inline const uint64_t FILE_MASKS[8] = {
    0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
    0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL
};
inline const uint64_t ADJACENT_FILE_MASKS[8] = {
    FILE_MASKS[1], FILE_MASKS[0] | FILE_MASKS[2], FILE_MASKS[1] | FILE_MASKS[3], FILE_MASKS[2] | FILE_MASKS[4],
    FILE_MASKS[3] | FILE_MASKS[5], FILE_MASKS[4] | FILE_MASKS[6], FILE_MASKS[5] | FILE_MASKS[7], FILE_MASKS[6]
};
inline const uint64_t RANK_MASKS[8] = {
    0x00000000000000FFULL, 0x000000000000FF00ULL, 0x0000000000FF0000ULL, 0x00000000FF000000ULL,
    0x000000FF00000000ULL, 0x0000FF0000000000ULL, 0x00FF000000000000ULL, 0xFF00000000000000ULL      
};
inline const uint64_t WHITE_PASSED_RANK_MASKS[8] = {
    0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFF000000ULL, 0xFFFFFFFF00000000ULL, 
    0xFFFFFF0000000000ULL, 0xFFFF000000000000ULL, 0xFF00000000000000ULL, 0x0000000000000000ULL 
};
inline const uint64_t BLACK_PASSED_RANK_MASKS[8] = {
    0x0000000000000000ULL, 0x00000000000000FFULL, 0x000000000000FFFFULL, 0x0000000000FFFFFFULL, 
    0x00000000FFFFFFFFULL, 0x000000FFFFFFFFFFULL, 0x0000FFFFFFFFFFFFULL, 0x00FFFFFFFFFFFFFFULL  
};

inline uint64_t FILE_AB = FILE_MASKS[0] | FILE_MASKS[1];
inline uint64_t FILE_GH = FILE_MASKS[6] | FILE_MASKS[7];

struct Bitboards {
    uint64_t bitboards[16]; ///< array of bitboards for every unique type of piece on the board
    uint64_t occupied_tables[2]; ///< bitboards representing which squares are occupied by white or black pieces
    uint64_t occupied; ///< a bitboard representing which squares are occupied
    static uint64_t knight_attacks[64]; ///< bitboards showing possible knight moves from each square
    static uint64_t king_moves[64]; ///< bitboards showing possible king moves from each square
    static uint64_t pawn_attacks[2][64]; ///< bitboards showing possible pawn capture moves for white and black
    static uint64_t pawn_moves[2][64]; ///< bitboards showing possible pawn non-capture moves for white and black
    static uint64_t between_table[64][64]; ///< bitboards holding bit masks between squares of the same row/col/diagonal
    static uint64_t rook_attack_table[64][4096]; ///< bitboards for looking up rook attacks based on the piece arrangement/square
    static uint64_t bishop_attack_table[64][512]; ///< bitboards for looking up bishop attacks based on the piece arrangement/square
    static uint64_t rook_masks[64]; ///< bitboards representing the possible rook moves from each square
    static uint64_t bishop_masks[64]; ///< bitboards representing the possible bishop moves from each square
    static uint64_t rook_magic_nums[64]; ///< magic numbers for hashing purposes, preventing collisions
    static uint64_t bishop_magic_nums[64]; ///< magic numbers for hashing purposes, preventing collisions
    static int rook_shifts[64];
    static int bishop_shifts[64];
    static bool initialised;

    Bitboards();

    /**
     * @brief initialises the array of bitboards to all zeroes
     */
    void init_bitboards();

    /**
     * @brief sets the bitboards representing which squares are occupied, using the 
     * bitboards for all the pieces
     */
    void update_occupied();

    /**
     * @brief precomputes the valid knight moves from every single square
     */
    void find_valid_knight_moves();

    /**
     * @brief precomputes the valid king moves from every single square
     */
    void find_valid_king_moves();

    /**
     * @brief creates bitmasks representing cells on the same diagonal and cells on the same row/column
     */
    void make_between_table();

    /**
     * @brief precomputes the pawn moves/captures from every square on the board
     */
    void find_pawn_attacks();

    /**
     * @brief precalculates/initialises lookup tables for sliding piece attacks using magic bitboards
     * @param square the index of the relevant square
     * @param piece the relevant piece (ROOK or BISHOP)
     */
    constexpr void fill_attack_square(int square, int piece);

    /**
     * @brief fills the attack tables for bishops/rooks for each square 
     */
    constexpr void fill_attack_tables();

    /**
     * @brief rook attack table lookup using magic nums (in definitions.h so it can be inlined)
     * @param square the index of the relevant square
     * @param occupancy the bitboard occupancy table
     * @return a mask representing the possible rook attacks from that square
     */
    uint64_t get_rook_attacks(int square, uint64_t occupancy);

    /**
     * @brief bishop attack table lookup using magic nums (in definitions.h so it can be inlined)
     * @param square the index of the relevant square
     * @param occupancy the bitboard occupancy table
     * @return a mask representing the possible bishop attacks from that square 
     */
    uint64_t get_bishop_attacks(int square, uint64_t occupancy);

    /**
     * @brief precomputes a rook mask (all the squares the rook could move to) for the square, 
     * taking into account of other pieces
     * @param square the index of the relevant square 
     * @param occupied the bitboard occupancy table
     * @return the rook mask
     */
    inline uint64_t find_rook_attacks(int square, uint64_t& occupied);

    /**
     * @brief precomputes a bishop mask (all the squares the bishop could move to) for the square, 
     * taking into account of other pieces
     * @param square the index of the relevant square
     * @param occupied the bitboard occupancy table
     * @return the bishop mask
     */
    inline uint64_t find_bishop_attacks(int square, uint64_t& occupied);

    /**
     * @brief computes a rook mask (all the squares a rook could move to), ignoring other pieces
     * @param square the index of the relevant square
     * @return the rook mask
     */
    uint64_t get_rook_mask(int square);

    /**
     * @brief computes a bishop mask (all the squares a bishop could move to), ignoring other pieces
     * @param square the index of the relevant square
     * @return the bishop mask
     */
    uint64_t get_bishop_mask(int square);

    /**
     * @brief computes the blocker pattern for the sliding pieces
     * @param index number from 0 to 2^num_bits representing all possible permutations of blockers
     * @param num_bits total number of set bits (relevant blockers) in the attack mask
     * @param attack_mask attack mask of the bishop/rook from a particular square 
     * @return a bitboard mask representing the blocker pattern
     */
    uint64_t set_occupancy(int index, int num_bits, uint64_t attack_mask);

    /**
     * @brief function to prevent board wraparounds.
     * A wraparound occurs when a piece is about to go off the board and instead reappears on the other side. 
     * @param square the index of the current square
     * @param direction the direction of the piece from this square (represented as a bitwise shift)
     * @return false if a wraparound is about to occur, and true otherwise
     */
    bool determine_square_validity(int square, int direction);
};

inline uint64_t Bitboards::get_rook_attacks(int square, uint64_t occupancy) {
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_nums[square];
    occupancy >>= (64 - rook_shifts[square]);
    return rook_attack_table[square][occupancy];
}

inline uint64_t Bitboards::get_bishop_attacks(int square, uint64_t occupancy) {
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_nums[square];
    occupancy >>= (64 - bishop_shifts[square]);
    return bishop_attack_table[square][occupancy];
}

enum class Gamestate {
    Intro,
    Playing,
    Promoting_pawn,
    Gameover,
    Resetting
};

enum class Gamemode {
    CPUwhite,
    CPUblack,
    Twoplayer
};

std::ostream& operator<<(std::ostream& os, const Move& move);
bool operator==(Move& move1, Move& move2);






