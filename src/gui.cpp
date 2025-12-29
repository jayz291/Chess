#include <iostream>
#include <cmath>
#include "logic.h"
#include "gui.h"

void handle_input(Game& game, sf::RenderWindow& window, Assets& assets) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* mouse_move = event->getIf<sf::Event::MouseMoved>()) {
            game.current_mouse_pos = window.mapPixelToCoords(mouse_move->position);
        }
   
        if (const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>()) {
            //std::cout << "clicked\n";
            if (game.state == Gamestate::Intro) {
                handle_clicks_intro(game, window, assets, mouse_press->position);
            } else if (game.state == Gamestate::Playing) {
                if (game.mode == Gamemode::Twoplayer || (game.mode == Gamemode::CPUwhite && game.turn == black) ||
                    game.mode == Gamemode::CPUblack && game.turn == white) {
                    handle_clicks_playing(game, window, mouse_press->position, assets);
                    if (game.selected_square != -1) {
                        game.is_dragging = true;
                        game.current_mouse_pos = window.mapPixelToCoords(mouse_press->position);
                        game.dragged_piece = game.board[game.selected_square].piece_occupying.piece_type;
                    }
                    handle_clicks_undoing(game, mouse_press->position);
                }

            } else if (game.state == Gamestate::Promoting_pawn) {
                handle_clicks_promoting(game, mouse_press->position);
            } else if (game.state == Gamestate::Gameover) {
                game.state = Gamestate::Resetting;
            } else if (game.state == Gamestate::Resetting) {
                handle_clicks_resetting(game, mouse_press->position);
            }
            if (game.state != Gamestate::Intro) {
                handle_clicks_returning(game, mouse_press->position);
            }
        }
        if (const auto* key_event = event->getIf<sf::Event::KeyPressed>()) {
            if (game.state == Gamestate::Intro && game.typing) {
                if (key_event->system) {
                    if (key_event->code == sf::Keyboard::Key::V) {
                        std::cout << (game.fen_string + sf::Clipboard::getString()).getSize() << '\n';
                        if ((game.fen_string + sf::Clipboard::getString()).getSize() <= 105) {
                            game.fen_string.insert(game.cursor_index, sf::Clipboard::getString());
                            game.cursor_index += sf::Clipboard::getString().getSize();
                        }
                    } else if (key_event->code == sf::Keyboard::Key::C) {
                        sf::Clipboard::setString(game.fen_string);
                    }
                }
                if (key_event->code == sf::Keyboard::Key::Left) {
                    if (game.fen_string.getSize() > 0) {
                        game.cursor_index--;
                    }
                } else if (key_event->code == sf::Keyboard::Key::Right) {
                    if (game.cursor_index < game.fen_string.getSize()) {
                        game.cursor_index++;
                    }
                }
            }
        }
        if (const auto* text_event = event->getIf<sf::Event::TextEntered>()) {
            if (game.state == Gamestate::Intro && game.typing) {
                if (text_event->unicode < 32 && text_event->unicode != 8) {
                    continue;
                }
                if (text_event->unicode == 8) {
                    if (!game.fen_string.isEmpty() && game.cursor_index > 0) {
                        game.fen_string.erase(game.cursor_index - 1, 1);
                        game.cursor_index--;
                    }
                } else if (text_event->unicode < 128 && game.fen_string.getSize() <= 110) {
                    game.fen_string.insert(game.cursor_index, text_event->unicode);
                    game.cursor_index++;
                }
            }
        }
            
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
            window.setView(sf::View(visibleArea));
        }
        if (const auto* mouse_release = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (game.is_dragging) {
                sf::Vector2i mouse_pos = mouse_release->position;
                handle_drag_release(game, window, mouse_pos, assets);
            }

            game.is_dragging = false;
        }
    }
}

void render(Game& game, sf::RenderWindow& window, Assets& assets) {
    
    window.clear(sf::Color::Blue);
    sf::RectangleShape board({760, 760});
    if (game.state == Gamestate::Intro) {
        draw_intro_screen(window, game, assets);
    }
    if (game.state != Gamestate::Intro) {
        draw_board(game, window, assets);
        draw_return_to_home_button(window, assets);
    }
    if (game.state != Gamestate::Gameover && game.state != Gamestate::Resetting && game.state != Gamestate::Intro) {
        draw_undo_button(window, assets);
    }
    if (game.state == Gamestate::Gameover) {
        draw_end_screen(game, window, assets);
    } else if (game.state == Gamestate::Resetting) {
        draw_reset_button(window, assets);
    }
    if (game.state == Gamestate::Promoting_pawn) {
        draw_pawn_promotion_screen(game, window, assets);
    }
    window.display();
}

void draw_intro_screen(sf::RenderWindow& window, Game& game, Assets& assets) {
   
    sf::RectangleShape play_button = make_rectangle({330, 330}, {350, 160}, Colours::white);
 
    sf::Text text = configure_text(assets.font, "Chess", {200, 20}, 210, Colours::black);
    sf::Text text2 = configure_text(assets.font, "Play", {450, 380}, 60, Colours::black);
 
    sf::RectangleShape choice1_button = make_rectangle({110, 530}, {250, 110}, Colours::white);
    sf::RectangleShape choice2_button = make_rectangle({400, 530}, {250, 110}, Colours::white);
    sf::RectangleShape choice3_button = make_rectangle({690, 530}, {250, 110}, Colours::white);
    sf::RectangleShape text_box = make_rectangle({60, 690}, {870, 50}, Colours::white);

    sf::Text choice1_text = configure_text(assets.font, "Play CPU as\n white", {130, 540}, 30, Colours::black);
    sf::Text choice2_text = configure_text(assets.font, "Play CPU as\n black", {420, 540}, 30, Colours::black);
    sf::Text choice3_text = configure_text(assets.font, "Two player", {710, 540}, 30, Colours::black);
    sf::Text entered_fen = configure_text(assets.font, game.fen_string.toAnsiString(), {70, 700}, 15, Colours::black);
    //std::cout << game.fen_string.toAnsiString() << '\n';

    if (game.mode == Gamemode::CPUblack) {
        choice1_button.setOutlineThickness(-5.0f);
        choice1_button.setOutlineColor(sf::Color::Black);
    } else if (game.mode == Gamemode::CPUwhite) {
        choice2_button.setOutlineThickness(-5.0f);
        choice2_button.setOutlineColor(sf::Color::Black);
    } else {
        choice3_button.setOutlineThickness(-5.0f);
        choice3_button.setOutlineColor(sf::Color::Black);       
    }
    if (game.typing) {
        text_box.setOutlineThickness(-5.0f);
        text_box.setOutlineColor(sf::Color::Black);
    }
    window.draw(choice1_button);
    window.draw(choice2_button);
    window.draw(choice3_button);
    window.draw(text);
    window.draw(play_button);
    window.draw(text2);
    window.draw(choice1_text);
    window.draw(choice2_text);
    window.draw(choice3_text);
    window.draw(text_box);
    window.draw(entered_fen);
    if (!game.typing && game.fen_string.getSize() == 0) {
        sf::Text fen_instruction_text = configure_text(assets.font, "Enter FEN here for a custom position\n", 
            {70, 700}, 16, Colours::grey);
        window.draw(fen_instruction_text);
    }
    if (game.typing) {
        if (assets.cursor_clock.getElapsedTime().asSeconds() < 0.5f) {
            sf::Vector2f cursor_pos = entered_fen.findCharacterPos(game.cursor_index);
            sf::RectangleShape cursor_shape = make_rectangle({cursor_pos.x, cursor_pos.y }, {2, 15}, Colours::black);
            window.draw(cursor_shape);
        } else if (assets.cursor_clock.getElapsedTime().asSeconds() > 1.0f) {
            assets.cursor_clock.restart();
        }
    }
}

sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, Colours colour) {
    sf::Text text(font);
    text.setString(string);
    text.setPosition(pos);
    if (colour == Colours::black) {
        text.setFillColor(sf::Color::Black);
    } else if (colour == Colours::red) {
        text.setFillColor(sf::Color::Red);
    } else if (colour == Colours::grey) {
        text.setFillColor(sf::Color(211, 211, 211));
    }
    text.setCharacterSize(size);
    return text;
}

sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, Colours colour) {
    sf::RectangleShape rectangle(size);
    rectangle.setPosition(pos);
    if (colour == Colours::white) {
        rectangle.setFillColor(sf::Color::White);
    } else if (colour == Colours::black) {
        rectangle.setFillColor(sf::Color::Black);
    } else if (colour == Colours::blue) {
        rectangle.setFillColor(sf::Color::Blue);
    }
    return rectangle;
}

std::size_t get_cursor_index_from_click(const sf::Text& fen_string, float mouse_x) {
    sf::String string = fen_string.getString();
    for (std::size_t i = 0; i < string.getSize(); i++) {
        sf::Vector2f char_pos = fen_string.findCharacterPos(i);
        sf::Vector2f next_char_pos = fen_string.findCharacterPos(i + 1);
        float mid_point = char_pos.x + (next_char_pos.x - char_pos.x) / 2.f;
        if (mouse_x < mid_point) {
            return i;
        }
    }
    return string.getSize();
}

void handle_clicks_intro(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2i mouse_pos) {
    int x = mouse_pos.x;
    int y = mouse_pos.y;
    if (330 <= x && x <= 680 && 160 <= y && y <= 490) {
        game.state = Gamestate::Playing;
        game.initialise();
        handle_fen_string(game);
    }
    if (110 <= x && x <= 360 && 530 <= y && y <= 640) {
        game.view = white;
        game.mode = Gamemode::CPUblack;
    } else if (400 <= x && x <= 650 && 530 <= y && y <= 640) {
        game.view = black;
        game.mode = Gamemode::CPUwhite;
    } else if (690 <= x && x <= 940 && 530 <= y && y <= 640) {
        game.view = white;
        game.mode = Gamemode::Twoplayer;
    }
    if (100 <= x && x <= 920 && 690 <= y && y <= 740) {
        game.typing = true;
        sf::Text text_copy = configure_text(assets.font, game.fen_string.toAnsiString(), {70, 700}, 15, Colours::black);
        game.cursor_index = get_cursor_index_from_click(text_copy, (float) mouse_pos.x);
    } else {
        game.typing = false;
    }
}

void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets) {
    int result = select_square(mouse_pos.x, mouse_pos.y, game);

    if (result >= 0) {
        /*std::cout << game.current_move.prev_row << ' ' << game.current_move.prev_col << 
        ' ' << game.current_move.new_row << ' ' << game.current_move.new_col << '\n';*/
        make_game_move(game, game.bitboards, game.board, result, game.current_move);  
    }
    if (game.promoting_pawn) {
        draw_pawn_promotion_screen(game, window, assets);
        game.state = Gamestate::Promoting_pawn;
        return;
    }
    //print_all_bitboards(game.bitboards);
    if (result >= 0) {
        game.turn = ((game.turn == white) ? black : white);
        is_game_over(game);
    }
}

void handle_drag_release(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets) {
             
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    int square = 56 - 8 * row + col;
    if (game.mode == Gamemode::CPUwhite) {
        square = square ^ 56;
    }
    if (square != game.selected_square) {
        Move move { game.selected_square, square, game.turn, game.dragged_piece };
        if (game.board[square].piece_occupying.piece_type != none) {
            move.piece_taken = game.board[square].piece_occupying;
        }

        int result = validate_move(game, game.bitboards, game.board, move);
        if (result >= 0) {
            game.current_move = move;
            game.board[game.selected_square].selected = false;
            game.selected_square = -1;
            make_game_move(game, game.bitboards, game.board, result, game.current_move);  
        }
        if (game.promoting_pawn) {
            draw_pawn_promotion_screen(game, window, assets);
            game.state = Gamestate::Promoting_pawn;
            return;
        }
        //print_all_bitboards(game.bitboards);
        if (result >= 0) {
            game.turn = ((game.turn == white) ? black : white);
            is_game_over(game);
        }
    }
}

void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos) {
    bool selection_made = select_pawn_promotion(game, mouse_pos);
    if (selection_made) {
        game.state = Gamestate::Playing;
        game.turn = ((game.turn == white) ? black : white);
        is_game_over(game);
    }
    
    //print_all_bitboards(game.bitboards);
}

void handle_clicks_resetting(Game& game, sf::Vector2i mouse_pos) {
    if (10 <= mouse_pos.x && mouse_pos.x <= 54 && 10 <= mouse_pos.y && mouse_pos.y <= 45) {
        game.initialise();
        return;
    }
}

void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos) {
    if (950 <= mouse_pos.x && mouse_pos.x <= 994 && 10 <= mouse_pos.y && mouse_pos.y <= 45) {
        if (game.mode == Gamemode::Twoplayer) {
            undo_game_move(game);
        } else {
            undo_game_move(game);
            undo_game_move(game);
        }
    }
}

void handle_clicks_returning(Game& game, sf::Vector2i mouse_pos) {
    if (10 <= mouse_pos.x && mouse_pos.x <= 114 && 55 <= mouse_pos.y && mouse_pos.y <= 90) {
        game.state = Gamestate::Intro;
    }
}

void draw_reset_button(sf::RenderWindow& window, Assets& assets) {

    sf::RectangleShape reset_button = make_rectangle({10, 10}, {44, 35}, Colours::white);
    sf::Text text = configure_text(assets.font, "Reset", {13, 13}, 15, Colours::red);

    window.draw(reset_button);
    window.draw(text);
}

void draw_undo_button(sf::RenderWindow& window, Assets& assets) {

    sf::RectangleShape undo_button = make_rectangle({950, 10}, {44, 35}, Colours::white);
    sf::Text text = configure_text(assets.font, "Undo", {953, 13}, 15, Colours::red);

    window.draw(undo_button);
    window.draw(text);
}

void draw_return_to_home_button(sf::RenderWindow& window, Assets& assets) {

    sf::RectangleShape return_button = make_rectangle({10, 55}, {104, 35}, Colours::white);
    sf::Text text = configure_text(assets.font, "Back to Home", {13, 58}, 15, Colours::red);

    window.draw(return_button);
    window.draw(text);
}

void draw_board(Game& game, sf::RenderWindow& window, Assets& assets) {
    float x_offset {}, y_offset {};

    bool prev_move_available { false };
    Move prev_move { -1, -1 };
    if (game.move_record.size() > 0) {
        prev_move_available = true;
        prev_move = game.move_record.back();
        if (game.view == black) {
            prev_move.new_square = prev_move.new_square ^ 56;
            prev_move.prev_square = prev_move.prev_square ^ 56;
        }
    }
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            sf::RectangleShape cell({SQUARE_SIZE, SQUARE_SIZE});
            x_offset = 120 + j * (SQUARE_SIZE);
            y_offset = 20 + i * (SQUARE_SIZE);
            int square = 56 - 8 * i + j;
            if (game.board[square].colour == "brown") {
                if (game.view == white) {
                    if (prev_move_available && (square == prev_move.prev_square) ||
                        (square == prev_move.new_square)) {
                        cell.setFillColor(sf::Color(1, 140, 32));
                    } else {
                        cell.setFillColor(sf::Color(165, 42, 42));
                    }
                } else {
                    if (prev_move_available && (square == prev_move.prev_square) ||
                        (square == prev_move.new_square)) {
                        cell.setFillColor(sf::Color(144, 238, 144));
                    } else {
                        cell.setFillColor(sf::Color::Yellow);
                    }
                }
            } else {
                if (game.view == white) {
                    if (prev_move_available && (square == prev_move.prev_square) ||
                        (square == prev_move.new_square)) {
                        cell.setFillColor(sf::Color(144, 238, 144));
                    } else {
                        cell.setFillColor(sf::Color::Yellow);
                    }
                } else {
                    if (prev_move_available && (square == prev_move.prev_square) ||
                        (square == prev_move.new_square)) {
                        cell.setFillColor(sf::Color(1, 140, 32));
                    } else {
                        cell.setFillColor(sf::Color(165, 42, 42));
                    }
                }
            }
            if (game.board[square].selected && game.view == white) {
                //std::cout << i << " " << j << '\n';
                //std::cout << x_offset << " " << y_offset << '\n';
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);
            } else if (game.board[square ^ 56].selected && game.view == black) {
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);             
            }
            cell.setPosition({x_offset, y_offset});
            window.draw(cell);
        }
    }
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = 56 - rank * 8 + file;
            
            uint64_t mask = 1ULL << square;
            
            if (!game.is_dragging || square != game.selected_square) {
                if (game.bitboards.bitboards[white][pawn] & mask) {
                    draw_piece(game, window, assets, rank, file, pawn, white);
                } else if (game.bitboards.bitboards[black][pawn] & mask) {
                    draw_piece(game, window, assets, rank, file, pawn, black);
                } else if (game.bitboards.bitboards[white][knight] & mask) {
                    draw_piece(game, window, assets, rank, file, knight, white);
                } else if (game.bitboards.bitboards[black][knight] & mask) {
                    draw_piece(game, window, assets, rank, file, knight, black);
                } else if (game.bitboards.bitboards[white][bishop] & mask) {
                    draw_piece(game, window, assets, rank, file, bishop, white);
                } else if (game.bitboards.bitboards[black][bishop] & mask) {
                    draw_piece(game, window, assets, rank, file, bishop, black);
                } else if (game.bitboards.bitboards[white][rook] & mask) {
                    draw_piece(game, window, assets, rank, file, rook, white);
                } else if (game.bitboards.bitboards[black][rook] & mask) {
                    draw_piece(game, window, assets, rank, file, rook, black);
                } else if (game.bitboards.bitboards[white][queen] & mask) {
                    draw_piece(game, window, assets, rank, file, queen, white);
                } else if (game.bitboards.bitboards[black][queen] & mask) {
                    draw_piece(game, window, assets, rank, file, queen, black);
                } else if (game.bitboards.bitboards[white][king] & mask) {
                    draw_piece(game, window, assets, rank, file, king, white);
                } else if (game.bitboards.bitboards[black][king] & mask) {
                    draw_piece(game, window, assets, rank, file, king, black);
                }
            } else {
                continue;
            }
        }
    }
    if (game.is_dragging) {
        //std::cout << "here\n";
        draw_piece(game, window, assets, 7 - game.selected_square / 8, game.selected_square % 8, 
            game.board[game.selected_square].piece_occupying.piece_type,
            game.board[game.selected_square].piece_occupying.colour, true);
    }
}

void draw_end_screen(Game& game, sf::RenderWindow& window, Assets& assets) {
   
    sf::Text text(assets.font);
    text.setFillColor(sf::Color::Red);
    text.setCharacterSize(40);
    text.setPosition({280, 280}); 

    if (game.checkmate) {
        if (game.winner == white) {
            text.setString("CHECKMATE\nWHITE WON!\n-----------------------------\nClick anywhere to \ncontinue");
        } else {
            text.setString("CHECKMATE\nBLACK WON!\n-----------------------------\nClick anywhere to \ncontinue");
        }
    } else if (game.stalemate) {
        text.setString("Draw by stalemate\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.repetition) {
        text.setString("Draw by threefold \nrepetition\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.insufficient_material) {
        text.setString("Draw by insufficient \nmaterial\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.plys_to_100 == 100) {
        text.setString("Draw by the 50-move rule\n-----------------------------\nClick anywhere to \ncontinue");
    }

    sf::RectangleShape end_screen = make_rectangle({250, 250}, {500, 300}, Colours::black);

    window.draw(end_screen);
    window.draw(text);
}

void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window, Assets& assets) {

    sf::RectangleShape pawn_promotion_screen = make_rectangle({250, 250}, {500, 300}, Colours::blue);

    window.draw(pawn_promotion_screen);
    int rank = ((game.view == white) ? 4 : 3);
    draw_piece(game, window, assets, rank, 2, rook, game.turn);
    draw_piece(game, window, assets, rank, 3, knight, game.turn);
    draw_piece(game, window, assets, rank, 4, bishop, game.turn);
    draw_piece(game, window, assets, rank, 5, queen, game.turn);
 
    window.display();
}

void draw_piece(Game& game, sf::RenderWindow& window, Assets& assets, 
    int x, int y, int piece, int colour, bool dragging) {
    //std::string piece_type = piece->piece_type;
    sf::Texture texture = assets.array[colour][piece];

    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});

    if (!dragging) {
        float y_offset;
        float x_offset { static_cast<float>(135 + y * (SQUARE_SIZE)) };
        if (game.view == white) {
            y_offset = (30 + x * (SQUARE_SIZE));
        } else {
            y_offset = (695 - x * (SQUARE_SIZE));
        }
        sprite.setPosition({x_offset, y_offset});
    } else {
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin({bounds.size.x / 2, bounds.size.y / 2});
        sprite.setPosition(game.current_mouse_pos);
    }

    if (piece == king && colour == white) {
        if (game.white_in_check && game.turn == white) {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        } 
    } else if (piece == king && colour == black) {
        if (game.black_in_check && game.turn == black) {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        }
    }
    window.draw(sprite);

}

int select_square(int x, int y, Game& game) {
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    
    if (game.mode == Gamemode::CPUwhite) {
        row = 7 - row;
    }
    int square = 56 - 8 * row + col;
    uint64_t mask = 1ULL << square;
    
    if (row < 0 || col < 0 || row > 7 || col > 7) {
        return -2;
    }
    //std::cout << "Coords - row: " << row << " " << "col: "<< col << '\n';
    if (game.selected_square != -1) {
        
        Move move { game.selected_square, square, game.turn, 
            game.board[game.selected_square].piece_occupying.piece_type };

        if (game.board[square].piece_occupying.piece_type != none) {
            move.piece_taken = game.board[square].piece_occupying;
        }
        int result = validate_move(game, game.bitboards, game.board, move);
        game.board[game.selected_square].selected = false;
        game.selected_square = -1; 
  
        if (result >= 0) {
            game.current_move = move;
            return result;
        } 
        return -1;
    } else if (((mask & game.bitboards.white_occupied) && game.turn == white) || 
        ((mask & game.bitboards.black_occupied) && game.turn == black)) {
        game.board[square].selected = true;
        game.selected_square = 56 - 8 * row + col;
        return -1;
  
    } 
    return -3;
}

bool select_pawn_promotion(Game& game, sf::Vector2i mouse_pos) {
    int x, y;

    x = mouse_pos.x;
    y = mouse_pos.y;
    
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);

    if (row == 4 && col == 2) {
        game.piece_selected = rook;
    } else if (row == 4 && col == 3) {
        game.piece_selected = knight;
    } else if (row == 4 && col == 4) {
        game.piece_selected = bishop;
    } else if (row == 4 && col == 5) {
        game.piece_selected = queen;
    }
    if (game.piece_selected != -1) {
        handle_pawn_promotion(game, game.bitboards, game.board, game.current_move);
        return true;
    }
    return false;
}

