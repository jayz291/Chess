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
            view.setCenter({500.f, 400.f});
            window.setView(view);
        }
        if (const auto* mouse_release = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (game.is_dragging) {
                sf::Vector2f world_pos = window.mapPixelToCoords(mouse_release->position);
                sf::Vector2i game_pos = { (int)world_pos.x, (int)world_pos.y};
                handle_drag_release(game, window, game_pos, assets);
            }

            game.is_dragging = false;
        }
    }
}

void delegate_click_event(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2f& world_pos) {
    sf::Vector2i game_pos = {(int)world_pos.x, (int)world_pos.y};
    if (game.state == Gamestate::Intro) {
        handle_clicks_intro(game, window, assets, game_pos);
    } else if (game.state == Gamestate::Playing) {
        if (game.mode == Gamemode::Twoplayer || (game.mode == Gamemode::CPUwhite && game.turn == BLACK) ||
            game.mode == Gamemode::CPUblack && game.turn == WHITE) {
            handle_clicks_playing(game, window, game_pos, assets);
            if (game.selected_square != -1) {
                game.is_dragging = true;
                assets.current_mouse_pos = world_pos;
                game.dragged_piece = game.board[game.selected_square];
            }
            handle_clicks_undoing(game, assets, game_pos);
        }
    } else if (game.state == Gamestate::Promoting_pawn) {
        handle_clicks_promoting(game, game_pos);
    } else if (game.state == Gamestate::Gameover) {
        game.state = Gamestate::Resetting;
    } else if (game.state == Gamestate::Resetting) {
        handle_clicks_resetting(game, assets, game_pos);
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
    if (game.state == Gamestate::Intro) {
        draw_intro_screen(window, game, assets, mouse_pos);
    }
    if (game.state != Gamestate::Intro) {
        draw_board(game, window, assets);
  
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
        draw_end_screen(game, window, assets);
    } else if (game.state == Gamestate::Resetting) {
        assets.reset_button.update(window, mouse_pos);
    }
    if (game.state == Gamestate::Promoting_pawn) {
        draw_pawn_promotion_screen(game, window, assets);
    }
    if (game.mode == Gamemode::Twoplayer && game.state != Gamestate::Intro) {
        assets.flip_view_button.update(window, mouse_pos);
    }
    window.display();
}

void draw_intro_screen(sf::RenderWindow& window, Game& game, Assets& assets, sf::Vector2i& mouse_pos) {
 
    sf::Text title = configure_text(assets.font, "Chess", {200, 20}, 210, sf::Color::Black);

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
        int result = handle_fen_string(game);
        if (result == 0) {
            //std::cout << std::bitset<64>(game.zobrist_hash) << '\n';
            game.invalid_fen_position = false;
            game.state = Gamestate::Playing;
            //run_perft_suite(game, 5);
            is_game_over(game);
            verify_board_sync(game);
            verify_zobrist_sync(game);
        } else {
            game.invalid_fen_position = true;
        }
    }
    if (assets.play_black_cpu.is_clicked(mouse_pos)) {
        game.view = WHITE;
        game.mode = Gamemode::CPUblack;
    } else if (assets.play_white_cpu.is_clicked(mouse_pos)) {
        game.view = BLACK;
        game.mode = Gamemode::CPUwhite;
    } else if (assets.play_two_player.is_clicked(mouse_pos)) {
        game.view = WHITE;
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
    int result = select_square(mouse_pos.x, mouse_pos.y, game);
    //std::cout << result << '\n';
    if (result >= 0) {
        make_game_move(game, result, game.current_move);  
    }
    if (game.promoting_pawn) {
        game.state = Gamestate::Promoting_pawn;
        return;
    }
    //print_all_bitboards(game.bitboards);
    if (result >= 0) {
        //std::cout << std::bitset<64>(game.zobrist_hash) << '\n';
        verify_board_sync(game);
        verify_zobrist_sync(game);
        is_game_over(game);
    }
}

void handle_drag_release(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets) {
             
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    int square = 56 - 8 * row + col;
    if (game.view == BLACK) {
        square = square ^ 56;
    }
    if (square != game.selected_square) {
        Move move;
        move.set_from_square(game.selected_square);
        move.set_to_square(square);
        move.set_piece(game.dragged_piece);
        if (game.board[square] != EMPTY_SQUARE) {
            //move.get_captured_piece() = game.board[square];
            move.set_captured(game.board[square]);
        }

        int result = validate_move(game, move);
        if (result >= 0) {
            game.current_move = move;
            game.selected_square = -1;
            make_game_move(game, result, game.current_move);  
        }
        if (game.promoting_pawn) {
            game.state = Gamestate::Promoting_pawn;
            return;
        }
        //print_all_bitboards(game.bitboards);
        if (result >= 0) {
            is_game_over(game);
        }
    }
}

void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos) {
    bool selection_made = select_promotion_piece(game, mouse_pos);
    if (selection_made) {
        game.state = Gamestate::Playing;
        is_game_over(game);
        //std::cout << std::bitset<64>(game.zobrist_hash) << '\n';
    }
    //print_all_bitboards(game.bitboards);
}

void handle_clicks_resetting(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    if (assets.reset_button.is_clicked(mouse_pos)) {
        std::string fen_string = game.entered_fen;
        game.initialise();
        game.final_fen = fen_string;
        handle_fen_string(game);
 
        game.state = Gamestate::Playing;
        is_game_over(game);
        return;
    }
}

void handle_clicks_undoing(Game& game, Assets& assets, sf::Vector2i mouse_pos) {
    if (assets.undo_button.is_clicked(mouse_pos) && assets.allow_takebacks) {
        if (game.mode == Gamemode::Twoplayer) {
            undo_game_move(game);
        } else {
            undo_game_move(game);
            undo_game_move(game);
        }
        verify_board_sync(game);
        verify_zobrist_sync(game);
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
    if (920 <= mouse_pos.x && mouse_pos.x <= 994 && 55 <= mouse_pos.y && mouse_pos.y <= 90) {
        game.view = (game.view == WHITE) ? BLACK : WHITE;
    }
}

void draw_board(Game& game, sf::RenderWindow& window, Assets& assets) {
    float x_offset {}, y_offset {};

    bool prev_move_available { false };
    Move prev_move;
    int to_square, from_square;
    if (game.move_record.size() > 0) {
        
        prev_move = game.move_record.back();
        to_square = prev_move.get_to_square();
        from_square = prev_move.get_from_square();
        if (to_square != from_square) {
            prev_move_available = true;
        }
        if (game.view == BLACK) {
            to_square ^= 56;
            from_square ^= 56;
        } 
    }
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            sf::RectangleShape cell({SQUARE_SIZE, SQUARE_SIZE});
            x_offset = 120 + j * (SQUARE_SIZE);
            y_offset = 20 + i * (SQUARE_SIZE);
            int square = 56 - 8 * i + j;
            if (((i + j) & 1) != 0) {
                if (game.view == WHITE) {
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
            } else {
                if (game.view == WHITE) {
                    if (prev_move_available && (square == from_square ||
                        square == to_square)) {
                        cell.setFillColor(sf::Color(144, 238, 144));
                    } else {
                        cell.setFillColor(sf::Color::Yellow);
                    }
                } else {
                    if (prev_move_available && (square == from_square ||
                        square == to_square)) {
                        cell.setFillColor(sf::Color(1, 140, 32));
                    } else {
                        cell.setFillColor(sf::Color(165, 42, 42));
                    }
                }
            }
            if ((square == game.selected_square && game.view == WHITE) ||
                ((square ^ 56) == game.selected_square && game.view == BLACK)) {
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
            
            if (!game.is_dragging || square != game.selected_square) {
                if (game.board[square] != EMPTY_SQUARE) {
                    draw_piece(game, window, assets, rank, file, game.board[square]);
                }
            } else {
                continue;
            }
        }
    }
    if (game.is_dragging) {
        draw_piece(game, window, assets, 7 - game.selected_square / 8, game.selected_square % 8, 
            game.board[game.selected_square], true);
    } 
}

void draw_end_screen(Game& game, sf::RenderWindow& window, Assets& assets) {

    if (game.game_status & (1UL << 3)) {
        if (game.winner == WHITE) {
            assets.end_screen.set_text("CHECKMATE\nWHITE WON!\n-----------------------------\nClick anywhere to \ncontinue");
        } else {
            assets.end_screen.set_text("CHECKMATE\nBLACK WON!\n-----------------------------\nClick anywhere to \ncontinue");
        }
    } else if (game.game_status & (1UL << 2)) {
        assets.end_screen.set_text("Draw by stalemate\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.game_status & (1UL << 1)) {
        assets.end_screen.set_text("Draw by threefold \nrepetition\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.game_status & (1UL)) {
        assets.end_screen.set_text("Draw by insufficient \nmaterial\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.plys_to_100 == 100) {
        assets.end_screen.set_text("Draw by the 50-move rule\n-----------------------------\nClick anywhere to \ncontinue");
    }
    assets.end_screen.draw(window);
}

void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window, Assets& assets) {

    sf::Color colour = (game.turn == WHITE) ? sf::Color::White : sf::Color::Black;
    assets.pawn_promotion_screen.set_text_colour(colour);
    assets.pawn_promotion_screen.draw(window);
    int rank = ((game.view == WHITE) ? 4 : 3);
    draw_piece(game, window, assets, rank, 2, piece_array[game.turn][ROOK]);
    draw_piece(game, window, assets, rank, 3, piece_array[game.turn][KNIGHT]);
    draw_piece(game, window, assets, rank, 4, piece_array[game.turn][BISHOP]);
    draw_piece(game, window, assets, rank, 5, piece_array[game.turn][QUEEN]);
 
}

void draw_piece(Game& game, sf::RenderWindow& window, Assets& assets, 
    int x, int y, uint8_t piece, bool dragging) {
    //assert(piece >= 0 && piece <= 5);
    sf::Texture texture = assets.array[piece];
    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});

    if (!dragging) {
        float y_offset;
        float x_offset { static_cast<float>(135 + y * (SQUARE_SIZE)) };
        if (game.view == WHITE) {
            y_offset = (30 + x * (SQUARE_SIZE));
        } else {
            y_offset = (695 - x * (SQUARE_SIZE));
        }
        sprite.setPosition({x_offset, y_offset});
    } else {
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin({bounds.size.x / 2, bounds.size.y / 2});
        sprite.setPosition(assets.current_mouse_pos);
    }

    if (piece == WHITE_KING && game.white_in_check && game.turn == WHITE) {
        sprite.setColor(sf::Color(255, 0, 0, 100));
    } else if (piece == BLACK_KING && game.black_in_check && game.turn == BLACK) {
        sprite.setColor(sf::Color(255, 0, 0, 100));
    }
    window.draw(sprite);
}

int select_square(int x, int y, Game& game) {
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    
    if (game.mode == Gamemode::CPUwhite || game.view == BLACK) {
        row = 7 - row;
    }
    int square = 56 - 8 * row + col;
    uint64_t mask = 1ULL << square;
    
    if (row < 0 || col < 0 || row > 7 || col > 7) {
        return -2;
    }
    //std::cout << "Coords - row: " << row << " " << "col: "<< col << '\n';
    if (game.selected_square != -1) {
        
        Move move;
        move.set_from_square(game.selected_square);
        move.set_to_square(square);
        move.set_piece(game.board[game.selected_square]);
        if (game.board[square] != EMPTY_SQUARE) {
            move.set_captured(game.board[square]);
        }
        int result = validate_move(game, move);
        game.selected_square = -1; 
  
        if (result >= 0) {
            game.current_move = move;
            return result;
        } 
        return -1;
    } else if (((mask & game.bitboards.occupied_tables[WHITE]) && game.turn == WHITE) || 
        ((mask & game.bitboards.occupied_tables[BLACK]) && game.turn == BLACK)) {
        game.selected_square = 56 - 8 * row + col;
        return -1;
  
    } 
    return -3;
}

bool select_promotion_piece(Game& game, sf::Vector2i mouse_pos) {
    
    int col = floor(((mouse_pos.x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((mouse_pos.y - 30.f) / (SQUARE_SIZE)) + 0.0842105);

    if (row == 4 && col == 2) {
        game.piece_selected = P_ROOK;
    } else if (row == 4 && col == 3) {
        game.piece_selected = P_KNIGHT;
    } else if (row == 4 && col == 4) {
        game.piece_selected = P_BISHOP;
    } else if (row == 4 && col == 5) {
        game.piece_selected = P_QUEEN;
    }
    if (game.piece_selected != -1) {
        handle_pawn_promotion(game, game.current_move);
        return true;
    }
    return false;
}

