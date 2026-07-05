#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "game.h"

sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color colour);
sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, sf::Color colour);

class Button {
    public:
    Button(const sf::Font& font, const std::string& string, sf::Vector2f text_pos, 
    int text_size, sf::Color text_colour, sf::Vector2f rec_pos, sf::Vector2f rec_size, sf::Color rec_colour);

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
    void update(sf::RenderWindow& window, sf::Vector2i mouse_pos);

    private:
    sf::Text text;
    sf::RectangleShape rectangle;
    sf::Color default_colour;
};

using Screen = Button;

class TextBox {
    public:
    TextBox(const sf::Font& font, sf::Vector2f rec_pos, sf::Vector2f rec_size, 
    sf::Vector2f text_pos, int text_size);
    
    void set_string(const std::string& str);

    bool handle_event(const sf::Event& event, const sf::RenderWindow& window);

    void draw(Game& game, sf::RenderWindow& window);

    std::size_t get_cursor_index_from_click(float mouse_x);

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