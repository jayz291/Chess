#include <iostream>
#include <cmath>
#include "logic.h"
#include "gui.h"
#include "perft.h"
#include <atomic>

extern std::atomic<bool> thinking_in_progress;

Assets::Assets() {

    for (const auto& [piece, file_path] : piece_images) {
        if (!array[piece].loadFromFile(file_path)) {
            return;
        }
    }
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
    if (!font2.openFromFile("./assets/fonts/Roboto-Regular.ttf")) {
        return;
    }
    if (!move_sound_buffer.loadFromFile("./assets/sounds/piece-placement.wav")) {
        return;
    }
    if (!capture_sound_buffer.loadFromFile("./assets/sounds/capture2.wav")) {
        return;
    }
    move_sound.emplace(move_sound_buffer);
    capture_sound.emplace(capture_sound_buffer);
}

void Application::handle_input() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* mouse_move = event->getIf<sf::Event::MouseMoved>()) {
            world_pos = window.mapPixelToCoords(mouse_move->position);
            mouse_pos = {(int)world_pos.x, (int)world_pos.y};
            //sf::Vector2f game_pos = {world_pos.x, world_pos.y};
            assets.current_mouse_pos = world_pos;
        }
        if (const auto* scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            if (scrolled->wheel == sf::Mouse::Wheel::Vertical) {
                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
                if (mouse_pos.x >= 900 && mouse_pos.x <= 1080 && mouse_pos.y >= 120 && mouse_pos.y <= 770) {
                    int total_lines = (log.notation_history.size() + 1) / 2;
                    int max_scroll = std::max(0, total_lines - 26);
                    if (scrolled->delta > 0) {
                        if (log.history_scroll_offset > 0) {
                            log.history_scroll_offset--;
                        }
                    } else if (scrolled->delta < 0) {
                        if (log.history_scroll_offset < max_scroll) {
                            log.history_scroll_offset++;
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
            //sf::Vector2f world_pos = window.mapPixelToCoords(mouse_press->position);
            world_pos = window.mapPixelToCoords(mouse_press->position);
            mouse_pos = {(int)world_pos.x, (int)world_pos.y};
            delegate_click_event();
        }
            
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
            sf::View view(visibleArea); 
            view.setCenter({550, 400});
            window.setView(view);
        }
        if (const auto* mouse_release = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (ui.is_dragging) {
                //sf::Vector2f world_pos = window.mapPixelToCoords(mouse_release->position);
                //sf::Vector2i game_pos = { (int)world_pos.x, (int)world_pos.y};
                world_pos = window.mapPixelToCoords(mouse_release->position);
                mouse_pos = { (int)world_pos.x, (int)world_pos.y};
                if (!game.premove) {
                    handle_move_square_selection();
                } else {
                    handle_premove_square_selection();
                }  
            }

            ui.is_dragging = false;
        }
    }
}

void Application::delegate_click_event() {
    //sf::Vector2i game_pos = {(int)world_pos.x, (int)world_pos.y};
 
    if (assets.toggle_audio.is_clicked(mouse_pos)) {
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
        handle_clicks_intro();
    } else if (game.state == Gamestate::Playing) {
        if (game.mode == Gamemode::Twoplayer || !is_computer_turn()) {
            game.premove = false;
            handle_move_square_selection();
            set_piece_dragging();
            handle_clicks_undoing();
        } else if (is_computer_turn()) {
            game.premove = true;
            handle_premove_square_selection();
            set_piece_dragging();
        }
        handle_clicks_resigning();
    } else if (game.state == Gamestate::Promoting_pawn) {
        handle_clicks_promoting();
    } else if (game.state == Gamestate::Promoting_pawn_premove) {
        handle_clicks_promoting(true);
    } else if (game.state == Gamestate::Gameover) {
        game.state = Gamestate::Resetting;
    } else if (game.state == Gamestate::Resetting) {
        handle_clicks_resetting();
        if (assets.make_pgn_file.is_clicked({mouse_pos})) {
            game.create_pgn();
        }
        handle_clicks_navigate_forward();
        handle_clicks_navigate_backward();
    }
    if (game.state != Gamestate::Intro) {
        handle_clicks_returning();
    }
    if (game.mode == Gamemode::Twoplayer && (game.state == Gamestate::Playing || 
        game.state == Gamestate::Resetting)) {
        handle_clicks_flip_view();
    }
}

void Application::render() {
    
    window.clear(sf::Color::Blue);
    sf::RectangleShape board({760, 760});
    sf::Color rectangle_colour = thinking_in_progress ? sf::Color(128, 128, 128) : sf::Color::White;
    sf::Color text_colour = thinking_in_progress ? sf::Color(59, 59, 59) : sf::Color::Red;
    
    assets.toggle_audio.update(window, mouse_pos);
    if (game.state == Gamestate::Intro) {
        draw_intro_screen();
    }
    if (game.state != Gamestate::Intro) {
        draw_board();
        draw_move_history_panel();
        assets.home_button.update(window, mouse_pos);
        if (game.time_control != Timesetting::Untimed) {
            if (game.state == Gamestate::Playing || game.state == Gamestate::Promoting_pawn ||
                game.state == Gamestate::Promoting_pawn_premove) {
                game.update_time();
                assets.white_clock.set_text(convert_time_to_display(game.white_time));
                assets.black_clock.set_text(convert_time_to_display(game.black_time));
            }
            if (position.turn == WHITE) {
                assets.black_clock.set_rec_colour(sf::Color(128, 128, 128));
                assets.white_clock.set_rec_colour(sf::Color::White);
            } else {
                assets.black_clock.set_rec_colour(sf::Color::White);
                assets.white_clock.set_rec_colour(sf::Color(128, 128, 128));
            }
            assets.black_clock.draw(window);
            assets.white_clock.draw(window);
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
        assets.resign_button.update(window, mouse_pos);
    }
    if (game.state == Gamestate::Gameover) {
        draw_end_screen();
    } else if (game.state == Gamestate::Resetting) {
        assets.reset_button.update(window, mouse_pos);
        assets.go_back_button.update(window, mouse_pos);
        assets.go_forward_button.update(window, mouse_pos);
        assets.make_pgn_file.update(window, mouse_pos);
    }
    if (game.state == Gamestate::Promoting_pawn) {
        draw_pawn_promotion_screen();
    } else if (game.state == Gamestate::Promoting_pawn_premove) {
        draw_pawn_promotion_screen(true);
    }
    if (game.mode == Gamemode::Twoplayer && game.state != Gamestate::Intro) {
        assets.flip_view_button.update(window, mouse_pos);
    }
    window.display();
}

std::string Application::convert_time_to_display(double time) {
    return std::format("{:02}:{:02}:{:02}", (int) time / 60, (int) time % 60, 
    (int)((time - (int)time) * 100));
}

void Application::play_sound(Move& move) {
    if (!assets.sound_on) {
        return;
    }
    if (move.get_captured_piece() == EMPTY_SQUARE && move.get_move_type() != EN_PASSANT) {
        assets.move_sound->play(); 
    } else {
        assets.capture_sound->setVolume(50);
        assets.capture_sound->play();
    }
}

void Application::draw_intro_screen() {
 
    sf::Text title = configure_text(assets.font, "Chess", {270, 20}, 210, sf::Color::Black);
  
    draw_game_mode_buttons();
    draw_time_control_buttons();

    assets.play_button.update(window, mouse_pos);
    window.draw(title);
    assets.fen_input.draw(game, window);
    assets.toggle_takebacks.update(window, mouse_pos);
}

void Application::draw_game_mode_buttons() {
    for (auto& button : assets.gamemode_buttons) {
        button->set_default_thickness();
    }
    assets.gamemode_buttons[static_cast<int>(game.mode)]->set_outline_thickness(-5.0f, sf::Color::Black);

    for (auto& button : assets.gamemode_buttons) {
        button->update(window, mouse_pos);
    }
}

void Application::draw_time_control_buttons() {
    for (auto& choice : assets.time_control_choices) {
        choice->set_default_thickness();
    }
    assets.time_control_choices[static_cast<int>(game.time_control) + 1]->set_outline_thickness(-3.0f, sf::Color::Black);

    for (auto& choice : assets.time_control_choices) {
        choice->update(window, mouse_pos);
    }
}

void Application::draw_move_history_panel() {

    sf::Text move_record_title = configure_text(assets.font, "Move Record", {935, 20}, 20, sf::Color::White);
    window.draw(move_record_title);

    sf::RectangleShape background = make_rectangle(log.panel_pos, log.panel_size, sf::Color::White);
    background.setOutlineThickness(2);
    window.draw(background);

    log.total_pairs = (log.notation_history.size() + 1) / 2;
    sf::Text text_white(assets.font2, "", 18);
    sf::Text text_black(assets.font2, "", 18);
    sf::Text number(assets.font2, "", 18);
    int start_index = log.history_scroll_offset;
    int end_index = std::min(log.total_pairs, start_index + log.max_lines_visible);

    for (int i = start_index; i < end_index; ++i) {
        float x_pos = std::floor(log.panel_pos.x + 15);
        float y_pos = std::floor(log.panel_pos.y + 5 + (i - start_index) * log.line_height);
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
    if (log.total_pairs > log.max_lines_visible) {
        draw_scroll_bar();
    }
}

inline void Application::draw_scroll_bar() {
    float scroll_ratio = static_cast<float> (log.history_scroll_offset) / (log.total_pairs - log.max_lines_visible);
    float bar_height = 40.f;
    float bar_y_range = log.panel_size.y - bar_height;
    sf::RectangleShape scroll_bar = make_rectangle({log.panel_pos.x + log.panel_size.x - 5, 
        log.panel_pos.y + (bar_y_range * scroll_ratio)}, {5.f, bar_height}, sf::Color(100, 100, 100));
    window.draw(scroll_bar);
}

void Application::handle_clicks_intro() {
    int x = mouse_pos.x;
    int y = mouse_pos.y;
    if (assets.play_button.is_clicked({x, y})) {
        game.initialise();
        int result = game.handle_fen_string();
        if (result == VALID) {
            ui.invalid_fen_position = false;
            game.state = Gamestate::Playing;
            //run_perft_suite(game, 5);
            game.is_game_over();
            verify_board_sync(position);
            verify_zobrist_sync(position);
        } else {
            ui.invalid_fen_position = true;
        }
    }
    if (assets.play_black_cpu.is_clicked(mouse_pos)) {
        ui.view = WHITE;
        game.mode = Gamemode::CPUblack;
        set_clock_positions(ui.view);
    } else if (assets.play_white_cpu.is_clicked(mouse_pos)) {
        ui.view = BLACK;
        game.mode = Gamemode::CPUwhite;
        set_clock_positions(ui.view);
    } else if (assets.play_two_player.is_clicked(mouse_pos)) {
        ui.view = WHITE;
        game.mode = Gamemode::Twoplayer;
        set_clock_positions(ui.view);
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
    handle_time_control_selection();
}

void Application::handle_time_control_selection() {
    for (int i = 0; i < assets.time_control_choices.size(); i++) {
        if (assets.time_control_choices[i]->is_clicked(mouse_pos)) {
            game.time_control = static_cast<Timesetting>(i - 1);
        }
    }
}

void Application::handle_move_square_selection() {
    int result = select_square();
    process_move(result);
}

void Application::handle_premove_square_selection() {

    int square = find_square_selected();
    if (square == OUT_OF_BOUNDS) {
        position.premoves.clear();
        ui.selected_square = NO_SQUARE_SELECTED;
        return;
    }

    if (ui.selected_square != NO_SQUARE_SELECTED && ui.selected_square != square) {
        
        Move move = output_candidate_move(ui.selected_square, square, true);
        ui.selected_square = NO_SQUARE_SELECTED; 
        position.premoves.push_back(move);
        //std::cout << position.turn << ' ' << move.get_piece() << ' ' << move.get_to_square() / 8 << '\n';
        if ((position.turn == WHITE && move.get_piece() == BLACK_PAWN && move.get_to_square() / 8 == 0) || 
            (position.turn == BLACK && move.get_piece() == WHITE_PAWN && move.get_to_square() / 8 == 7)) {
            game.state = Gamestate::Promoting_pawn_premove;
        }
    } else {
        ui.selected_square = square;
    } 
}

void Application::set_piece_dragging() {
    if (ui.selected_square != NO_SQUARE_SELECTED) {
        ui.is_dragging = true;
        //assets.current_mouse_pos = world_pos;
        ui.dragged_piece = position.board[ui.selected_square];
    }
}

inline int Application::find_square_selected() {
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    //std::cout << row << ' ' << col << '\n';
    if (row < 0 || col < 0 || row > 7 || col > 7) {
        return OUT_OF_BOUNDS;
    }
    int square = 56 - 8 * row + col;
    if (ui.view == BLACK) {
        square = square ^ 63;
    }
    return square;
}

void Application::handle_clicks_promoting(bool premove) {
    bool selection_made = select_promotion_piece(premove);
    if (selection_made) {
        if (premove) {
            position.premoves.back().set_promotion_piece(ui.piece_selected);
        }
        game.state = Gamestate::Playing;
        if (!premove) {
            play_sound(position.move_record.back());
            game.is_game_over();
        }
        //std::cout << std::bitset<64>(game.zobrist_hash) << '\n';
    }
}

void Application::handle_clicks_resetting() {
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

void Application::handle_clicks_undoing() {
    if (assets.undo_button.is_clicked(mouse_pos) && assets.allow_takebacks) {
        if (game.mode == Gamemode::Twoplayer) {
            game.undo_game_move();
        } else {
            game.undo_game_move();
            game.undo_game_move();
        }
        verify_board_sync(position);
        verify_zobrist_sync(position);
    }
}

void Application::handle_clicks_returning() {
    if (assets.home_button.is_clicked(mouse_pos)) {
        terminate_search = true;
        thinking_in_progress = false;
        finished = false;
        game.state = Gamestate::Intro;
    }
}

void Application::handle_clicks_flip_view() {
    if (assets.flip_view_button.is_clicked({mouse_pos})) {
        ui.view = (ui.view == WHITE) ? BLACK : WHITE;
    }
    set_clock_positions(ui.view);
}

void Application::handle_clicks_resigning() {
    if (assets.resign_button.is_clicked({mouse_pos})) {
        terminate_search = true;
        thinking_in_progress = false;
        finished = false;
        game.end_game(false, true);
        game.state = Gamestate::Gameover;
    }
}

void Application::handle_clicks_navigate_forward() {
    if (assets.go_forward_button.is_clicked({mouse_pos})) {
        if (log.current_ply_num < position.move_record.size()) {
            Move chosen_move = position.move_record[log.current_ply_num];
            int result = position.validate_move(chosen_move);
            game.make_game_move(result, chosen_move, true);
            if (ui.promoting_pawn) {
                game.handle_pawn_promotion(chosen_move, true, true);
            }
            position.evaluate_king_checks();
        }
    }
}

void Application::handle_clicks_navigate_backward() {
    if (assets.go_back_button.is_clicked({mouse_pos})) {
        if (log.current_ply_num > 0) {
            game.undo_game_move(true);
        }
    }
}

void Application::set_clock_positions(int& view) {
    if (view == WHITE) {
        assets.white_clock.set_new_position({16, 425}, {10, 408});
        assets.black_clock.set_new_position({16, 335}, {10, 318});
    } else {
        assets.white_clock.set_new_position({16, 335}, {10, 318});
        assets.black_clock.set_new_position({16, 425}, {10, 408});
    }
}

void Application::draw_board() {
    float x_offset {}, y_offset {};

    uint64_t prev_move_squares = 0ULL;
    uint64_t premove_squares = 0ULL;
    Move prev_move;
    int to_square, from_square;
    if (position.move_record.size() > 0 && log.current_ply_num != 0) {
        prev_move = position.move_record[log.current_ply_num - 1];
        //std::cout << game.move_record[game.current_ply_num - 2] << '\n';
        to_square = prev_move.get_to_square();
        from_square = prev_move.get_from_square();
        if (ui.view == BLACK) {
            to_square ^= 63;
            from_square ^= 63;
        } 
        if (to_square != from_square) {
            prev_move_squares |= (1ULL << to_square);
            prev_move_squares |= (1ULL << from_square);
        }
    }

    for (Move& move : position.premoves) {
        int from_square = move.get_from_square();
        int to_square = move.get_to_square();
        if (ui.view == BLACK) {
            from_square ^= 63;
            to_square ^= 63;
        }
        premove_squares |= (1ULL << from_square);
        premove_squares |= (1ULL << to_square);
    }
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            sf::RectangleShape cell({SQUARE_SIZE, SQUARE_SIZE});
            x_offset = 120 + j * (SQUARE_SIZE);
            y_offset = 20 + i * (SQUARE_SIZE);
            int square = 56 - 8 * i + j;
            if (((i + j) & 1) != 0) {
                if (((1ULL << square) & prev_move_squares) && ((1ULL << square) & premove_squares)) {
                    cell.setFillColor(sf::Color(128, 0, 128));
                } else if ((1ULL << square) & prev_move_squares) {
                    cell.setFillColor(sf::Color(1, 140, 32));
                } else if ((1ULL << square) & premove_squares) {
                    cell.setFillColor(sf::Color::Red);
                } else {
                    cell.setFillColor(sf::Color(165, 42, 42));
                }
            } else {
                if (((1ULL << square) & prev_move_squares) && ((1ULL << square) & premove_squares)) {
                    cell.setFillColor(sf::Color(177, 156, 217));
                } else if ((1ULL << square) & prev_move_squares) {
                    cell.setFillColor(sf::Color(144, 238, 144));
                } else if ((1ULL << square) & premove_squares) {
                    cell.setFillColor(sf::Color(255, 176, 156));
                } else {
                    cell.setFillColor(sf::Color::Yellow);
                }
            } 
            if ((square == ui.selected_square && ui.view == WHITE) ||
                ((square ^ 63) == ui.selected_square && ui.view == BLACK)) {
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
            
            if (!ui.is_dragging || square != ui.selected_square) {
                if (position.board[square] != EMPTY_SQUARE) {
                    draw_piece(rank, file, position.board[square]);
                }
            } else {
                continue;
            }
        }
    }
    if (ui.is_dragging) {
        draw_piece(7 - ui.selected_square / 8, ui.selected_square % 8, 
            position.board[ui.selected_square], true);
    } 
}

void Application::draw_end_screen() {
    Result& result = game.result;

    if (result.status & (1UL << 5)) {
        if (result.winner == WHITE) {
            assets.end_screen.set_text("WHITE WON\nBY RESIGNATION\n-----------------------------\nClick anywhere to \ncontinue");
        } else {
            assets.end_screen.set_text("BLACK WON\nBY RESIGNATION\n-----------------------------\nClick anywhere to \ncontinue");
        }
    } else if (result.status & (1UL << 4)) {
        if (result.winner == WHITE) {
            assets.end_screen.set_text("WHITE WON\nON TIME\n-----------------------------\nClick anywhere to \ncontinue");
        } else {
            assets.end_screen.set_text("BLACK WON\nON TIME\n-----------------------------\nClick anywhere to \ncontinue");
        }
    } else if (result.status & (1UL << 3)) {
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

void Application::draw_pawn_promotion_screen(bool premove) {

    sf::Color colour;
    int piece_colour;
    if (!premove) {
        colour = (position.turn == WHITE) ? sf::Color::White : sf::Color::Black;
        piece_colour = position.turn;
    } else {
        colour = (position.turn == BLACK) ? sf::Color::White : sf::Color::Black;
        piece_colour = !position.turn;
    }
    
    assets.pawn_promotion_screen.set_text_colour(colour);
    assets.pawn_promotion_screen.draw(window);

    for (int piece_idx = 2; piece_idx <= 5; piece_idx++) {
        draw_piece(4, piece_idx, piece_array[piece_colour][piece_idx], false, true);
    }
}

void Application::draw_piece(int x, int y, uint8_t piece, bool dragging, bool for_pawn_promotion_options) {

    sf::Sprite sprite(assets.array[piece]);
    sprite.setScale({0.1f, 0.1f});

    if (!dragging) {
        float y_offset;
        float x_offset { static_cast<float>(135 + y * (SQUARE_SIZE)) };
        if (ui.view == WHITE || for_pawn_promotion_options) {
            y_offset = (30 + x * (SQUARE_SIZE));
            x_offset = 135 + y * (SQUARE_SIZE);
        } else {
            y_offset = (695 - x * (SQUARE_SIZE));
            x_offset = 135 + (7 - y) * SQUARE_SIZE;
        }
        sprite.setPosition({x_offset, y_offset});
    } else {
        if (world_pos.x > 880 || world_pos.x < 110 ||
            world_pos.y > 781 || world_pos.y < 13) {
            // if the piece is dragged out of bounds, snap it back in place
            ui.selected_square = NO_SQUARE_SELECTED;
            ui.is_dragging = false;
            return;
        } else {
            sf::FloatRect bounds = sprite.getLocalBounds();
            sprite.setOrigin({bounds.size.x / 2, bounds.size.y / 2});
            sprite.setPosition(world_pos);
        }
        //std::cout << assets.current_mouse_pos.y << '\n';
    }

    if (piece == WHITE_KING && position.white_in_check && position.turn == WHITE) {
        sprite.setColor(sf::Color(255, 0, 0, 100));
    } else if (piece == BLACK_KING && position.black_in_check && position.turn == BLACK) {
        sprite.setColor(sf::Color(255, 0, 0, 100));
    }
    window.draw(sprite);
}

int Application::select_square() {

    int square = find_square_selected();
    if (square == OUT_OF_BOUNDS) {
        return OUT_OF_BOUNDS;
    }

    uint64_t mask = 1ULL << square;
    if (ui.selected_square != NO_SQUARE_SELECTED && ui.selected_square != square) {

        Move move = output_candidate_move(ui.selected_square, square);
        int result = position.validate_move(move);
        ui.selected_square = NO_SQUARE_SELECTED; 
  
        if (result >= 0) {
            position.current_move = move;
        } 
        return result;
    } else if (((mask & position.bitboards.occupied_tables[WHITE]) && position.turn == WHITE) || 
        ((mask & position.bitboards.occupied_tables[BLACK]) && position.turn == BLACK)) {
        ui.selected_square = square;
        return INVALID;
  
    } 
    return -3;
}

Move Application::output_candidate_move(int& from_square, int& to_square, bool premove) {

    Move move;
    if (position.board[to_square] != EMPTY_SQUARE && !premove) {
        move.set_move(from_square, to_square, position.board[from_square], position.board[to_square]);
    } else {
        move.set_from_square(from_square);
        move.set_to_square(to_square);
        move.set_piece(position.board[from_square]);
    }
    return move;
}

void Application::process_move(int result) {
    bool move_made = false;
    if (result >= 0 && !is_computer_turn()) {     
        game.make_game_move(result, position.current_move);  
        move_made = true;
    }
    if (ui.promoting_pawn) {
        game.state = Gamestate::Promoting_pawn;
        return;
    }
    if (move_made) {
        play_sound(position.current_move); 
        verify_board_sync(position);
        verify_zobrist_sync(position);
        game.is_game_over();
    }
}

inline bool Application::is_computer_turn() {
    if ((position.turn == WHITE && game.mode == Gamemode::CPUwhite) || (position.turn == BLACK &&
        game.mode == Gamemode::CPUblack)) {
        return true;
    }
    return false;
}

bool Application::select_promotion_piece(bool premove) {
    
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);

    if (row == 4 && col == 2) {
        ui.piece_selected = P_KNIGHT;
    } else if (row == 4 && col == 3) {
        ui.piece_selected = P_BISHOP;
    } else if (row == 4 && col == 4) {
        ui.piece_selected = P_ROOK;
    } else if (row == 4 && col == 5) {
        ui.piece_selected = P_QUEEN;
    }
    if (ui.piece_selected != -1) {
        if (!premove) {
            game.handle_pawn_promotion(position.current_move);
        }
        return true;
    }
    return false;
}

