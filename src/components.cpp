#include "components.h"

sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color colour) {
    sf::RectangleShape rectangle(size);
    rectangle.setPosition(pos);
    rectangle.setFillColor(colour);
    return rectangle;
}

sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, sf::Color colour) {
    sf::Text text(font);
    text.setString(string);
    text.setPosition(pos);
    text.setFillColor(colour);
    text.setCharacterSize(size);
    return text;
}

Button::Button(const sf::Font& font, const std::string& string, sf::Vector2f text_pos, 
    int text_size, sf::Color text_colour, sf::Vector2f rec_pos, sf::Vector2f rec_size, sf::Color rec_colour) :  
    text(font), rectangle(rec_size), default_colour(rec_colour) {
    text = configure_text(font, string, text_pos, text_size, text_colour);
    rectangle = make_rectangle(rec_pos, rec_size, rec_colour);
}

TextBox::TextBox(const sf::Font& font, sf::Vector2f rec_pos, sf::Vector2f rec_size, 
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

void Button::update(sf::RenderWindow& window, sf::Vector2i mouse_pos) {
    if (is_clicked(mouse_pos)) {
        rectangle.setFillColor(sf::Color(255, 127, 0));
    } else {
        rectangle.setFillColor(default_colour);
    }
    draw(window);
}

void TextBox::set_string(const std::string& str) {
    content = str;
    text.setString(str);
    cursor_index = content.getSize();
}

bool TextBox::handle_event(const sf::Event& event, const sf::RenderWindow& window) {
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

void TextBox::draw(Game& game, sf::RenderWindow& window) {
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

std::size_t TextBox::get_cursor_index_from_click(float mouse_x) {
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



