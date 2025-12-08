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
    bool captured { false };
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

class Game {
    public:
        //std::vector<Piece> pieces{32}; 
        std::array<std::array<Cell, 8>, 8> board {};
        Coords selected {};
        std::string turn { "white" };
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
void select_square(int x, int y, Game& game);

int validate_move(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col, std::string& turn);
int validate_move_pawn(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col, std::string& turn);
int validate_move_knight(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col);
int validate_move_bishop(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col);
int validate_move_rook(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col);
int validate_move_queen(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col);
int validate_move_king(Game &game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col, std::string& turn);
int check_checks(Game &game, std::array<std::array<Cell, 8>, 8>& copy, int& new_row, int& new_col);

int main() {
    Game game {};
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    sf::RectangleShape board({760, 760});
   
    while (window.isOpen()) {
        
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>()) {
                select_square(mouse_press->position.x, mouse_press->position.y, game);
            }

        }
        window.clear(sf::Color::Red);
        draw_board(game, window);
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
  
    sprite.setPosition({x_offset, y_offset});
    window.draw(sprite);

}

void select_square(int x, int y, Game& game) {
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
        if (validate_move(game, game.board, game.selected.row, game.selected.col, row, col, game.turn) == 0) {

            game.board[row][col].piece_occupying = nullptr;
            game.board[row][col].piece_occupying = std::move(game.board[game.selected.row][game.selected.col].piece_occupying);
            if (game.board[row][col].piece_occupying == nullptr) {
                std::cout << "failed\n";
                return;
            }
            game.board[row][col].piece_occupying->row = row;
            game.board[row][col].piece_occupying->col = col;
            if (game.turn == "white") {
                game.turn = "black";
            } else {
                game.turn = "white";
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

int validate_move(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, 
    int& new_row, int& new_col, std::string& turn) {
    if (board[prev_row][prev_col].piece_occupying != nullptr 
        && (prev_row != new_row || prev_row != new_col) && 
        (!board[new_row][new_col].piece_occupying || board[new_row][new_col].piece_occupying->colour != turn)) {
        
        
        std::string piece = board[prev_row][prev_col].piece_occupying->piece_type;
        if (piece == "pawn") {
            return validate_move_pawn(game, board, prev_row, prev_col, new_row, new_col, turn);
        } else if (piece == "knight") {
            return validate_move_knight(game, board, prev_row, prev_col, new_row, new_col);
        } else if (piece == "bishop") {
            return validate_move_bishop(game, board, prev_row, prev_col, new_row, new_col);
        } else if (piece == "rook") {
            return validate_move_rook(game, board, prev_row, prev_col, new_row, new_col);
        } else if (piece == "queen") {
            return validate_move_queen(game, board, prev_row, prev_col, new_row, new_col);
        } else if (piece == "king") {
            return validate_move_king(game, board, prev_row, prev_col, new_row, new_col, turn);
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

int validate_move_pawn(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, 
    int& new_row, int& new_col, std::string& turn) {
    std::vector<std::pair<int, int>> no_capture_moves {};
    std::vector<std::pair<int, int>> capture_moves {};
    std::pair<int, int> move { new_row, new_col };
    std::vector<std::pair<int, int>>::iterator it {};
    int pawn_row { prev_row };
    int pawn_col { prev_col };

    if (turn == "white") {
        if (pawn_row == 6 && !board[pawn_row - 1][pawn_col].piece_occupying) {
            no_capture_moves = { { pawn_row - 1, pawn_col }, { pawn_row - 2, pawn_col } };
        } else {
            no_capture_moves = { { pawn_row - 1, pawn_col } };
        }
        capture_moves = { { pawn_row - 1, pawn_col - 1 }, { pawn_row - 1, pawn_col + 1 } };

    } else {
        if (pawn_row == 1 && !board[pawn_row + 1][pawn_col].piece_occupying) {
            no_capture_moves = { { pawn_row + 1, pawn_col }, { pawn_row + 2, pawn_col } };
        } else {
            no_capture_moves = { { pawn_row + 1, pawn_col } };
        }
        capture_moves = { { pawn_row + 1, pawn_col - 1 }, { pawn_row + 1, pawn_col + 1 } };
    }
    std::cout << "here\n";
    if (board[new_row][new_col].piece_occupying) {
        std::cout << "here2\n";
        it = std::find(capture_moves.begin(), capture_moves.end(), move);
        if (it != capture_moves.end()) {
            return 0;
        } else {
            return -1;
        }
    } else {
        it = std::find(no_capture_moves.begin(), no_capture_moves.end(), move);
        if (it != no_capture_moves.end()) {
            return 0;
        } else {
            return -1;
        }
    }
    
}

int validate_move_knight(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col) {
    std::pair<int, int> move { new_row, new_col };
    
    int knight_row { prev_row };
    int knight_col { prev_col };
    std::vector<std::pair<int, int>> moves { { knight_row + 1, knight_col + 2 }, { knight_row + 2, knight_col + 1}, 
    { knight_row - 1, knight_col - 2 }, { knight_row - 2, knight_col - 1 }, { knight_row + 1, knight_col - 2 }, 
    { knight_row - 1, knight_col + 2 }, { knight_row - 2, knight_col + 1 }, { knight_row + 2, knight_col - 1 } };
    auto it = std::find(moves.begin(), moves.end(), move);
    if (it != moves.end()) {
        return 0;
    } else {
        return -1;
    }
}

int validate_move_bishop(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col) {
    int bishop_row { prev_row };
    int bishop_col { prev_col };
    int row_change { new_row - bishop_row };
    int col_change { new_col - bishop_col };
    if (std::abs(row_change) == std::abs(col_change)) {
        if (row_change > 0 && col_change > 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[bishop_row + i][bishop_col + i].piece_occupying) {
                    return -1;
                }
            }
        } else if (row_change < 0 && col_change > 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[bishop_row - i][bishop_col + i].piece_occupying) {
                    return -1;
                }
            }           
        } else if (row_change > 0 && col_change < 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[bishop_row + i][bishop_col - i].piece_occupying) {
                    return -1;
                }
            }           
        } else if (row_change < 0 && col_change < 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[bishop_row - i][bishop_col - i].piece_occupying) {
                    return -1;
                }
            }              
        }
        return 0;
    }
    return -1;
}

int validate_move_rook(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col) {
    int rook_row { prev_row };
    int rook_col { prev_col };
    int row_change { new_row - rook_row };
    int col_change { new_col - rook_col };
    if (row_change == 0) {
        if (col_change > 0) {
            for (int i { 1 }; i < std::abs(col_change); i++) {
                if (board[rook_row][rook_col + i].piece_occupying) {
                    return -1;
                }
            } 
        } else if (col_change < 0) {
            for (int i { 1 }; i < std::abs(col_change); i++) {
                if (board[rook_row][rook_col - i].piece_occupying) {
                    return -1;
                }
            }         
        }
        return 0;
    } else if (col_change == 0) {
        if (row_change > 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[rook_row + i][rook_col].piece_occupying) {
                    return -1;
                }
            } 
        } else if (row_change < 0) {
            for (int i { 1 }; i < std::abs(row_change); i++) {
                if (board[rook_row - i][rook_col].piece_occupying) {
                    return -1;
                }
            }         
        }
        return 0;
    }
    return -1;
}

int validate_move_queen(Game& game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, int& new_row, int& new_col) {
    int rook_move = validate_move_rook(game, board, prev_row, prev_col, new_row, new_col);
    int bishop_move = validate_move_bishop(game, board, prev_row, prev_col, new_row, new_col);
    if (rook_move == 0 || bishop_move == 0) {
        return 0;
    } else {
        return -1;
    }
}

int validate_move_king(Game &game, std::array<std::array<Cell, 8>, 8>& board, int& prev_row, int& prev_col, 
    int& new_row, int& new_col, std::string& turn) {
    
    int king_row { prev_row };
    int king_col { prev_col };
    int row_change { new_row - king_row };
    int col_change { new_col - king_col };
    std::vector<std::pair<int, int>> moves { { new_row + 1, new_col }, { new_row - 1, new_col }, 
    { new_row + 1, new_col + 1 }, { new_row - 1, new_col - 1 }, 
    { new_row - 1, new_col + 1 }, { new_row + 1, new_col - 1 } };

    if (sqrt(pow(row_change, 2) + pow(col_change, 2)) <= sqrt(2)) {
        for (auto coord: moves) {
            if (board[coord.first][coord.second].piece_occupying && 
                board[coord.first][coord.second].piece_occupying->piece_type == "king" && 
                board[coord.first][coord.second].piece_occupying->colour != turn) {
                return -1;
            }
        }
        std::array<std::array<Cell, 8>, 8> copy {};
        if (check_checks(game, copy, new_row, new_col) == 0) {
            return 0;
        } 
        return -1;
    }
    return -1;
}

int check_checks(Game &game, std::array<std::array<Cell, 8>, 8>& copy, int& new_row, int& new_col) {
    int test {};
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            copy[i][j].piece_occupying = game.board[i][j].piece_occupying;
        }
    }
    copy[new_row][new_col].piece_occupying = nullptr;
    copy[new_row][new_col].piece_occupying = std::move(copy[game.selected.row][game.selected.col].piece_occupying);
    if (game.turn == "white") {
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == "black") {
                    std::string turn {"black"};
                    test = validate_move(game, copy, i, j, new_row, new_col, turn);
                    if (test == 0) {
                        return 1;
                    }
                }
            }
        }
        return 0;
    } else {
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == "white") {
                    std::string turn {"white"};
                    test = validate_move(game, copy, i, j, new_row, new_col, turn);
                    if (test == 0) {
                        return 1;
                    }
                }
            }
        }
        return 0;       
    }
}





