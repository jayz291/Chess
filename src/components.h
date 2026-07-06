#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "game.h"

sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color colour);
sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, sf::Color colour);

class Button {
    public:

    /**
     * @brief initialises all the attributes of a Button
     * @param font the font of the button text
     * @param string the button text content
     * @param text_pos the position of the text
     * @param text_size the text size
     * @param text_colour the text colour
     * @param rec_pos the position of the button rectangle
     * @param rec_size the size of the rectangle
     * @param rec_colour the colour of the rectangle
     */
    Button(const sf::Font& font, const std::string& string, sf::Vector2f text_pos, 
    int text_size, sf::Color text_colour, sf::Vector2f rec_pos, sf::Vector2f rec_size, sf::Color rec_colour);

    /**
     * @brief changes the rectangle colour 
     * @param new_rec_colour the replacement colour
     */
    void set_rec_colour(sf::Color new_rec_colour) {
        rectangle.setFillColor(new_rec_colour);
    }

    /**
     * @brief sets a new default rectangle colour
     * @param new_rec_colour the replacement colour
     */
    void set_new_default_rec_colour(sf::Color new_rec_colour) {
        default_colour = new_rec_colour;
    }

    /**
     * @brief changes the text colour
     * @param new_text_colour the replacement colour
     */
    void set_text_colour(sf::Color new_text_colour) {
        text.setFillColor(new_text_colour);
    }

    /**
     * @brief changes the text content 
     * @param str the replacement text
     */
    void set_text(std::string str) {
        text.setString(str);
    }

    /**
     * @brief checks whether the button is being clicked
     * @param mouse_pos the position of the cursor
     * @return true if the button is being clicked and false otherwise
     */
    bool is_clicked(sf::Vector2i mouse_pos) {
        return rectangle.getGlobalBounds().contains({static_cast<float>(mouse_pos.x), 
        static_cast<float>(mouse_pos.y)});
    }

    /**
     * @brief sets the style of the button border
     * @param thickness the thickness of the border
     * @param colour the colour of the border
     */
    void set_outline_thickness(float thickness, sf::Color colour) {
        rectangle.setOutlineThickness(thickness);
        rectangle.setOutlineColor(colour); 
    }

    /**
     * @brief sets the default border thickness of the button
     */
    void set_default_thickness() {
        rectangle.setOutlineThickness(0.0f);
    }

    /**
     * @brief draws the button the screen
     * @param window serves as target for 2D drawing
     */
    void draw(sf::RenderWindow& window) {
        window.draw(rectangle);
        window.draw(text);
    }

    /**
     * @brief changes the button if it is being clicked/hovered on
     * @param window serves as target for 2D drawing
     * @param mouse_pos the position of the cursor
     */
    void update(sf::RenderWindow& window, sf::Vector2i mouse_pos);

    private:
    sf::Text text;
    sf::RectangleShape rectangle;
    sf::Color default_colour;
};

using Screen = Button;

class TextBox {
    public:

    /**
     * @brief initialises an instance of a textbox with the necessary attributes 
     * @param font the font to be used in the input textbox
     * @param rec_pos the position of the textbox
     * @param rec_size the size of the textbox
     * @param text_pos the position of the text in the textbox
     * @param text_size the size of the text in the textbox
     */
    TextBox(const sf::Font& font, sf::Vector2f rec_pos, sf::Vector2f rec_size, 
    sf::Vector2f text_pos, int text_size);
    
    /**
     * @brief sets the text for the textbox
     * @param str the new text
     */
    void set_string(const std::string& str);

    /**
     * @brief allows for user interaction with the textbox
     * @param event defines a type of event
     * @param window represents the currently drawn screen
     * @return true if the text in the textbox has been changed and false otherwise 
     */
    bool handle_event(const sf::Event& event, const sf::RenderWindow& window);

    /**
     * @brief draws the textbox 
     * @param game class containing all game variables/classes
     * @param window serves as target for 2D drawing
     */
    void draw(Game& game, sf::RenderWindow& window);

    /**
     * @brief finds the position of the cursor in the letters based on a click
     * @param mouse_x the x-position of the cursor 
     * @return the number of characters of the string 
     */
    std::size_t get_cursor_index_from_click(float mouse_x);

    /**
     * @brief return the player input from the textbox
     * @return the text content of the textbox
     */
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
    bool is_focused; ///< true if the user has selected the textbox and false otherwise
    sf::Clock cursor_clock;
};