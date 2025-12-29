#pragma once
#include <SFML/Graphics.hpp>
#include "game.h"

struct Assets {
    sf::Font font;
    sf::Texture array[2][6];
    sf::Clock cursor_clock;
    Assets() {
        if (!array[black][pawn].loadFromFile("./assets/images/Chess_pdt45.png")) {
            return;
        }
        if (!array[white][pawn].loadFromFile("./assets/images/Chess_plt45.png")) {
            return;
        }
        if (!array[black][knight].loadFromFile("./assets/images/Chess_ndt45.png")) {
            return;
        }
        if (!array[white][knight].loadFromFile("./assets/images/Chess_nlt45.png")) {
            return;
        }
        if (!array[black][bishop].loadFromFile("./assets/images/Chess_bdt45.png")) {
            return;
        }
        if (!array[white][bishop].loadFromFile("./assets/images/Chess_blt45.png")) {
            return;
        }
        if (!array[black][rook].loadFromFile("./assets/images/Chess_rdt45.png")) {
            return;
        }
        if (!array[white][rook].loadFromFile("./assets/images/Chess_rlt45.png")) {
            return;
        }
        if (!array[black][queen].loadFromFile("./assets/images/Chess_qdt45.png")) {
            return;
        }
        if (!array[white][queen].loadFromFile("./assets/images/Chess_qlt45.png")) {
            return;
        }
        if (!array[black][king].loadFromFile("./assets/images/Chess_kdt45.png")) {
            return;
        }
        if (!array[white][king].loadFromFile("./assets/images/Chess_klt45.png")) {
            return;
        }
        if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
        }
    }
};

void render(Game& game, sf::RenderWindow& window, Assets& assets);
void draw_intro_screen(sf::RenderWindow& window, Game& game, Assets& assets);
void draw_board(Game& game, sf::RenderWindow& window, Assets& assets);
void draw_piece(Game& game, sf::RenderWindow& window, Assets& assets, int x, int y, int piece, 
    int colour, bool dragging = false);
void draw_reset_button(sf::RenderWindow& window, Assets& assets);
void draw_undo_button(sf::RenderWindow& window, Assets& assets);
void draw_end_screen(Game& game, sf::RenderWindow& window, Assets& assets);
void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window, Assets& assets);
void draw_return_to_home_button(sf::RenderWindow& window, Assets& assets);

sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, sf::Color colour);
sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color colour);

void handle_input(Game& game, sf::RenderWindow& window, Assets& assets);
void handle_clicks_intro(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2i mouse_pos);
void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);
void handle_drag_release(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);
void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_resetting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_returning(Game& game, sf::Vector2i mouse_pos);

int select_square(int x, int y, Game& game);
bool select_pawn_promotion(Game& game, sf::Vector2i mouse_pos);







