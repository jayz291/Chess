#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <utility>


struct Piece {
    std::string piece_type {};
    std::string colour {};
    bool captured { false };
    int row;
    int col; 
};

struct Cell {
    std::string colour {};
    std::unique_ptr<Piece> piece_occupying { nullptr };
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
            board[1][i].piece_occupying = std::make_unique<Piece> ("pawn", "black", false, 1, i);
        }
        for (int i { 0 }; i < 8; i++) {
            board[6][i].piece_occupying = std::make_unique<Piece> ("pawn", "white", false, 6, i);
        }
        board[0][0].piece_occupying = std::make_unique<Piece> ("rook", "black", false, 0, 0);
        board[0][1].piece_occupying = std::make_unique<Piece> ("knight", "black", false, 0, 1);
        board[0][2].piece_occupying = std::make_unique<Piece> ("bishop", "black", false, 0, 2);
        board[0][3].piece_occupying = std::make_unique<Piece> ("queen", "black", false, 0, 3);
        board[0][4].piece_occupying = std::make_unique<Piece> ("king", "black", false, 0, 4);
        board[0][5].piece_occupying = std::make_unique<Piece> ("bishop", "black", false, 0, 5);
        board[0][6].piece_occupying = std::make_unique<Piece> ("knight", "black", false, 0, 6);
        board[0][7].piece_occupying = std::make_unique<Piece> ("rook", "black", false, 0, 7);
        board[7][0].piece_occupying = std::make_unique<Piece> ("rook", "white", false, 7, 0);
        board[7][1].piece_occupying = std::make_unique<Piece> ("knight", "white", false, 7, 1);
        board[7][2].piece_occupying = std::make_unique<Piece> ("bishop", "white", false, 7, 2);
        board[7][3].piece_occupying = std::make_unique<Piece> ("queen", "white", false, 7, 3);
        board[7][4].piece_occupying = std::make_unique<Piece> ("king", "white", false, 7, 4);
        board[7][5].piece_occupying = std::make_unique<Piece> ("bishop", "white", false, 7, 5);
        board[7][6].piece_occupying = std::make_unique<Piece> ("knight", "white", false, 7, 6);
        board[7][7].piece_occupying = std::make_unique<Piece> ("rook", "white", false, 7, 7);
    }
};

void draw_board(Game& game, sf::RenderWindow& window);
void draw_piece(Game& game, sf::RenderWindow& window, std::unique_ptr<Piece>& piece);
void select_square(int x, int y, Game& game);

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
            x_offset = 120 + i * (760 / 8);
            y_offset = 20 + j * (760 / 8);
            cell.setPosition({x_offset, y_offset});
            if (game.board[i][j].colour == "brown") {
                cell.setFillColor(sf::Color(165, 42, 42));
            } else {
                cell.setFillColor(sf::Color::Yellow);
            }
            if (game.board[i][j].selected) {
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);
            }
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

void draw_piece(Game& game, sf::RenderWindow& window, std::unique_ptr<Piece>& piece) {
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
    if (game.selected.row > -1 && game.selected.col > -1) {
        std::cout << game.selected.row << ' ' << game.selected.col << '\n';
        if (game.board[row][col].piece_occupying) {
            game.board[game.selected.row][game.selected.col].selected = false;
            game.selected.row = game.selected.col = -1;
            return;
        } else if (game.board[game.selected.row][game.selected.col].piece_occupying != nullptr) {

            game.board[row][col].piece_occupying = std::move(game.board[game.selected.row][game.selected.col].piece_occupying);
            if (game.board[row][col].piece_occupying == nullptr) {
                std::cout << "failed\n";
                return;
            }
            game.board[row][col].piece_occupying->row = row;
            game.board[row][col].piece_occupying->col = col;
            game.board[game.selected.col][game.selected.row].selected = false;
            game.selected.row = game.selected.col = -1;
            return;
        }
    } else {
        game.board[col][row].selected = true;
        game.selected.row = row;
        game.selected.col = col;
    }
}





