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
    std::unique_ptr<Piece> piece_occupying {};
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
        board[0][0].piece_occupying = std::make_unique<Piece> ("rook", "black", false, 0, 0);
        board[0][1].piece_occupying = std::make_unique<Piece> ("knight", "black", false, 0, 1);
    }
};

int main() {
    Game game {};
    sf::Window window(sf::VideoMode({1000, 800}), "Chess");
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
    }

}



