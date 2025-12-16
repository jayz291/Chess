#pragma once
#include <SFML/Graphics.hpp>
#include "game.h"

void render(Game& game, sf::RenderWindow& window);
void draw_intro_screen(sf::RenderWindow& window, Game& game);
void draw_board(Game& game, sf::RenderWindow& window);
void draw_piece(Game& game, sf::RenderWindow& window, std::shared_ptr<Piece>& piece);
void draw_reset_button(sf::RenderWindow& window);
void draw_undo_button(sf::RenderWindow& window);
void draw_end_screen(Game& game, sf::RenderWindow& window);
void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window);
void draw_return_to_home_button(sf::RenderWindow& window);

sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, const std::string& colour);
sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, const std::string& colour);

void handle_input(Game& game, sf::RenderWindow& window);
void handle_clicks_intro(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos);
void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos);
void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_resetting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_returning(Game& game, sf::Vector2i mouse_pos);

int select_square(int x, int y, Game& game);
bool select_pawn_promotion(Game& game, sf::Vector2i mouse_pos);







