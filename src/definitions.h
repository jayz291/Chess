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

uint64_t find_rook_attacks(int square, uint64_t& bitboards);
uint64_t find_bishop_attacks(int square, uint64_t& bitboards);
bool determine_square_validity(int square, int direction);
uint64_t get_rook_mask(int square);
uint64_t get_bishop_mask(int square);
uint64_t set_occupancy(int index, int num_bits, uint64_t attack_mask);

constexpr int SQUARE_SIZE = 95;

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

struct Move {
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
        //data &= ~(SQUARE_MASK << FROM_SHIFT);
        data |= (from_square & SQUARE_MASK) << FROM_SHIFT;
    }
    void set_to_square(int to_square) {
        //data &= ~(SQUARE_MASK << TO_SHIFT);
        data |= (to_square & SQUARE_MASK) << TO_SHIFT;
    }
    void set_piece(uint8_t piece) {
        //data &= ~(MASK << PIECE_SHIFT);
        data |= (piece & MASK) << PIECE_SHIFT;
    }
    void set_another_piece(uint8_t piece) {
        data &= ~(MASK << PIECE_SHIFT);
        data |= (piece & MASK) << PIECE_SHIFT;
    }
    void set_captured(uint8_t captured) {
        data &= ~(MASK << CAPTURED_SHIFT);
        data |= (captured & MASK) << CAPTURED_SHIFT;
    }
    void set_move_type(uint8_t move_type) {
        //data &= ~(TWO_BIT_MASK << SPECIAL_MOVE_SHIFT);
        data |= (move_type & TWO_BIT_MASK) << SPECIAL_MOVE_SHIFT;
    }
    void set_promotion_piece(uint8_t promotion_piece) {
        //data &= ~(TWO_BIT_MASK << PROMOTION_PIECE_SHIFT);
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

struct Move_record {
    std::array<Move, 2048> history;
    int moves_recorded {};
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

const int piece_values[16] = { 0, 100, 300, 300, 500, 900, 20000, 0, 0, 100, 300, 300, 500, 900, 20000, 0 };

const int start_value_tables[6][64] = {
    // pawn 
    { 0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, -20, -20, 10, 10, 5,
    5, -5, -10, 0, 0, -10, -5, 5, 
    0, 0, 0, 20, 20, 0, 0, 0,
    5, 5, 10, 25, 25, 10, 5, 5,
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

extern uint64_t FILE_H;
extern uint64_t FILE_A;
extern uint64_t FILE_B;
extern uint64_t FILE_G;
extern uint64_t FILE_AB;
extern uint64_t FILE_GH;
extern uint64_t RANK_4;
extern uint64_t RANK_5;
extern uint64_t RANK_2;
extern uint64_t RANK_7;

struct Bitboards {
    uint64_t bitboards[16];
    uint64_t occupied_tables[2];
    uint64_t occupied;
    static uint64_t knight_attacks[64];
    static uint64_t king_moves[64];
    static uint64_t pawn_attacks[2][64];
    static uint64_t pawn_moves[2][64];
    static uint64_t between_table[64][64];
    static uint64_t rook_attack_table[64][4096];
    static uint64_t bishop_attack_table[64][512];
    static uint64_t rook_masks[64];
    static uint64_t bishop_masks[64];
    inline static uint64_t rook_magic_nums[64] = {
        0x80102040008000, 0x40004020001000, 0x8801000800c2001, 0x1200082004401200, 
        0x3200200200040910, 0x200011450120028, 0x880008002000100, 0x4200008100402402, 
        0x20800080400020, 0x81003100400080, 0x2000801000200080, 0x6800801000800806, 
        0x800800040082, 0x1c1000400030008, 0x860808001000200, 0x102001108804224, 
        0x410020800104, 0x1860004040100020, 0x4021010010402000, 0x811010024100008, 
        0x40850008001100, 0x3811010008040002, 0x4040008011002, 0x20000804104, 
        0x8d40400080003080, 0x200810100204004, 0x200300180200081, 0x4000100080800800, 
        0xc000080080800400, 0x4004020080800400, 0x4001020080800100, 0x800480068000d100, 
        0x100400221800090, 0x4110004000402000, 0x6400200182801000, 0x8010010100200, 
        0x4000800800480, 0x110800400800201, 0x40080204001001, 0x74041042000881, 
        0x1180022000414000, 0x2010002003444004, 0x5000804200120020, 0x10500100021000c, 
        0x1043000800050010, 0x1000020004008080, 0x8080106228040001, 0x800884102000c, 
        0xc00420804a010200, 0x804000200480, 0xa101002000401100, 0x400081001002300, 
        0x20208008000c0180, 0xe12c004100020040, 0x12250880400, 0x1181048041240e00, 
        0x800102042008102, 0x2a04344001810621, 0x20601802010e842, 0x4050008201001, 
        0x2c02010844502002, 0x20a001124383002, 0x240221008008144, 0x100502240b8842
    };
    inline static uint64_t bishop_magic_nums[64] = {
        0xb0a0021000488088, 0x4940410420000, 0x81080224042040a8, 0x8080a0020000044, 
        0x101104100042108, 0x4444108c0101600, 0x40840420060114, 0x3c402801105080, 
        0x1000242008012b06, 0x10040800a020, 0x4200108102002000, 0x4001109092000800, 
        0x421104001c400, 0x2000020190080008, 0x20210040480, 0x8006420100a80444, 
        0x84091010100120, 0xa4049204444400, 0x1008208010100, 0x404080a122004, 
        0x24000088a00040, 0x2802044101008200, 0x8414d1c0b088800, 0x6082818704012100, 
        0x8204041440100, 0x8024101102108129, 0x680808040bc010, 0x1202008008008002, 
        0x1001001004000, 0x82220102880100, 0xa000841081040200, 0x40411840840103, 
        0x4001202001114488, 0x20180218800a0880, 0x8082003202040804, 0x20280380080, 
        0x40404000cd010, 0x10100440002400, 0x2480042010401, 0x400800435c028200, 
        0xa22010006108, 0x6008220002800, 0x130c0402000400, 0x640008a011068804, 
        0x1400810418200, 0x1224180a007020, 0x8008410102006401, 0x88808400400082, 
        0x880402201002, 0xc09088201206010, 0x85202404120082, 0x249100020880947, 
        0x240014208221000, 0x2008401002208a00, 0x4028821808011080, 0x40b004029410c000, 
        0xa08e030100908400, 0x1000452882482002, 0x400242042064100, 0x100a0420200, 
        0x2002020412020210, 0x402a44bd02, 0x10400860010a0221, 0x4091012028301
    };
    inline static int rook_shifts[64] = {
        12, 11, 11, 11, 11, 11, 11, 12,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        12, 11, 11, 11, 11, 11, 11, 12
    };
    inline static int bishop_shifts[64] = {
        6, 5, 5, 5, 5, 5, 5, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 5, 5, 5, 5, 5, 5, 6
    };
    static bool initialised;
    Bitboards() {
        bitboards[WHITE_PAWN] = 0x000000000000FF00ULL;
        bitboards[WHITE_KNIGHT] = 0x0000000000000042ULL;
        bitboards[WHITE_BISHOP] = 0x0000000000000024ULL;
        bitboards[WHITE_ROOK] = 0x0000000000000081ULL;
        bitboards[WHITE_QUEEN] = 0x0000000000000008ULL;
        bitboards[WHITE_KING] = 0x0000000000000010ULL;
        bitboards[BLACK_PAWN] = 0x00FF000000000000ULL;
        bitboards[BLACK_KNIGHT] = 0x4200000000000000ULL;
        bitboards[BLACK_BISHOP] = 0x2400000000000000ULL;
        bitboards[BLACK_ROOK] = 0x8100000000000000ULL;
        bitboards[BLACK_QUEEN] = 0x0800000000000000ULL;
        bitboards[BLACK_KING] = 0x1000000000000000ULL;
        update_occupied();
        if (!initialised) {
            find_valid_knight_moves();
            find_valid_king_moves();
            make_between_table();
            find_pawn_attacks();
            fill_attack_tables();
            initialised = true;
        }
    }
    void update_occupied() {
        occupied_tables[WHITE] = bitboards[WHITE_PAWN] | bitboards[WHITE_KNIGHT] | bitboards[WHITE_BISHOP] | 
        bitboards[WHITE_ROOK] | bitboards[WHITE_QUEEN] | bitboards[WHITE_KING];
        occupied_tables[BLACK] = bitboards[BLACK_PAWN] | bitboards[BLACK_KNIGHT] | bitboards[BLACK_BISHOP] | 
        bitboards[BLACK_ROOK] | bitboards[BLACK_QUEEN] | bitboards[BLACK_KING];
        occupied = occupied_tables[WHITE] | occupied_tables[BLACK];
    }
    void find_valid_knight_moves() {
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t moves = 0;
            moves |= (position >> 17 & ~FILE_H);
            moves |= (position >> 15 & ~FILE_A);
            moves |= (position >> 10 & ~FILE_GH);
            moves |= (position >> 6 & ~FILE_AB);
            moves |= (position << 6 & ~FILE_GH);
            moves |= (position << 10 & ~FILE_AB);
            moves |= (position << 15 & ~FILE_H);
            moves |= (position << 17 & ~FILE_A);
            knight_attacks[cell] = moves;
        }
    }
    void find_valid_king_moves() {
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t moves = 0;
            moves |= (position >> 9 & ~FILE_H);
            moves |= (position >> 8);
            moves |= (position >> 7 & ~FILE_A);
            moves |= (position >> 1 & ~FILE_H);
            moves |= (position << 1 & ~FILE_A);
            moves |= (position << 7 & ~FILE_H);
            moves |= (position << 8);
            moves |= (position << 9 & ~FILE_A);
            king_moves[cell] = moves;
        }
    }
    void make_between_table() {
        for (int from { 0 }; from < 64; from++) {
            for (int to { 0 }; to < 64; to++) {
                int row_change = to / 8 - from / 8;
                int col_change = to % 8 - from % 8;
                uint64_t mask = 0ULL;
                if (std::abs(row_change) == std::abs(col_change) && from != to) {
                    int row_step = ((row_change < 0) ? -1 : 1);
                    int col_step = ((col_change < 0) ? -1 : 1);
                    int curr_square = from + 8 * row_step + col_step;
                    while (curr_square != to) {
                        mask |= (1ULL << curr_square);
                        curr_square += 8 * row_step + col_step;
                    }
                } else if (std::abs(row_change) == 0 && from != to) {
                    int col_step = ((col_change < 0) ? -1 : 1);
                    int curr_square = from + col_step;
                    while (curr_square != to) {
                        mask |= (1ULL << curr_square);
                        curr_square += col_step;
                    }
                } else if (std::abs(col_change) == 0 && from != to) {
                    int row_step = ((row_change < 0) ? -1 : 1);
                    int curr_square = from + 8 * row_step;
                    while (curr_square != to) {
                        mask |= (1ULL << curr_square);
                        curr_square += 8 * row_step;
                    }
                }
                between_table[from][to] = mask;
            }
        }
    }
    void find_pawn_attacks() {
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t non_capture_moves = 0ULL;
            uint64_t capture_moves = 0ULL;
            non_capture_moves |= (position << 8);
            capture_moves |= (position << 7 & ~FILE_H);
            capture_moves |= (position << 9 & ~FILE_A);
            if (position & RANK_2) {
                non_capture_moves |= (position << 16);
            }
            pawn_attacks[WHITE][cell] = capture_moves;
            pawn_moves[WHITE][cell] = non_capture_moves;
        }
        for (int cell { 0 }; cell < 64; cell++) {
            uint64_t position = 1ULL << cell;
            uint64_t non_capture_moves = 0ULL;
            uint64_t capture_moves = 0ULL;
            non_capture_moves |= (position >> 8);
            capture_moves |= (position >> 7 & ~FILE_A);
            capture_moves |= (position >> 9 & ~FILE_H);
            if (position & RANK_7) {
                non_capture_moves |= (position >> 16);
            }
            pawn_attacks[BLACK][cell] = capture_moves;
            pawn_moves[BLACK][cell] = non_capture_moves;
        }
    }

    constexpr void fill_attack_square(int square, int piece) {
        uint64_t mask = (piece == WHITE_BISHOP) ? get_bishop_mask(square) : get_rook_mask(square);
        if (piece == WHITE_BISHOP) {
            bishop_masks[square] = mask;
        } else {
            rook_masks[square] = mask;
        }
        int num_bits = __builtin_popcountll(mask);
        
        uint64_t blocker[4096], attack[4096], used[4096];
        for (int i { 0 }; i < (1 << num_bits); i++) {
            blocker[i] = set_occupancy(i, num_bits, mask);
            attack[i] = (piece == WHITE_BISHOP) ? find_bishop_attacks(square, blocker[i]) : find_rook_attacks(square, blocker[i]);
        }

        if (piece == WHITE_BISHOP) {
            for (int i = 0; i < (1 << num_bits); i++) {
                int magic_index = (int)((blocker[i] * bishop_magic_nums[square]) >> (64 - bishop_shifts[square]));
                bishop_attack_table[square][magic_index] = attack[i];
            }
        } else {
            for (int i = 0; i < (1 << num_bits); i++) {
                int magic_index = (int)((blocker[i] * rook_magic_nums[square]) >> (64 - rook_shifts[square]));
                rook_attack_table[square][magic_index] = attack[i];
            }
        }
    }
    constexpr void fill_attack_tables() {
        //std::cout << "initialising magic bitboards\n";
        for (int square { 0 }; square < 64; square++) {
            fill_attack_square(square, WHITE_ROOK);
            fill_attack_square(square, WHITE_BISHOP);
        }
        //std::cout << "done\n";
    }
};

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

uint64_t get_rook_attacks(int square, uint64_t occupancy, Bitboards& bitboards);
uint64_t get_bishop_attacks(int square, uint64_t occupancy, Bitboards& bitboards);





