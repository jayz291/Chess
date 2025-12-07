#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

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
};



class Game {
    public:
        //std::vector<Piece> pieces{32}; 
        std::array<std::array<Cell, 8>, 8> board {};
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
        //board[0][0].piece_occupying = std::make_unique<Piece> ("rook", "black", false, 0, 0);
        //board[0][1].piece_occupying = std::make_unique<Piece> ("knight", "black", false, 0, 1);
    }
};

void draw_board(Game& game, sf::RenderWindow& window);
void draw_piece(Game& game, sf::RenderWindow& window, std::unique_ptr<Piece>& piece);

int main() {
    Game game {};
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    sf::RectangleShape board({760, 760});
   
    while (window.isOpen()) {
        
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
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
        texture.loadFromFile("./src/Chess_pdt45.png");
    } else if (piece->piece_type == "pawn" && piece->colour == "white") {
        texture.loadFromFile("./src/Chess_plt45.png");
    }
    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});
    float x_offset { static_cast<float>(135 + piece->col * (760 / 8)) };
    float y_offset { static_cast<float>(30 + piece->row * (760 / 8)) };
  
    sprite.setPosition({x_offset, y_offset});
    window.draw(sprite);

}



