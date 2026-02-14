#include <iostream>
#include <cmath>
#include "logic.h"
#include "gui.h"
#include "perft.h"
#include <atomic>

extern std::atomic<bool> thinking_in_progress;

void handle_input(Game& game, sf::RenderWindow& window, Assets& assets) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* mouse_move = event->getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f world_pos = window.mapPixelToCoords(mouse_move->position);
            sf::Vector2f game_pos = {world_pos.x, world_pos.y};
            assets.current_mouse_pos = game_pos;
        }
        if (const auto* scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            if (scrolled->wheel == sf::Mouse::Wheel::Vertical) {
                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
                if (mouse_pos.x >= 900 && mouse_pos.x <= 1080 && mouse_pos.y >= 120 && mouse_pos.y <= 770) {
                    int total_lines = (game.log.notation_history.size() + 1) / 2;
                    int max_scroll = std::max(0, total_lines - 26);
                    if (scrolled->delta > 0) {
                        if (game.log.history_scroll_offset > 0) {
                            game.log.history_scroll_offset--;
                        }
                    } else if (scrolled->delta < 0) {
                        if (game.log.history_scroll_offset < max_scroll) {
                            game.log.history_scroll_offset++;
                        }
                    }
                }
            }
        }
        if (game.state == Gamestate::Intro) {
            bool text_changed = assets.fen_input.handle_event(*event, window);
            if (text_changed) {
                game.entered_fen = assets.fen_input.get_string();
            }
        }
   
        if (const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f world_pos = window.mapPixelToCoords(mouse_press->position);
            delegate_click_event(game, window, assets, world_pos);
        }
            
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
            sf::View view(visibleArea); 
            view.setCenter({550, 400});
            window.setView(view);
        }
        if (const auto* mouse_release = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (game.ui.is_dragging) {
                sf::Vector2f world_pos = window.mapPixelToCoords(mouse_release->position);
                sf::Vector2i game_pos = { (int)world_pos.x, (int)world_pos.y};
                handle_drag_release(game, window, game_pos, assets);
            }

            game.ui.is_dragging = false;
        }
    }
}

void delegate_click_event(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2f& world_pos) {
    sf::Vector2i game_pos = {(int)world_pos.x, (int)world_pos.y};
    auto& position = game.position;
    auto& log = game.log;
    auto& ui = game.ui;
    if (assets.toggle_audio.is_clicked(game_pos)) {
        assets.sound_on = (assets.sound_on == false) ? true : false;
        if (assets.sound_on) {
            assets.toggle_audio.set_text("Sound: On");
            assets.toggle_audio.set_new_default_rec_colour(sf::Color::Green);
        } else {
            assets.toggle_audio.set_text("Sound: Off");
            assets.toggle_audio.set_new_default_rec_colour(sf::Color::Red);
        }
    }

    if (game.state == Gamestate::Intro) {
        handle_clicks_intro(game, window, assets, game_pos);
    } else if (game.state == Gamestate::Playing) {
        if (game.mode == Gamemode::Twoplayer || (game.mode == Gamemode::CPUwhite && position.turn == BLACK) ||
            game.mode == Gamemode::CPUblack && position.turn == WHITE) {
            handle_clicks_playing(game, window, game_pos, assets);
            if (ui.selected_square != -1) {
                ui.is_dragging = true;
                assets.current_mouse_pos = world_pos;
                ui.dragged_piece = position.board[ui.selected_square];
            }
            handle_clicks_undoing(game, assets, game_pos);
        }
    } else if (game.state == Gamestate::Promoting_pawn) {
        handle_clicks_promoting(game, assets, game_pos);
    } else if (game.state == Gamestate::Gameover) {
        game.state = Gamestate::Resetting;
    } else if (game.state == Gamestate::Resetting) {
        handle_clicks_resetting(game, assets, game_pos);
        if (assets.go_back_button.is_clicked({game_pos})) {
            if (log.current_ply_num > 0) {
                game.undo_game_move(true);
            }
        }
        if (assets.make_pgn_file.is_clicked({game_pos})) {
            game.create_pgn();
        }
        if (assets.go_forward_button.is_clicked({game_pos})) {
            if (log.current_ply_num < position.move_record.size()) {
                Move chosen_move = position.move_record[log.current_ply_num];
                int result = position.validate_move(chosen_move);
                game.make_game_move(result, chosen_move, true);
                if (ui.promoting_pawn) {
                    game.handle_pawn_promotion(chosen_move, true, true);
                }
                game.position.evaluate_king_checks();
            }
        }
    }
    if (game.state != Gamestate::Intro) {
        handle_clicks_returning(game, assets, game_pos);
    }
    if (game.mode == Gamemode::Twoplayer && (game.state == Gamestate::Playing || 
        game.state == Gamestate::Resetting)) {
        handle_clicks_flip_view(game, assets, game_pos);
    }
}

void render(Game& game, sf::RenderWindow& window, Assets& assets) {
    
    window.clear(sf::Color::Blue);
    sf::RectangleShape board({760, 760});
    sf::Color rectangle_colour = thinking_in_progress ? sf::Color(128, 128, 128) : sf::Color::White;
    sf::Color text_colour = thinking_in_progress ? sf::Color(59, 59, 59) : sf::Color::Red;
    sf::Vector2i mouse_pos = {static_cast<int>(assets.current_mouse_pos.x), 
                static_cast<int>(assets.current_mouse_pos.y)};
    assets.toggle_audio.update(window, mouse_pos);
    if (game.state == Gamestate::Intro) {
        draw_intro_screen(window, game, assets, mouse_pos);
    }
    if (game.state != Gamestate::Intro) {
        draw_board(game, window, assets);
        draw_move_history_panel(game.log, window, assets);
        assets.home_button.set_rec_colour(rectangle_colour);
        assets.home_button.set_text_colour(text_colour);
        if (thinking_in_progress) {
            assets.home_button.draw(window);
        } else {
            assets.home_button.update(window, mouse_pos);
        }
    }
    if (game.state != Gamestate::Gameover && game.state != Gamestate::Resetting && game.state != Gamestate::Intro) {
        if (assets.allow_takebacks) {
            assets.undo_button.set_rec_colour(rectangle_colour);
            assets.undo_button.set_text_colour(text_colour);
        } else {
            assets.undo_button.set_rec_colour(sf::Color(128, 128, 128));
            assets.undo_button.set_text_colour(sf::Color(59, 59, 59));
        }
        if (thinking_in_progress || !assets.allow_takebacks) {
            assets.undo_button.draw(window);
        } else {
            assets.undo_button.update(window, mouse_pos);
        }
    }
    if (game.state == Gamestate::Gameover) {
        draw_end_screen(game.position, game.result, window, assets);
    } else if (game.state == Gamestate::Resetting) {
        assets.reset_button.update(window, mouse_pos);
        assets.go_back_button.update(window, mouse_pos);
        assets.go_forward_button.update(window, mouse_pos);
        assets.make_pgn_file.update(window, mouse_pos);
    }
    if (game.state == Gamestate::Promoting_pawn) {
        draw_pawn_promotion_screen(game.position, game.ui, window, assets);
    }
    if (game.mode == Gamemode::Twoplayer && game.state != Gamestate::Intro) {
        assets.flip_view_button.update(window, mouse_pos);
    }
    window.display();
}

void play_sound(Assets& assets, Game& game) {
    if (!assets.sound_on) {
        return;
    }
    Move& prev_move = game.position.move_record.back();
    if (prev_move.get_captured_piece() == EMPTY_SQUARE) {
        assets.sound->play(); 
    } else {
        assets.sound2->setVolume(50);
        assets.sound2->play();
    }
}

void draw_intro_screen(sf::RenderWindow& window, Game& game, Assets& assets, sf::Vector2i& mouse_pos) {
 
    sf::Text title = configure_text(assets.font, "Chess", {270, 20}, 210, sf::Color::Black);

    if (game.mode == Gamemode::CPUblack) {
        assets.play_black_cpu.set_outline_thickness(-5.0f, sf::Color::Black);
        assets.play_white_cpu.set_default_thickness();
        assets.play_two_player.set_default_thickness();
    } else if (game.mode == Gamemode::CPUwhite) {
        assets.play_white_cpu.set_outline_thickness(-5.0f, sf::Color::Black);
        assets.play_two_player.set_default_thickness();
        assets.play_black_cpu.set_default_thickness();
    } else {
        assets.play_two_player.set_outline_thickness(-5.0f, sf::Color::Black);   
        assets.play_white_cpu.set_default_thickness();
        assets.play_black_cpu.set_default_thickness();  
    }
    assets.play_black_cpu.update(window, mouse_pos);
    assets.play_white_cpu.update(window, mouse_pos);
    assets.play_two_player.update(window, mouse_pos);
    assets.play_button.update(window, mouse_pos);
    window.draw(title);
    assets.fen_input.draw(game, window);
    assets.toggle_takebacks.update(window, mouse_pos);
}

void draw_move_history_panel(Log& log, sf::RenderWindow& window, Assets& assets) {
    sf::Text move_record_title = configure_text(assets.font, "Move Record", {935, 20}, 20, sf::Color::White);
    window.draw(move_record_title);
    sf::Vector2f panel_pos = {900.f, 58.f};
    sf::Vector2f panel_size = {180.f, 660.f};
    int line_height = 25;
    int max_lines_visible = panel_size.y / line_height;

    sf::RectangleShape background = make_rectangle(panel_pos, panel_size, sf::Color::White);
    background.setOutlineThickness(2);
    window.draw(background);

    int total_pairs = (log.notation_history.size() + 1) / 2;
    sf::Text text_white(assets.font2, "", 18);
    sf::Text text_black(assets.font2, "", 18);
    sf::Text number(assets.font2, "", 18);
    int start_index = log.history_scroll_offset;
    int end_index = std::min(total_pairs, start_index + max_lines_visible);

    for (int i = start_index; i < end_index; ++i) {
        float x_pos = std::floor(panel_pos.x + 15);
        float y_pos = std::floor(panel_pos.y + 5 + (i - start_index) * line_height);
        std::string line_str = std::to_string(i + log.move_num) + ".";
        std::string white_turn_txt;
        std::string black_turn_txt;
        number.setString(line_str);
        number.setPosition({x_pos - 10, y_pos});
        number.setFillColor(sf::Color::Black);
        window.draw(number);
        int first_offset = number.getLocalBounds().size.x;
        int second_offset;
        int ply_offset = (log.first_move_filler) ? 1 : 0;
        if (i * 2 < log.notation_history.size()) {
            if (log.current_ply_num - 1 + ply_offset == i * 2) {
                text_white.setFillColor(sf::Color::Red);
            } else {
                text_white.setFillColor(sf::Color::Black);
            }
            white_turn_txt = log.notation_history[i * 2];
            text_white.setPosition({x_pos + first_offset, y_pos});
            text_white.setString(white_turn_txt);
            second_offset = text_white.getLocalBounds().size.x;
            window.draw(text_white);
        } 
        if (i * 2 + 1 < log.notation_history.size()) {
            if (log.current_ply_num - 1 + ply_offset == i * 2 + 1) {
                text_black.setFillColor(sf::Color::Red);
            } else {
                text_black.setFillColor(sf::Color::Black);
            }
            black_turn_txt = log.notation_history[i * 2 + 1];
            text_black.setPosition({x_pos + 30 + second_offset + first_offset, y_pos});
            text_black.setString(black_turn_txt);
            window.draw(text_black);
        }
    }
    if (total_pairs > max_lines_visible) {
        float scroll_ratio = static_cast<float> (log.history_scroll_offset) / (total_pairs - max_lines_visible);
        float bar_height = 40.f;
        float bar_y_range = panel_size.y - bar_height;
        sf::RectangleShape scroll_bar = make_rectangle({panel_pos.x + panel_size.x - 5, 
            panel_pos.y + (bar_y_range * scroll_ratio)}, {5.f, bar_height}, sf::Color(100, 100, 100));
        window.draw(scroll_bar);
    }
}

// set the font, content, position, size and colour of a text string
sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, sf::Color colour) {
    sf::Text text(font);
    text.setString(string);
    text.setPosition(pos);
    text.setFillColor(colour);
    text.setCharacterSize(size);
    return text;
}

// set the position, size and colour of a rectangle
sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color colour) {
    sf::RectangleShape rectangle(size);
    rectangle.setPosition(pos);
    rectangle.setFillColor(colour);
    return rectangle;
}

void handle_clicks_intro(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2i mouse_pos) {
    int x = mouse_pos.x;
    int y = mouse_pos.y;
    if (assets.play_button.is_clicked({x, y})) {
        game.initialise();
        int result = game.handle_fen_string();
        if (result == 0) {
            //std::cout << std::bitset<64>(game.zobrist_hash) << '\n';
            game.ui.invalid_fen_position = false;
            game.state = Gamestate::Playing;
            //run_perft_suite(game, 5);
            game.is_game_over();
            verify_board_sync(game.position);
            verify_zobrist_sync(game.position);
        } else {
            game.ui.invalid_fen_position = true;
        }
    }
    if (assets.play_black_cpu.is_clicked(mouse_pos)) {
        game.ui.view = WHITE;
        game.mode = Gamemode::CPUblack;
    } else if (assets.play_white_cpu.is_clicked(mouse_pos)) {
        game.ui.view = BLACK;
        game.mode = Gamemode::CPUwhite;
    } else if (assets.play_two_player.is_clicked(mouse_pos)) {
        game.ui.view = WHITE;
        game.mode = Gamemode::Twoplayer;
    } else if (assets.toggle_takebacks.is_clicked(mouse_pos)) {
        assets.allow_takebacks = (assets.allow_takebacks == true) ? false : true;
        if (assets.allow_takebacks) {
            assets.toggle_takebacks.set_text("Allow takebacks: Yes");
            assets.toggle_takebacks.set_new_default_rec_colour(sf::Color::Green);
        } else {
            assets.toggle_takebacks.set_text("Allow takebacks: No");
            assets.toggle_takebacks.set_new_default_rec_colour(sf::Color::Red);
        }
    }
}

void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets) {
    int result = select_square(mouse_pos.x, mouse_pos.y, game.position, game.ui);
    if (result >= 0) {
        game.make_game_move(result, game.position.current_move);  
    }
    if (game.ui.promoting_pawn) {
        game.state = Gamestate::Promoting_pawn;
        return;
    }
    if (result >= 0) {
        verify_board_sync(game.position);
        verify_zobrist_sync(game.position);
        play_sound(assets, game);
        game.is_game_over();
    }
}

void handle_drag_release(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets) {
             
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    int square = 56 - 8 * row + col;
    if (game.ui.view == BLACK) {
        square = square ^ 63;
    }
    if (square != game.ui.selected_square) {
        Move move;
        move.set_from_square(game.ui.selected_square);
        move.set_to_square(square);
        move.set_piece(game.ui.dragged_piece);
        if (game.position.board[square] != EMPTY_SQUARE) {
            move.set_captured(game.position.board[square]);
        }

        int result = game.position.validate_move(move);
        if (result >= 0) {
            game.position.current_move = move;
            game.ui.selected_square = -1;
            game.make_game_move(result, game.position.current_move);  
        }
        if (game.ui.promoting_pawn) {
            game.state = Gamestate::Promoting_pawn;
            return;
        }
        //print_all_bitboards(game.bitboards);
        if (result >= 0) {
            game.is_game_over();
            play_sound(assets, game);
        }
    }
}

void handle_clicks_promoting(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    bool selection_made = select_promotion_piece(game, mouse_pos);
    if (selection_made) {
        game.state = Gamestate::Playing;
        play_sound(assets, game);
        game.is_game_over();
        //std::cout << std::bitset<64>(game.zobrist_hash) << '\n';
    }
}

void handle_clicks_resetting(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    if (assets.reset_button.is_clicked(mouse_pos)) {
        std::string fen_string = game.entered_fen;
        game.initialise();
        game.final_fen = fen_string;
        game.handle_fen_string();
 
        game.state = Gamestate::Playing;
        game.is_game_over();
        return;
    }
}

void handle_clicks_undoing(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    if (assets.undo_button.is_clicked(mouse_pos) && assets.allow_takebacks) {
        if (game.mode == Gamemode::Twoplayer) {
            game.undo_game_move();
        } else {
            game.undo_game_move();
            game.undo_game_move();
        }
        verify_board_sync(game.position);
        verify_zobrist_sync(game.position);
    }
}

void handle_clicks_returning(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    if (thinking_in_progress) {
        return; 
    }
    if (assets.home_button.is_clicked(mouse_pos)) {
        game.state = Gamestate::Intro;
    }
}

void handle_clicks_flip_view(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    if (assets.flip_view_button.is_clicked({mouse_pos})) {
        game.ui.view = (game.ui.view == WHITE) ? BLACK : WHITE;
    }
}

void draw_board(Game& game, sf::RenderWindow& window, Assets& assets) {
    float x_offset {}, y_offset {};

    bool prev_move_available { false };
    Move prev_move;
    int to_square, from_square;
    if (game.position.move_record.size() > 0 && game.log.current_ply_num != 0) {
        prev_move = game.position.move_record[game.log.current_ply_num - 1];
        //std::cout << game.move_record[game.current_ply_num - 2] << '\n';
        to_square = prev_move.get_to_square();
        from_square = prev_move.get_from_square();
        if (to_square != from_square) {
            prev_move_available = true;
        }
        if (game.ui.view == BLACK) {
            to_square ^= 63;
            from_square ^= 63;
        } 
    }
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            sf::RectangleShape cell({SQUARE_SIZE, SQUARE_SIZE});
            x_offset = 120 + j * (SQUARE_SIZE);
            y_offset = 20 + i * (SQUARE_SIZE);
            int square = 56 - 8 * i + j;
            if (((i + j) & 1) != 0) {
                if (prev_move_available && (square == from_square ||
                    square == to_square)) {
                        cell.setFillColor(sf::Color(1, 140, 32));
                    } else {
                        cell.setFillColor(sf::Color(165, 42, 42));
                    }
                } else {
                    if (prev_move_available && (square == from_square ||
                        square == to_square)) {
                        cell.setFillColor(sf::Color(144, 238, 144));
                    } else {
                        cell.setFillColor(sf::Color::Yellow);
                }
            } 
            if ((square == game.ui.selected_square && game.ui.view == WHITE) ||
                ((square ^ 63) == game.ui.selected_square && game.ui.view == BLACK)) {
                //std::cout << i << " " << j << '\n';
                //std::cout << x_offset << " " << y_offset << '\n';
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
            
            if (!game.ui.is_dragging || square != game.ui.selected_square) {
                if (game.position.board[square] != EMPTY_SQUARE) {
                    draw_piece(game.position, game.ui, window, assets, rank, file, game.position.board[square]);
                }
            } else {
                continue;
            }
        }
    }
    if (game.ui.is_dragging) {
        draw_piece(game.position, game.ui, window, assets, 7 - game.ui.selected_square / 8, game.ui.selected_square % 8, 
            game.position.board[game.ui.selected_square], true);
    } 
}

void draw_end_screen(Position& position, Result& result, sf::RenderWindow& window, Assets& assets) {

    if (result.status & (1UL << 3)) {
        if (result.winner == WHITE) {
            assets.end_screen.set_text("CHECKMATE\nWHITE WON!\n-----------------------------\nClick anywhere to \ncontinue");
        } else {
            assets.end_screen.set_text("CHECKMATE\nBLACK WON!\n-----------------------------\nClick anywhere to \ncontinue");
        }
    } else if (result.status & (1UL << 2)) {
        assets.end_screen.set_text("Draw by stalemate\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (result.status & (1UL << 1)) {
        assets.end_screen.set_text("Draw by threefold \nrepetition\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (result.status & (1UL)) {
        assets.end_screen.set_text("Draw by insufficient \nmaterial\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (position.plys_to_100 == 100) {
        assets.end_screen.set_text("Draw by the 50-move rule\n-----------------------------\nClick anywhere to \ncontinue");
    }
    assets.end_screen.draw(window);
}

void draw_pawn_promotion_screen(Position& position, UI& ui, sf::RenderWindow& window, Assets& assets) {

    sf::Color colour = (position.turn == WHITE) ? sf::Color::White : sf::Color::Black;
    assets.pawn_promotion_screen.set_text_colour(colour);
    assets.pawn_promotion_screen.draw(window);
    int rank = ((ui.view == WHITE) ? 4 : 3);
    if (ui.view == WHITE) {
        draw_piece(position, ui, window, assets, 4, 2, piece_array[position.turn][ROOK]);
        draw_piece(position, ui, window, assets, 4, 3, piece_array[position.turn][KNIGHT]);
        draw_piece(position, ui, window, assets, 4, 4, piece_array[position.turn][BISHOP]);
        draw_piece(position, ui, window, assets, 4, 5, piece_array[position.turn][QUEEN]); 
    } else {
        draw_piece(position, ui, window, assets, 3, 5, piece_array[position.turn][ROOK]);
        draw_piece(position, ui, window, assets, 3, 4, piece_array[position.turn][KNIGHT]);
        draw_piece(position, ui, window, assets, 3, 3, piece_array[position.turn][BISHOP]);
        draw_piece(position, ui, window, assets, 3, 2, piece_array[position.turn][QUEEN]);
    }
}

void draw_piece(Position& position, UI& ui, sf::RenderWindow& window, Assets& assets, 
    int x, int y, uint8_t piece, bool dragging) {
    //assert(piece >= 0 && piece <= 5);
    sf::Sprite sprite(assets.array[piece]);
    sprite.setScale({0.1f, 0.1f});

    if (!dragging) {
        float y_offset;
        float x_offset { static_cast<float>(135 + y * (SQUARE_SIZE)) };
        if (ui.view == WHITE) {
            y_offset = (30 + x * (SQUARE_SIZE));
            x_offset = 135 + y * (SQUARE_SIZE);
        } else {
            y_offset = (695 - x * (SQUARE_SIZE));
            x_offset = 135 + (7 - y) * SQUARE_SIZE;
        }
        sprite.setPosition({x_offset, y_offset});
    } else {
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin({bounds.size.x / 2, bounds.size.y / 2});
        sprite.setPosition(assets.current_mouse_pos);
    }

    if (piece == WHITE_KING && position.white_in_check && position.turn == WHITE) {
        sprite.setColor(sf::Color(255, 0, 0, 100));
    } else if (piece == BLACK_KING && position.black_in_check && position.turn == BLACK) {
        sprite.setColor(sf::Color(255, 0, 0, 100));
    }
    window.draw(sprite);
}

int select_square(int x, int y, Position& position, UI& ui) {
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    
    if (ui.view == BLACK) {
        row = 7 - row;
        col = 7 - col;
    }
    int square = 56 - 8 * row + col;
    uint64_t mask = 1ULL << square;
    
    if (row < 0 || col < 0 || row > 7 || col > 7) {
        return -2;
    }
    //std::cout << "Coords - row: " << row << " " << "col: "<< col << '\n';
    if (ui.selected_square != -1) {
        
        Move move;
        move.set_from_square(ui.selected_square);
        move.set_to_square(square);
        move.set_piece(position.board[ui.selected_square]);
        if (position.board[square] != EMPTY_SQUARE) {
            move.set_captured(position.board[square]);
        }
        int result = position.validate_move(move);
        ui.selected_square = -1; 
  
        if (result >= 0) {
            position.current_move = move;
            return result;
        } 
        return -1;
    } else if (((mask & position.bitboards.occupied_tables[WHITE]) && position.turn == WHITE) || 
        ((mask & position.bitboards.occupied_tables[BLACK]) && position.turn == BLACK)) {
        ui.selected_square = 56 - 8 * row + col;
        return -1;
  
    } 
    return -3;
}

bool select_promotion_piece(Game& game, sf::Vector2i mouse_pos) {
    
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);

    if (row == 4 && col == 2) {
        game.ui.piece_selected = P_ROOK;
    } else if (row == 4 && col == 3) {
        game.ui.piece_selected = P_KNIGHT;
    } else if (row == 4 && col == 4) {
        game.ui.piece_selected = P_BISHOP;
    } else if (row == 4 && col == 5) {
        game.ui.piece_selected = P_QUEEN;
    }
    if (game.ui.piece_selected != -1) {
        game.handle_pawn_promotion(game.position.current_move);
        return true;
    }
    return false;
}

