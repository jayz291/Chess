#include "perft.h"

long long perft(Game& game, int depth) {
    if (depth == 0) {
        return 1ULL;
    }

    long long nodes = 0;
    
    Move_list moves = determine_possible_moves(game);

    for (int i { 0 }; i < moves.num_moves; i++) {
        auto& move = moves.list[i];

        make_test_move(game, move);
        nodes += perft(game, depth - 1);
        undo_test_move(game, move);
    }

    return nodes;
}

// Function to run the test and print detailed results
void run_perft_suite(Game& game, int depth) {
    std::cout << "Starting Perft Test Depth " << depth << "...\n";
    auto start = std::chrono::steady_clock::now();
    
    long long total_nodes = 0;
    
    Move_list moves = determine_possible_moves(game);
    
    // sort by alphabetical order
    std::sort(moves.list.begin(), moves.list.begin() + moves.num_moves, [](const Move& a, const Move& b) {
        return to_chess_notation(a) < to_chess_notation(b);
    });
    
    for (int i { 0 }; i < moves.num_moves; i++) {
        auto& move = moves.list[i];
        make_test_move(game, move);
        
        long long branches = perft(game, depth - 1);
        total_nodes += branches;
        std::cout << to_chess_notation(move) << ": " << branches << "\n";
        
        undo_test_move(game, move);
    }
    
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    std::cout << "\n===========================\n";
    std::cout << "Total Nodes: " << total_nodes << "\n";
    std::cout << "Time: " << elapsed.count() << " s\n";
    std::cout << "NPS: " << (long long)(total_nodes / elapsed.count());
    std::cout << "\n===========================\n";
}


std::string to_chess_notation(const Move& move) {

    if (move.get_from_square() == move.get_to_square()) {
        return "(none)";
    }
    std::string s = "";

    int prev_row = 7 - move.get_from_square() / 8;
    int prev_col = move.get_from_square() % 8;
    int new_row = 7 - move.get_to_square() / 8;
    int new_col = move.get_to_square() % 8;

    s += ('a' + prev_col);      
    s += ('8' - prev_row);     
    s += ('a' + new_col);
    s += ('8' - new_row);
    
    if (move.get_move_type() == PROMOTION) { 
        if (move.get_promotion_piece() == P_QUEEN) {
            s += 'q'; 
        } else if (move.get_promotion_piece() == P_ROOK) {
            s += 'r';
        } else if (move.get_promotion_piece() == P_BISHOP) {
            s += 'b';
        } else if (move.get_promotion_piece() == P_KNIGHT) {
            s += 'n';
        }
    }
    
    return s;
}

Move parse_move_string(Game& game, const std::string& s) {

    int prev_col = s[0] - 'a';
    int prev_row = '8' - s[1];
    int new_col = s[2] - 'a';
    int new_row = '8' - s[3];

    int prev_square = 56 - 8 * prev_row + prev_col;
    int new_square = 56 - 8 * new_row + new_col;
    int piece_type = game.board[prev_square];
    uint8_t captured = game.board[new_square];

    Move move;
    move.set_move(prev_square, new_square, piece_type, captured);
    
    //move.get_move_type() = 0; 

    if (s.length() == 5) {
        move.set_move_type(PROMOTION); 
        char promo_char = s[4];
        
        if (promo_char == 'q') {
            move.set_promotion_piece(P_QUEEN);
        } else if (promo_char == 'r') {
            move.set_promotion_piece(P_ROOK);
        } else if (promo_char == 'b') {
            move.set_promotion_piece(P_BISHOP);
        } else if (promo_char == 'n') {
            move.set_promotion_piece(P_KNIGHT);
        }
    }

    if ((piece_type == BLACK_KING || piece_type == WHITE_KING) && std::abs(new_col - prev_col) == 2) {
        move.set_move_type(CASTLING);
    }

    if ((piece_type == BLACK_PAWN || piece_type == WHITE_PAWN) && prev_col != new_col && captured == EMPTY_SQUARE) {
        move.set_move_type(EN_PASSANT);
        move.set_captured(game.board[56 - 8 * prev_row + new_col]); 
    }

    return move;
}

