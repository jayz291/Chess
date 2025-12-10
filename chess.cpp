#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <utility>
#include <algorithm>


struct Piece {
    std::string piece_type {};
    std::string colour {};
    bool moved { false };
    int row;
    int col; 
};

struct Cell {
    std::string colour {};
    std::shared_ptr<Piece> piece_occupying { nullptr };
    bool selected { false };
};

struct Coords {
    int row { -1 };
    int col { -1 };
};

struct Move {
    int prev_row {};
    int prev_col {};
    int new_row {};
    int new_col {};
};

using Chessboard = std::array<std::array<Cell, 8>, 8>;

class Game {
    public:
        //std::vector<Piece> pieces{32}; 
        Chessboard board {};
        Coords selected {};
        std::string turn { "white" };
        bool white_in_check { false };
        bool black_in_check { false };
        Coords black_king_position { 0, 4 };
        Coords white_king_position { 7, 4 };
        bool game_over { false };
        bool checkmate { false };
        bool stalemate { false };
        std::string winner {};
        bool promoting_pawn { false };
        std::string piece_selected { "None" };
    Game() {
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if ((i + j) % 2 == 0) {
                    board[i][j].colour = "yellow";
                } else {
                    board[i][j].colour = "brown";
                }
            }
        }
        for (int i { 0 }; i < 8; i++) {
            board[1][i].piece_occupying = std::make_shared<Piece> ("pawn", "black", false, 1, i);
        }
        for (int i { 0 }; i < 8; i++) {
            board[6][i].piece_occupying = std::make_shared<Piece> ("pawn", "white", false, 6, i);
        }
        board[0][0].piece_occupying = std::make_shared<Piece> ("rook", "black", false, 0, 0);
        board[0][1].piece_occupying = std::make_shared<Piece> ("knight", "black", false, 0, 1);
        board[0][2].piece_occupying = std::make_shared<Piece> ("bishop", "black", false, 0, 2);
        board[0][3].piece_occupying = std::make_shared<Piece> ("queen", "black", false, 0, 3);
        board[0][4].piece_occupying = std::make_shared<Piece> ("king", "black", false, 0, 4);
        board[0][5].piece_occupying = std::make_shared<Piece> ("bishop", "black", false, 0, 5);
        board[0][6].piece_occupying = std::make_shared<Piece> ("knight", "black", false, 0, 6);
        board[0][7].piece_occupying = std::make_shared<Piece> ("rook", "black", false, 0, 7);
        board[7][0].piece_occupying = std::make_shared<Piece> ("rook", "white", false, 7, 0);
        board[7][1].piece_occupying = std::make_shared<Piece> ("knight", "white", false, 7, 1);
        board[7][2].piece_occupying = std::make_shared<Piece> ("bishop", "white", false, 7, 2);
        board[7][3].piece_occupying = std::make_shared<Piece> ("queen", "white", false, 7, 3);
        board[7][4].piece_occupying = std::make_shared<Piece> ("king", "white", false, 7, 4);
        board[7][5].piece_occupying = std::make_shared<Piece> ("bishop", "white", false, 7, 5);
        board[7][6].piece_occupying = std::make_shared<Piece> ("knight", "white", false, 7, 6);
        board[7][7].piece_occupying = std::make_shared<Piece> ("rook", "white", false, 7, 7);
    }
};

void draw_board(Game& game, sf::RenderWindow& window);
void draw_piece(Game& game, sf::RenderWindow& window, std::shared_ptr<Piece>& piece);
void select_square(int x, int y, Game& game, sf::RenderWindow& window);
void select_pawn_promotion(Game& game, sf::RenderWindow& window, int pawn_row, int pawn_col);
void move_piece(Chessboard& board, int prev_row, int prev_col, int new_row, int new_col);
int determine_possible_moves(Game& game);
void draw_end_screen(Game& game, sf::RenderWindow& window);
void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window);
void end_game(Game& game);

int validate_move(Game& game, Chessboard& board, Move& move, std::string& turn, bool only_checking_checks = false);
int validate_move_pawn(Game& game, Chessboard& board, Move& move, std::string& turn);
int validate_move_knight(Game& game, Chessboard& board, Move& move);
int validate_move_bishop(Game& game, Chessboard& board, Move& move);
int validate_move_rook(Game& game, Chessboard& board, Move& move);
int validate_move_queen(Game& game, Chessboard& board, Move& move);
int validate_move_king(Game &game, Chessboard& board, Move& move, std::string& turn);
int check_checks(Game &game, Chessboard& copy, Move& move, std::string& turn);
void evaluate_king_checks(Game& game);
int test_castling(Game &game, Chessboard& copy, int prev_row, int prev_col, 
    int new_row, int new_col, std::string& turn);
void handle_pawn_promotion(Game& game, int& row, int& col);

int main() {
    Game game {};
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    sf::RectangleShape board({760, 760});
    bool end_screen_clicked { false };
    while (window.isOpen()) {
        
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (!game.game_over) {
                    select_square(mouse_press->position.x, mouse_press->position.y, game, window);
                } else if (game.game_over) {
                    end_screen_clicked = true;
                } 
            }
               
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
                window.setView(sf::View(visibleArea));
            }
        }
        window.clear(sf::Color::Red);
        draw_board(game, window);
        
        if (game.game_over && !end_screen_clicked) {
            draw_end_screen(game, window);
        } else if (game.promoting_pawn && !game.game_over) {
            draw_pawn_promotion_screen(game, window);
        }

        window.display();

    }

}

void draw_board(Game& game, sf::RenderWindow& window) {
    float x_offset {}, y_offset {};
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            sf::RectangleShape cell({760 / 8, 760 / 8});
            x_offset = 120 + j * (760 / 8);
            y_offset = 20 + i * (760 / 8);
            
            if (game.board[i][j].colour == "brown") {
                cell.setFillColor(sf::Color(165, 42, 42));
            } else {
                cell.setFillColor(sf::Color::Yellow);
            }
            if (game.board[i][j].selected) {
                //std::cout << i << " " << j << '\n';
                //std::cout << x_offset << " " << y_offset << '\n';
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);
            }
            cell.setPosition({x_offset, y_offset});
            window.draw(cell);
        }
    }
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying != nullptr) {
                draw_piece(game, window, game.board[i][j].piece_occupying);
                
            }
        }
    }

}

void draw_end_screen(Game& game, sf::RenderWindow& window) {
    sf::RectangleShape end_screen({500, 300});
    sf::Font font;
    if (!font.openFromFile("./src/HeadingNowTrial-14Regular.ttf")) {
        return;
    }
    sf::Text text(font);
    text.setFillColor(sf::Color::Red);
    text.setCharacterSize(100);
    ;
    if (game.checkmate) {
        text.setPosition({420, 240});
        if (game.winner == "white") {
            text.setString("White won");
        } else {
            text.setString("Black won");
        }
    } else if (game.stalemate) {
        text.setPosition({380, 240});
        text.setString("It is a draw by stalemate");
    } else {
        text.setString("Default");
    }

    float x_offset = 250;
    float y_offset = 250;
    end_screen.setPosition({x_offset, y_offset});
    end_screen.setFillColor(sf::Color::Black);
    window.draw(end_screen);
    window.draw(text);
}

void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window) {
    sf::RectangleShape pawn_promotion_screen({500, 300});
    float x_offset = 250;
    float y_offset = 250;
    pawn_promotion_screen.setPosition({x_offset, y_offset});
    pawn_promotion_screen.setFillColor(sf::Color::Blue);
    window.draw(pawn_promotion_screen);
    std::shared_ptr<Piece> piece1 = std::make_shared<Piece> ("rook", game.turn, false, 4, 2);
    std::shared_ptr<Piece> piece2 = std::make_shared<Piece> ("knight", game.turn, false, 4, 3);
    std::shared_ptr<Piece> piece3 = std::make_shared<Piece> ("bishop", game.turn, false, 4, 4);
    std::shared_ptr<Piece> piece4 = std::make_shared<Piece> ("queen", game.turn, false, 4, 5);
    
    std::vector<std::shared_ptr<Piece>> promotion_pieces { piece1, piece2, piece3, piece4 };
    for (auto piece: promotion_pieces) {
        draw_piece(game, window, piece);
    }
    window.display();
}

void draw_piece(Game& game, sf::RenderWindow& window, std::shared_ptr<Piece>& piece) {
    //std::string piece_type = piece->piece_type;
    sf::Texture texture;

    if (piece->piece_type == "pawn" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_pdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "pawn" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_plt45.png")) {
            return;
        }
    } else if (piece->piece_type == "knight" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_ndt45.png")) {
            return;
        }
    } else if (piece->piece_type == "knight" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_nlt45.png")) {
            return;
        }
    }  else if (piece->piece_type == "bishop" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_bdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "bishop" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_blt45.png")) {
            return;
        }
    } else if (piece->piece_type == "rook" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_rdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "rook" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_rlt45.png")) {
            return;
        }
    } else if (piece->piece_type == "queen" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_qdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "queen" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_qlt45.png")) {
            return;
        }
    } else if (piece->piece_type == "king" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_kdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "king" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_klt45.png")) {
            return;
        }
    }

    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});
    float x_offset { static_cast<float>(135 + piece->col * (760 / 8)) };
    float y_offset { static_cast<float>(30 + piece->row * (760 / 8)) };
    if (piece->piece_type == "king") {
        if (piece->colour == "white" && game.white_in_check && game.turn == "white") {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        } else if (piece->colour == "black" && game.black_in_check && game.turn == "black") {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        }
    }
    sprite.setPosition({x_offset, y_offset});
    window.draw(sprite);

}

void select_square(int x, int y, Game& game, sf::RenderWindow& window) {
    int col = floor(((x - 135.f) / (760 / 8)) + 0.1473);
    int row = floor(((y - 30.f) / (760 / 8)) + 0.0842105);
    std::cout << "Coords - row: " << row << " " << "col: "<< col << '\n';
    if (row < 0) {
        row = 0;
    } else if (col < 0) {
        col = 0;
    }
    if (row > 7) {
        row = 7;
    } else if (col > 7) {
        col = 7;
    }

    if (game.selected.row > -1 && game.selected.col > -1) {
        //std::cout << game.selected.row << ' ' << game.selected.col << '\n';
        //std::cout << row << ' ' << col << '\n';
        Move move { game.selected.row, game.selected.col, row, col };
        int result = validate_move(game, game.board, move, game.turn);
        if (result >= 0) {
            move_piece(game.board, game.selected.row, game.selected.col, row, col);

            if (result == 1 && game.turn == "white") { 
                move_piece(game.board, 7, 7, 7, 5);
            } else if (result == 2 && game.turn == "white") {
                move_piece(game.board, 7, 0, 7, 3);
            } else if (result == 1 && game.turn == "black") {
                move_piece(game.board, 0, 7, 0, 5);
            } else if (result == 2 && game.turn == "black") {
                move_piece(game.board, 0, 0, 0, 3);
            }

            if (game.board[row][col].piece_occupying->piece_type == "king") {
                if (game.turn == "white") {
                    game.white_king_position.row = row;
                    game.white_king_position.col = col;
                } else {
                    game.black_king_position.row = row;
                    game.black_king_position.col = col;
                }
            }
            if ((game.turn == "black" && game.board[row][col].piece_occupying->piece_type == "pawn" && row == 7) || 
                (game.turn == "white" && game.board[row][col].piece_occupying->piece_type == "pawn" && row == 0)) {
                game.promoting_pawn = true;
                std::cout << "pawn promotion";
                draw_pawn_promotion_screen(game, window);
                while (game.promoting_pawn) {
                    
                    select_pawn_promotion(game, window, row, col);
                }
            }
            if (game.turn == "white") {
                game.turn = "black";
            } else {
                game.turn = "white";
            }
            evaluate_king_checks(game);
            if (determine_possible_moves(game) == -1) {
                end_game(game);
            }
            game.board[game.selected.row][game.selected.col].selected = false;
            game.selected.row = game.selected.col = -1;
            return;
        } else {

            game.board[game.selected.row][game.selected.col].selected = false;
            game.selected.row = game.selected.col = -1;           
        }
    } else if (!game.board[row][col].piece_occupying || game.board[row][col].piece_occupying->colour == game.turn) {
        game.board[row][col].selected = true;
        game.selected.row = row;
        game.selected.col = col;
    }
}

void select_pawn_promotion(Game& game, sf::RenderWindow& window, int pawn_row, int pawn_col) {
    int x, y;
    const std::optional event = window.pollEvent();
   
    if (event) {
        const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>();
        if (mouse_press) {
            x = mouse_press->position.x;
            y = mouse_press->position.y;
        }
    }
    
    
    int col = floor(((x - 135.f) / (760 / 8)) + 0.1473);
    int row = floor(((y - 30.f) / (760 / 8)) + 0.0842105);
    if (row < 0) {
        row = 0;
    } else if (col < 0) {
        col = 0;
    }
    if (row > 7) {
        row = 7;
    } else if (col > 7) {
        col = 7;
    }
    if (row == 4 && col == 2) {
        game.piece_selected = "rook";
    } else if (row == 4 && col == 3) {
        game.piece_selected = "knight";
    } else if (row == 4 && col == 4) {
        game.piece_selected = "bishop";
    } else if (row == 4 && col == 5) {
        game.piece_selected = "queen";
    }
    if (game.piece_selected != "None") {
        handle_pawn_promotion(game, pawn_row, pawn_col);
        return;
    }
}

void handle_pawn_promotion(Game& game, int& row, int& col) {

    game.board[row][col].piece_occupying->piece_type = game.piece_selected;
    game.piece_selected = "None";
    game.promoting_pawn = false;
}

void evaluate_king_checks(Game& game) {
    Move move { 0, 0, 0, 0 };
    if (game.turn == "white") {
        std::string next = "black";
        int new_result = check_checks(game, game.board, move, game.turn);
        if (new_result == 1) {
            game.white_in_check = true;
        } else {
            game.white_in_check = false;
        }
    } else {
        std::string next = "white";
        int new_result = check_checks(game, game.board, move, game.turn);
        if (new_result == 1) {
            game.black_in_check = true;
        } else {
            game.black_in_check = false;
        }
    }
}

void end_game(Game& game) {
    std::cout << "It is over\n";
    game.game_over = true;
    if (game.turn == "black") {
        if (game.black_in_check) {
            game.checkmate = true;
            game.winner = "white";
        } else {
            game.stalemate = true;
        }
    } else {
        if (game.white_in_check) {
            game.checkmate = true;
            game.winner = "black";
        } else {
            game.stalemate = true;
        }
    }
}

void move_piece(Chessboard& board, int prev_row, int prev_col, int new_row, int new_col) {
    board[new_row][new_col].piece_occupying = nullptr;
    board[new_row][new_col].piece_occupying = std::move(board[prev_row][prev_col].piece_occupying);
    if (board[new_row][new_col].piece_occupying == nullptr) {
        std::cout << "failed\n";
        return;
    }
    board[new_row][new_col].piece_occupying->row = new_row;
    board[new_row][new_col].piece_occupying->col = new_col;

    if (!board[new_row][new_col].piece_occupying->moved) {
        board[new_row][new_col].piece_occupying->moved = true;
    }
}

int validate_move(Game& game, Chessboard& board, Move& move, std::string& turn, bool only_checking_checks) {
    int result { 0 };
    if (move.new_row > 7 || move.new_col > 7 || move.new_row < 0 || move.new_col < 0) {
        return -1;
    }
    if ((move.prev_row != move.new_row || move.prev_col != move.new_col) && 
        (!board[move.new_row][move.new_col].piece_occupying || 
        board[move.new_row][move.new_col].piece_occupying->colour != turn)) {
        
        std::string piece = board[move.prev_row][move.prev_col].piece_occupying->piece_type;
        if (piece == "pawn") {
            result = validate_move_pawn(game, board, move, turn);
        } else if (piece == "knight") {
            result = validate_move_knight(game, board, move);
        } else if (piece == "bishop") {
            result = validate_move_bishop(game, board, move);
        } else if (piece == "rook") {
            result = validate_move_rook(game, board, move);
        } else if (piece == "queen") {
            result = validate_move_queen(game, board, move);
        } else if (piece == "king") {
            result = validate_move_king(game, board, move, turn);
        } else {
            return 0;
        }
        if (result >= 0 && !only_checking_checks) {
            Chessboard copy {};
            for (int i { 0 }; i < 8; i++) {
                for (int j { 0 }; j < 8; j++) {
                    copy[i][j].piece_occupying = board[i][j].piece_occupying;
                }
            }
            int new_result = check_checks(game, copy, move, game.turn);
            std::cout << new_result << '\n';
            if (new_result == 0) {
                return result;
            }
            return -1;
        } else if (result != 0) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

int validate_move_pawn(Game& game, Chessboard& board, Move& move, std::string& turn) {
    std::vector<std::pair<int, int>> no_capture_moves {};
    std::vector<std::pair<int, int>> capture_moves {};
    int prev_row { move.prev_row };
    int prev_col { move.prev_col };
    int new_row { move.new_row };
    int new_col { move.new_col };
    std::pair<int, int> new_move { new_row, new_col };
    std::vector<std::pair<int, int>>::iterator it {};

    if (turn == "white") {
        if (prev_row == 6 && !board[prev_row - 1][prev_col].piece_occupying) {
            no_capture_moves = { { prev_row - 1, prev_col }, { prev_row - 2, prev_col } };
        } else {
            no_capture_moves = { { prev_row - 1, prev_col } };
        }
        capture_moves = { { prev_row - 1, prev_col - 1 }, { prev_row - 1, prev_col + 1 } };

    } else {
        if (prev_row == 1 && !board[prev_row + 1][prev_col].piece_occupying) {
            no_capture_moves = { { prev_row + 1, prev_col }, { prev_row + 2, prev_col } };
        } else {
            no_capture_moves = { { prev_row + 1, prev_col } };
        }
        capture_moves = { { prev_row + 1, prev_col - 1 }, { prev_row + 1, prev_col + 1 } };
    }

    if (board[new_row][new_col].piece_occupying) {
        it = std::find(capture_moves.begin(), capture_moves.end(), new_move);
        if (it != capture_moves.end()) {
            return 0;
        } else {
            return -1;
        }
    } else {
        it = std::find(no_capture_moves.begin(), no_capture_moves.end(), new_move);
        if (it != no_capture_moves.end()) {
            return 0;
        } else {
            return -1;
        }
    }
    
}

int validate_move_knight(Game& game, Chessboard& board, Move& move) {
    int prev_row { move.prev_row };
    int prev_col { move.prev_col };
    int new_row { move.new_row };
    int new_col { move.new_col };
    std::pair<int, int> new_move { new_row, new_col };
    std::vector<std::pair<int, int>> moves { { prev_row + 1, prev_col + 2 }, { prev_row + 2, prev_col + 1}, 
    { prev_row - 1, prev_col - 2 }, { prev_row - 2, prev_col - 1 }, { prev_row + 1, prev_col - 2 }, 
    { prev_row - 1, prev_col + 2 }, { prev_row - 2, prev_col + 1 }, { prev_row + 2, prev_col - 1 } };
    auto it = std::find(moves.begin(), moves.end(), new_move);
    if (it != moves.end()) {
        return 0;
    } else {
        return -1;
    }
}

int validate_move_bishop(Game& game, Chessboard& board, Move& move) {
    int prev_row { move.prev_row };
    int prev_col { move.prev_col };
    int new_row { move.new_row };
    int new_col { move.new_col };
    int row_change { new_row - prev_row };
    int col_change { new_col - prev_col };

    std::cout << "prev row: " << prev_row << " prev_col: " << prev_col << '\n';
    std::cout << "new row: " << new_row << " new col: " << new_col << '\n';
    std::cout << "row change: " << row_change << " column change: " << col_change << '\n';
    if (std::abs(row_change) == std::abs(col_change)) {
        if (row_change > 0 && col_change > 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[prev_row + i][prev_col + i].piece_occupying) {
                    return -1;
                }
            }
        } else if (row_change < 0 && col_change > 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[prev_row - i][prev_col + i].piece_occupying) {
                    return -1;
                }
            }           
        } else if (row_change > 0 && col_change < 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[prev_row + i][prev_col - i].piece_occupying) {
                    return -1;
                }
            }           
        } else if (row_change < 0 && col_change < 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[prev_row - i][prev_col - i].piece_occupying) {
                    return -1;
                }
            }              
        }
        return 0;
    }
    return -1;
}

int validate_move_rook(Game& game, Chessboard& board, Move& move) {
    int prev_row { move.prev_row };
    int prev_col { move.prev_col };
    int new_row { move.new_row };
    int new_col { move.new_col };
    int row_change { new_row - prev_row };
    int col_change { new_col - prev_col };

    if (row_change == 0) {
        if (col_change > 0) {
            for (int i { 1 }; i < std::abs(col_change); i++) {
                if (board[prev_row][prev_col + i].piece_occupying) {
                    return -1;
                }
            } 
        } else if (col_change < 0) {
            for (int i { 1 }; i < std::abs(col_change); i++) {
                if (board[prev_row][prev_col - i].piece_occupying) {
                    return -1;
                }
            }         
        }
        return 0;
    } else if (col_change == 0) {
        if (row_change > 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[prev_row + i][prev_col].piece_occupying) {
                    return -1;
                }
            } 
        } else if (row_change < 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[prev_row - i][prev_col].piece_occupying) {
                    return -1;
                }
            }         
        }
        return 0;
    }
    return -1;
}

int validate_move_queen(Game& game, Chessboard& board, Move& move) {
    int rook_move = validate_move_rook(game, board, move);
    int bishop_move = validate_move_bishop(game, board, move);
    std::cout << "rook: " << rook_move << " bishop: " << bishop_move << '\n';
    if (rook_move == 0 || bishop_move == 0) {
        return 0;
    } else {
        return -1;
    }
}

int validate_move_king(Game &game, Chessboard& board, Move& move, std::string& turn) {
    std::cout << "Checking king move\n";

    int prev_row { move.prev_row };
    int prev_col { move.prev_col };
    int new_row { move.new_row };
    int new_col { move.new_col };
    int row_change { new_row - prev_row };
    int col_change { new_col - prev_col };
    std::vector<std::pair<int, int>> moves { { new_row + 1, new_col }, { new_row - 1, new_col }, 
    { new_row + 1, new_col + 1 }, { new_row - 1, new_col - 1 }, 
    { new_row - 1, new_col + 1 }, { new_row + 1, new_col - 1 } };

    //std::cout << "Distance: " << sqrt(pow(row_change, 2) + pow(col_change, 2)) << '\n';
    if (sqrt(pow(row_change, 2) + pow(col_change, 2)) <= sqrt(2)) {
        for (auto coord: moves) {
            if (coord.first >= 0 && coord.first <= 7 && coord.second >= 0 && coord.second <= 7 &&
                board[coord.first][coord.second].piece_occupying && 
                board[coord.first][coord.second].piece_occupying->piece_type == "king" && 
                board[coord.first][coord.second].piece_occupying->colour != turn) {
                return -1;
            }
        }
    
        return 0;
    } else if (row_change == 0 && std::abs(col_change) == 2) {
        Chessboard copy {};
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                copy[i][j].piece_occupying = board[i][j].piece_occupying;
            }
        }
        std::cout << "testing..\n";
        return test_castling(game, copy, prev_row, prev_col, new_row, new_col, turn);
    }
    return -1;
}

int check_checks(Game &game, Chessboard& copy, Move& move, std::string& turn) {
    int test {};
    Coords white_king_position {};
    Coords black_king_position {};
    std::cout << "checking checks\n";
    int prev_row { move.prev_row };
    int prev_col { move.prev_col };
    int new_row { move.new_row };
    int new_col { move.new_col };
    if (prev_row != new_row || prev_col != new_col) {
        copy[new_row][new_col].piece_occupying = nullptr;
        copy[new_row][new_col].piece_occupying = std::move(copy[prev_row][prev_col].piece_occupying);
    }
    if (turn == "white") {
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == "white" &&
                    copy[i][j].piece_occupying->piece_type == "king") {
                    white_king_position.row = i;
                    white_king_position.col = j;
                    break;
                }
            }
        }   
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == "black" &&
                    copy[i][j].piece_occupying->piece_type != "king") {
                    std::string other_turn {"black"};
                    Move test_move { i, j, white_king_position.row, white_king_position.col };
                    test = validate_move(game, copy, test_move, other_turn, true);
                    if (test == 0) {
                        std::cout << "prev: " << i << ' ' << j << " new: " << new_row << ' ' << new_col << '\n';
                        return 1;
                    }
                }
            }
        }
        return 0;
    } else {
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == "black" &&
                    copy[i][j].piece_occupying->piece_type == "king") {
                    black_king_position.row = i;
                    black_king_position.col = j;
                    break;
                }
            }
        }   
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == "white" && 
                    copy[i][j].piece_occupying->piece_type != "king") {
                    std::string other_turn {"white"};
                    Move test_move { i, j, black_king_position.row, black_king_position.col };
                    test = validate_move(game, copy, test_move, other_turn, true);
                    if (test == 0) {
                        std::cout << "prev: " << i << ' ' << j << " new: " << new_row << ' ' << new_col << '\n';
                        return 1;
                    }
                }
            }
        }
        return 0;       
    }
}

int test_castling(Game &game, Chessboard& copy, int prev_row, int prev_col, 
    int new_row, int new_col, std::string& turn) {
    if (turn == "white" && prev_row == 7 && prev_col == 4 && !game.white_in_check) {
        if (!copy[7][4].piece_occupying->moved) {
            if (new_row == 7 && new_col == 6 && copy[7][7].piece_occupying && 
                copy[7][7].piece_occupying->piece_type == "rook" && 
                !copy[7][7].piece_occupying->moved) {
                
                for (int i { 5 }; i < 7; i++) {
                    if (copy[7][i].piece_occupying) {
                        return -1;
                    } else {
                        Move move { prev_row, prev_col, new_row, i };
                        if (check_checks(game, copy, move, turn) == 1) {
                            std::cout << "castlefail\n";
                            return -1;
                        }
                    }
                }
                return 1;
            } else if (new_row == 7 && new_col == 2 && copy[7][0].piece_occupying && 
                copy[7][0].piece_occupying->piece_type == "rook" &&
                !copy[7][0].piece_occupying->moved) {
                for (int i { 3 }; i > 1; i--) {
                    if (copy[7][i].piece_occupying) {
                        return -1;
                    } else {
                        Move move { prev_row, prev_col, new_row, i };
                        if (check_checks(game, copy, move, turn) == 1) {
                            return -1;
                        }
                    }
                }
                return 2;
            }
        }
        return -1;
    } else if (turn == "black" && prev_row == 0 && prev_col == 4 && !game.black_in_check) {
        if (!copy[0][4].piece_occupying->moved) {
            if (new_row == 0 && new_col == 6 && copy[0][7].piece_occupying && 
                copy[0][7].piece_occupying->piece_type == "rook" && 
                !copy[0][7].piece_occupying->moved) {
                for (int i { 5 }; i < 7; i++) {
                    if (copy[0][i].piece_occupying) {
                        return -1;
                    } else {
                        Move move { prev_row, prev_col, new_row, i };
                        if (check_checks(game, copy, move, turn) == 1) {
                            return -1;
                        }
                    }
                }
                return 1;
            } else if (new_row == 0 && new_col == 2 && copy[0][0].piece_occupying && 
                copy[0][0].piece_occupying->piece_type == "rook" &&
                !copy[0][0].piece_occupying->moved) {
                for (int i { 3 }; i > 1; i--) {
                    if (copy[0][i].piece_occupying) {
                        return -1;
                    } else {
                        Move move { prev_row, prev_col, new_row, i };
                        if ((check_checks(game, copy, move, turn) == 1)) {
                            return -1;
                        }
                    }
                }
                return 2;
            }
        }
        return -1;
    } else {
        return -1;
    }
}

int determine_possible_moves(Game& game) {
    int result {};

    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying && game.board[i][j].piece_occupying->colour == game.turn) {
                if (game.board[i][j].piece_occupying->piece_type == "pawn" && game.turn == "white") {
                    std::vector<std::pair<int, int>> possible_moves { { i - 1, j }, 
                    { i - 2, j }, { i - 1, j + 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second };
                        result = validate_move(game, game.board, move, game.turn);
                        if (result == 0) {
                            return 0;
                        }
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "pawn" && game.turn == "black") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j }, 
                    { i + 2, j }, { i + 1, j + 1 }, { i + 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second };
                        result = validate_move(game, game.board, move, game.turn);
                        if (result == 0) {
                            return 0;
                        }
                    }                   
                } else if (game.board[i][j].piece_occupying->piece_type == "knight") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 2 }, { i + 2, j + 1 }, 
                    { i - 1, j - 2 }, { i - 2, j - 1 }, { i + 1, j - 2 }, 
                    { i - 1, j + 2 }, { i - 2, j + 1 }, { i + 2, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second };
                        result = validate_move(game, game.board, move, game.turn);
                        if (result == 0) {
                            return 0;
                        }
                    }  
                } else if (game.board[i][j].piece_occupying->piece_type == "bishop") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j - k };
                        Move move2 { i, j, i + k, j + k };
                        Move move3 { i, j, i - k, j + k };
                        Move move4 { i, j, i - k, j - k };
                        std::vector<Move> moves { move1, move2, move3, move4 };
                        for (Move move: moves) {
                            result = validate_move(game, game.board, move, game.turn);
                            if (result == 0) {
                                return 0;
                            }
                        }
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "rook") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j };
                        Move move2 { i, j, i - k, j };
                        Move move3 { i, j, i, j + k };
                        Move move4 { i, j, i, j - k };
                        std::vector<Move> moves { move1, move2, move3, move4 };
                        for (Move move: moves) {
                            result = validate_move(game, game.board, move, game.turn);
                            if (result == 0) {
                                return 0;
                            }
                        }
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "queen") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j - k };
                        Move move2 { i, j, i + k, j + k };
                        Move move3 { i, j, i - k, j + k };
                        Move move4 { i, j, i - k, j - k };  
                        Move move5 { i, j, i + k, j };
                        Move move6 { i, j, i - k, j };
                        Move move7 { i, j, i, j + k };
                        Move move8 { i, j, i, j - k };  
                        std::vector<Move> moves { move1, move2, move3, move4, move5, move6, move7, move8 };
                        for (Move move: moves) {
                            result = validate_move(game, game.board, move, game.turn);
                            if (result == 0) {
                                return 0;
                            }
                        }                 
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "king") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 1 }, { i + 1, j }, 
                    { i, j + 1 }, { i + 1, j - 1 }, { i - 1, j + 1 }, 
                    { i - 1, j }, { i, j - 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second };
                        result = validate_move(game, game.board, move, game.turn);
                        if (result == 0) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return -1;
}





