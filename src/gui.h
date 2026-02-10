#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "game.h"

sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color colour);
sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, sf::Color colour);

class Button {
    public:
    Button(const sf::Font& font, const std::string& string, sf::Vector2f text_pos, 
    int text_size, sf::Color text_colour, sf::Vector2f rec_pos, sf::Vector2f rec_size, sf::Color rec_colour) :
        text(font), rectangle(rec_size), default_colour(rec_colour) {
        text = configure_text(font, string, text_pos, text_size, text_colour);
        rectangle = make_rectangle(rec_pos, rec_size, rec_colour);
    }
    void set_rec_colour(sf::Color new_rec_colour) {
        rectangle.setFillColor(new_rec_colour);
    }
    void set_new_default_rec_colour(sf::Color new_rec_colour) {
        default_colour = new_rec_colour;
    }
    void set_text_colour(sf::Color new_text_colour) {
        text.setFillColor(new_text_colour);
    }
    void set_text(std::string str) {
        text.setString(str);
    }
    bool is_clicked(sf::Vector2i mouse_pos) {
        return rectangle.getGlobalBounds().contains({static_cast<float>(mouse_pos.x), 
        static_cast<float>(mouse_pos.y)});
    }
    void set_outline_thickness(float thickness, sf::Color colour) {
        rectangle.setOutlineThickness(thickness);
        rectangle.setOutlineColor(colour); 
    }
    void set_default_thickness() {
        rectangle.setOutlineThickness(0.0f);
    }
    void draw(sf::RenderWindow& window) {
        window.draw(rectangle);
        window.draw(text);
    }
    void update(sf::RenderWindow& window, sf::Vector2i mouse_pos) {
        if (is_clicked(mouse_pos)) {
            rectangle.setFillColor(sf::Color(255, 127, 0));
        } else {
            rectangle.setFillColor(default_colour);
        }
        draw(window);
    }
    private:
    sf::Text text;
    sf::RectangleShape rectangle;
    sf::Color default_colour;
};

using Screen = Button;

class TextBox {
    public:
    TextBox(const sf::Font& font, sf::Vector2f rec_pos, sf::Vector2f rec_size, 
    sf::Vector2f text_pos, int text_size) : 
    text(font), cursor_index(0), is_focused(false), fen_instruction_text(font), 
    error_text(font) {
        box = make_rectangle(rec_pos, rec_size, sf::Color::White);
        box.setOutlineColor(sf::Color::Black);
        text.setFillColor(sf::Color::Black);
        text.setPosition(text_pos);
        text.setCharacterSize(text_size);
    
        fen_instruction_text = configure_text(font, 
            "Enter FEN here for a custom position\n", text_pos, text_size, sf::Color(142, 142, 142));
        error_text = configure_text(font, "Invalid fen string\n", {70, 750}, 16, sf::Color::Red);
        cursor.setSize({2, 15});
        cursor.setFillColor(sf::Color::Black);
    };  
    void set_string(const std::string& str) {
        content = str;
        text.setString(str);
        cursor_index = content.getSize();
    }
    bool handle_event(const sf::Event& event, const sf::RenderWindow& window) {
        if (const auto* mouse_press = event.getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f mouse_pos = window.mapPixelToCoords(mouse_press->position);
            
            if (box.getGlobalBounds().contains(mouse_pos)) {
                is_focused = true;
                box.setOutlineThickness(5.0f); 
                cursor_index = get_cursor_index_from_click(mouse_pos.x);
            } else {
                is_focused = false;
                box.setOutlineThickness(0.0f);
            }
            return false;
        }
        if (!is_focused) {
            return false;
        }
        if (const auto* key_event = event.getIf<sf::Event::KeyPressed>()) {
            if (key_event->system) {
                if (key_event->code == sf::Keyboard::Key::V) {
                    sf::String raw_text = sf::Clipboard::getString();
                    sf::String filtered_text = "";
                    for (auto& letter: raw_text) {
                        if (letter != 10 && letter != 13) {
                            filtered_text += letter;
                        }
                    }
                    //std::cout << (game.entered_fen + sf::Clipboard::getString()).getSize() << '\n';
                    if ((content + filtered_text).getSize() <= 105) {
                        content.insert(cursor_index, filtered_text);
                        cursor_index += filtered_text.getSize();
                    }
                } else if (key_event->code == sf::Keyboard::Key::C) {
                    sf::Clipboard::setString(content);
                }
            }
            if (key_event->code == sf::Keyboard::Key::Left) {
                if (content.getSize() > 0) {
                    cursor_index--;
                }
            } else if (key_event->code == sf::Keyboard::Key::Right) {
                if (cursor_index < content.getSize()) {
                    cursor_index++;
                }
            }
            text.setString(content);
            return true;
        }
        if (const auto* text_event = event.getIf<sf::Event::TextEntered>()) {
            if (text_event->unicode == 8) {
                if (!content.isEmpty() && cursor_index > 0) {
                    content.erase(cursor_index - 1, 1);
                    cursor_index--;
                }
            } else if (text_event->unicode >= 32 && text_event->unicode < 128 && 
                content.getSize() <= 110) {
                content.insert(cursor_index, text_event->unicode);
                cursor_index++;
            } 
            text.setString(content);
            return true; 
        }
        return false;
    }
    void draw(Game& game, sf::RenderWindow& window) {
        window.draw(box);
        if (!is_focused && content.getSize() == 0) {
            window.draw(fen_instruction_text);
        } else {
            window.draw(text);
            if (is_focused) {
                if (cursor_clock.getElapsedTime().asSeconds() < 0.5f) {
                    sf::Vector2f cursor_pos = text.findCharacterPos(cursor_index);
                    cursor.setPosition({cursor_pos.x, cursor_pos.y});
                    window.draw(cursor);
                } else if (cursor_clock.getElapsedTime().asSeconds() > 1.0f) {
                    cursor_clock.restart();
                }
            }
        }
        if (game.ui.invalid_fen_position) {
            window.draw(error_text);
        }
    }
    std::size_t get_cursor_index_from_click(float mouse_x) {
        sf::String string = content;
        for (std::size_t i = 0; i < content.getSize(); i++) {
            sf::Vector2f char_pos = text.findCharacterPos(i);
            sf::Vector2f next_char_pos = text.findCharacterPos(i + 1);
            float mid_point = char_pos.x + (next_char_pos.x - char_pos.x) / 2.f;
            if (mouse_x < mid_point) {
                return i;
            }
        }
        return string.getSize();
    }
    std::string get_string() {
        return content;
    }

    private:
    sf::RectangleShape box;
    sf::Text fen_instruction_text;
    sf::Text error_text;
    sf::Text text;
    sf::RectangleShape cursor;
    sf::String content;
    std::size_t cursor_index;
    bool is_focused;
    sf::Clock cursor_clock;
};

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
    Assets() {
        if (!array[BLACK_PAWN].loadFromFile("./assets/images/Chess_pdt45.png")) {
            return;
        }
        if (!array[WHITE_PAWN].loadFromFile("./assets/images/Chess_plt45.png")) {
            return;
        }
        if (!array[BLACK_KNIGHT].loadFromFile("./assets/images/Chess_ndt45.png")) {
            return;
        }
        if (!array[WHITE_KNIGHT].loadFromFile("./assets/images/Chess_nlt45.png")) {
            return;
        }
        if (!array[BLACK_BISHOP].loadFromFile("./assets/images/Chess_bdt45.png")) {
            return;
        }
        if (!array[WHITE_BISHOP].loadFromFile("./assets/images/Chess_blt45.png")) {
            return;
        }
        if (!array[BLACK_ROOK].loadFromFile("./assets/images/Chess_rdt45.png")) {
            return;
        }
        if (!array[WHITE_ROOK].loadFromFile("./assets/images/Chess_rlt45.png")) {
            return;
        }
        if (!array[BLACK_QUEEN].loadFromFile("./assets/images/Chess_qdt45.png")) {
            return;
        }
        if (!array[WHITE_QUEEN].loadFromFile("./assets/images/Chess_qlt45.png")) {
            return;
        }
        if (!array[BLACK_KING].loadFromFile("./assets/images/Chess_kdt45.png")) {
            return;
        }
        if (!array[WHITE_KING].loadFromFile("./assets/images/Chess_klt45.png")) {
            return;
        }
        if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
            return;
        }
        if (!font2.openFromFile("./assets/fonts/Roboto-Regular.ttf")) {
            return;
        }
        if (!buffer.loadFromFile("./assets/sounds/piece-placement.wav")) {
            return;
        }
        if (!buffer2.loadFromFile("./assets/sounds/capture2.wav")) {
            return;
        }
        sound.emplace(buffer);
        sound2.emplace(buffer2);
    }
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





