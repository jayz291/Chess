#include "definitions.h"

uint64_t Bitboards::knight_attacks[64];
uint64_t Bitboards::king_moves[64];
uint64_t Bitboards::pawn_attacks[2][64];
uint64_t Bitboards::pawn_moves[2][64];
uint64_t Bitboards::between_table[64][64];
uint64_t Bitboards::rook_attack_table[64][4096];
uint64_t Bitboards::bishop_attack_table[64][512];
uint64_t Bitboards::rook_masks[64];
uint64_t Bitboards::bishop_masks[64];
bool Bitboards::initialised = false;

uint64_t Bitboards::rook_magic_nums[64] = {
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

uint64_t Bitboards::bishop_magic_nums[64] = {
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

int Bitboards::rook_shifts[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

int Bitboards::bishop_shifts[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

Bitboards::Bitboards() {
    init_bitboards();
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

void Bitboards::init_bitboards() {
    for (int i { 0 }; i <= 14; i++) {
        bitboards[i] = 0ULL;
    }
}

void Bitboards::update_occupied() {
    occupied_tables[WHITE] = bitboards[WHITE_PAWN] | bitboards[WHITE_KNIGHT] | bitboards[WHITE_BISHOP] | 
    bitboards[WHITE_ROOK] | bitboards[WHITE_QUEEN] | bitboards[WHITE_KING];
    occupied_tables[BLACK] = bitboards[BLACK_PAWN] | bitboards[BLACK_KNIGHT] | bitboards[BLACK_BISHOP] | 
    bitboards[BLACK_ROOK] | bitboards[BLACK_QUEEN] | bitboards[BLACK_KING];
    occupied = occupied_tables[WHITE] | occupied_tables[BLACK];
}

void Bitboards::find_valid_knight_moves() {
    for (int cell { 0 }; cell < 64; cell++) {
        uint64_t position = 1ULL << cell;
        uint64_t moves = 0;
        moves |= (position >> 17 & ~FILE_MASKS[7]);
        moves |= (position >> 15 & ~FILE_MASKS[0]);
        moves |= (position >> 10 & ~FILE_GH);
        moves |= (position >> 6 & ~FILE_AB);
        moves |= (position << 6 & ~FILE_GH);
        moves |= (position << 10 & ~FILE_AB);
        moves |= (position << 15 & ~FILE_MASKS[7]);
        moves |= (position << 17 & ~FILE_MASKS[0]);
        knight_attacks[cell] = moves;
    }
}

void Bitboards::find_valid_king_moves() {
    for (int cell { 0 }; cell < 64; cell++) {
        uint64_t position = 1ULL << cell;
        uint64_t moves = 0;
        moves |= (position >> 9 & ~FILE_MASKS[7]);
        moves |= (position >> 8);
        moves |= (position >> 7 & ~FILE_MASKS[0]);
        moves |= (position >> 1 & ~FILE_MASKS[7]);
        moves |= (position << 1 & ~FILE_MASKS[0]);
        moves |= (position << 7 & ~FILE_MASKS[7]);
        moves |= (position << 8);
        moves |= (position << 9 & ~FILE_MASKS[0]);
        king_moves[cell] = moves;
    }
}

void Bitboards::make_between_table() {
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

uint64_t Bitboards::set_occupancy(int index, int num_bits, uint64_t attack_mask) {
    uint64_t occupancy = 0ULL;
    for (int i { 0 }; i < num_bits; i++) {
        int square = __builtin_ctzll(attack_mask);
        attack_mask &= attack_mask - 1;  // remove bit

        if (index & (1ULL << i)) {
            occupancy |= (1ULL << square);
        }
    }
    return occupancy;
}

// function to prevent board wraparounds
bool Bitboards::determine_square_validity(int square, int direction) {
    int rank = square / 8;
    int file = square % 8;
    if ((direction == 9 || direction == -7 || direction == 1) && file == 7) {
        return false;
    } else if ((direction == -9 || direction == 7 || direction == -1) && file == 0) {
        return false;
    } else if ((direction >= 7 && rank == 7) || (direction <= -7 && rank == 0)) {
        return false;
    } 
    return true; 
}

void Bitboards::find_pawn_attacks() {
    for (int cell { 0 }; cell < 64; cell++) {
        uint64_t position = 1ULL << cell;
        uint64_t non_capture_moves = 0ULL;
        uint64_t capture_moves = 0ULL;
        non_capture_moves |= (position << 8);
        capture_moves |= (position << 7 & ~FILE_MASKS[7]);
        capture_moves |= (position << 9 & ~FILE_MASKS[0]);
        if (position & RANK_MASKS[1]) {
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
        capture_moves |= (position >> 7 & ~FILE_MASKS[0]);
        capture_moves |= (position >> 9 & ~FILE_MASKS[7]);
        if (position & RANK_MASKS[6]) {
            non_capture_moves |= (position >> 16);
        }
        pawn_attacks[BLACK][cell] = capture_moves;
        pawn_moves[BLACK][cell] = non_capture_moves;
    }
}

uint64_t Bitboards::find_rook_attacks(int square, uint64_t& occupied) {
    int directions[4] = {-1, 1, 8, -8};
    int curr = square;
    uint64_t attacks = 0ULL;
    for (int i { 0 }; i < 4; i++) {
        int direction = directions[i];
        curr = square;
        while (determine_square_validity(curr, direction) == true) {
            curr += direction;
            uint64_t mask = 1ULL << curr;
            attacks |= mask;
            if (occupied & mask) {
                break;
            }
        }
    }
    return attacks;
}

uint64_t Bitboards::find_bishop_attacks(int square, uint64_t& occupied) {
    int directions[4] = {7, -7, 9, -9};
    int curr = square;
    uint64_t attacks = 0ULL;
    for (int i { 0 }; i < 4; i++) {
        int direction = directions[i];
        curr = square;
        while (determine_square_validity(curr, direction) == true) {
            curr += direction;
            uint64_t mask = 1ULL << curr;
            attacks |= mask;
            if (occupied & mask) {
                break;
            }
        }
    }
    return attacks;
}

uint64_t Bitboards::get_rook_mask(int square) {
    uint64_t attacks = 0ULL;
    int row = 7 - square / 8;
    int col = square % 8;
    for (int i = row + 1; i < 7; i++) {
        attacks |= (1ULL << (56 - 8 * i + col));
    }
    for (int i = row - 1; i > 0; i--) {
        attacks |= (1ULL << (56 - 8 * i + col));
    }
    for (int i = col + 1; i < 7; i++) {
        attacks |= (1ULL << (56 - 8 * row + i));
    }
    for (int i = col - 1; i > 0; i--) {
        attacks |= (1ULL << (56 - 8 * row + i));
    }
    return attacks;
}

uint64_t Bitboards::get_bishop_mask(int square) {
    uint64_t attacks = 0ULL;
    int row = 7 - square / 8;
    int col = square % 8;
    for (int i = row + 1, j = col + 1; i < 7 && j < 7; i++, j++) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    for (int i = row + 1, j = col - 1; i < 7 && j > 0; i++, j--) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    for (int i = row - 1, j = col + 1; i > 0 && j < 7; i--, j++) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    for (int i = row - 1, j = col - 1; i > 0 && j > 0; i--, j--) {
        attacks |= (1ULL << (56 - 8 * i + j));
    }
    return attacks;
}

constexpr void Bitboards::fill_attack_square(int square, int piece) {
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

constexpr void Bitboards::fill_attack_tables() {
    //std::cout << "initialising magic bitboards\n";
    for (int square { 0 }; square < 64; square++) {
        fill_attack_square(square, WHITE_ROOK);
        fill_attack_square(square, WHITE_BISHOP);
    }
}



