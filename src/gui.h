#pragma once
#include "components.h"

struct Assets {
    sf::Font font;
    sf::Font font2;
    sf::SoundBuffer buffer;
    sf::SoundBuffer buffer2;
    std::optional<sf::Sound> sound;
    std::optional<sf::Sound> sound2;
    sf::Texture array[16];
    sf::Vector2f current_mouse_pos;
    bool allow_takebacks { true };
    bool sound_on { false };
    
    Assets();

    Button play_black_cpu {font, "Play CPU as\n WHITE", {155, 540}, 30, sf::Color::Black, 
        {135, 530}, {250, 110}, sf::Color::White};
    Button play_white_cpu {font, "Play CPU as\n BLACK", {445, 540}, 30, sf::Color::Black, 
        {425, 530}, {250, 110}, sf::Color::White};
    Button play_two_player {font, "Two player", {735, 540}, 30, sf::Color::Black, 
        {715, 530}, {250, 110}, sf::Color::White};
    Button play_button {font, "Play", {500, 380}, 60, sf::Color::Black, 
        {380, 330}, {350, 160}, sf::Color::White};
    Button flip_view_button {font, "Flip view", {13, 148}, 15, sf::Color::Red, 
        {10, 145}, {74, 35}, sf::Color::White};
    Button home_button {font, "Back to Home", {13, 58}, 15, sf::Color::Red, 
        {10, 55}, {104, 35}, sf::Color::White};
    Button undo_button {font, "Undo", {13, 103}, 15, sf::Color::Red, 
        {10, 100}, {44, 35}, sf::Color::White};
    Button reset_button {font, "Reset", {13, 103}, 15, sf::Color::Red, 
        {10, 100}, {44, 35}, sf::Color::White};
    Button toggle_takebacks {font, "Allow Takebacks: Yes", {853, 753}, 15, sf::Color::Black, 
        {850, 750}, {157, 35}, sf::Color::Green};
    Button make_pgn_file {font, "Make PGN file", {13, 728}, 15, sf::Color::Red, {10, 725}, {104, 35}, sf::Color::White};
    Button toggle_audio {font, "Sound: Off", {13, 13}, 15, sf::Color::Black, {10, 10}, {104, 35}, sf::Color::Red};
    Button go_back_button {font, "<-", {920, 730}, 40, sf::Color::Black, {900, 730}, {85, 50}, sf::Color::White};
    Button go_forward_button {font, "->", {1015, 730}, 40, sf::Color::Black, {995, 730}, {85, 50}, sf::Color::White};
    TextBox fen_input {font, {60, 690}, {930, 50}, {70, 700}, 15};
    Screen pawn_promotion_screen {font, "Choose Promotion Piece", {280, 280}, 40, sf::Color::White, 
        {250, 250}, {500, 300}, sf::Color::Blue};
    Screen end_screen {font, "", {280, 280}, 40, sf::Color::Red, {250, 250}, {500, 300}, sf::Color::Black};
};

void render(Game& game, sf::RenderWindow& window, Assets& assets);
void draw_intro_screen(sf::RenderWindow& window, Game& game, Assets& assets, sf::Vector2i& mouse_pos);
void draw_board(Game& game, sf::RenderWindow& window, Assets& assets);
void draw_piece(Position& position, UI& ui, sf::RenderWindow& window, Assets& assets, 
    int x, int y, uint8_t piece, bool dragging = false);
void draw_end_screen(Position& position, Result& result, sf::RenderWindow& window, Assets& assets);
void draw_pawn_promotion_screen(Position& position, UI& ui, sf::RenderWindow& window, Assets& assets);
void draw_move_history_panel(Log& log, sf::RenderWindow& window, Assets& assets);

void handle_input(Game& game, sf::RenderWindow& window, Assets& assets);
void delegate_click_event(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2f& world_pos);
void handle_clicks_intro(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2i mouse_pos);
void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);
void handle_drag_release(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);
void handle_clicks_promoting(Game& game, Assets& assets, sf::Vector2i mouse_pos);
void handle_clicks_resetting(Game& game, Assets& assets, sf::Vector2i mouse_pos);
void handle_clicks_undoing(Game& game, Assets& assets, sf::Vector2i mouse_pos);
void handle_clicks_returning(Game& game, Assets& assets, sf::Vector2i mouse_pos);
void handle_clicks_flip_view(Game& game, Assets& assets, sf::Vector2i mouse_pos);

int select_square(int x, int y, Position& position, UI& ui); 
bool select_promotion_piece(Game& game, sf::Vector2i mouse_pos);

void play_sound(Assets& assets, Game& game);





