#include <iostream>
#include <cmath>
#include "logic.h"
#include "gui.h"

void handle_input(Game& game, sf::RenderWindow& window) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
   
        if (const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>()) {
            //std::cout << "clicked\n";
            if (game.state == Gamestate::Intro) {
                handle_clicks_intro(game, window, mouse_press->position);
            } else if (game.state == Gamestate::Playing && game.mode == Gamemode::Twoplayer) {
               
                handle_clicks_playing(game, window, mouse_press->position);
                handle_clicks_undoing(game, mouse_press->position);
            } else if (game.mode == Gamemode::CPUwhite && game.state == Gamestate::Playing) {
                if (game.turn == black) {
                    handle_clicks_playing(game, window, mouse_press->position);
                    handle_clicks_undoing(game, mouse_press->position);                     
                } 
            } else if (game.mode == Gamemode::CPUblack && game.state == Gamestate::Playing) {
                if (game.turn == white) {
                    handle_clicks_playing(game, window, mouse_press->position);
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
            
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
            window.setView(sf::View(visibleArea));
        }
    }
}

void render(Game& game, sf::RenderWindow& window) {
    
    window.clear(sf::Color::Blue);
    sf::RectangleShape board({760, 760});
    if (game.state == Gamestate::Intro) {
        draw_intro_screen(window, game);
    }
    if (game.state != Gamestate::Intro) {
        draw_board(game, window);
        draw_return_to_home_button(window);
    }
    if (game.state != Gamestate::Gameover && game.state != Gamestate::Resetting && game.state != Gamestate::Intro) {
        draw_undo_button(window);
    }
    if (game.state == Gamestate::Gameover) {
        draw_end_screen(game, window);
    } else if (game.state == Gamestate::Resetting) {
        draw_reset_button(window);
    }
    if (game.state == Gamestate::Promoting_pawn) {
        draw_pawn_promotion_screen(game, window);
    }
    window.display();
}

void draw_intro_screen(sf::RenderWindow& window, Game& game) {
    sf::Font font;
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
   
    sf::RectangleShape play_button = make_rectangle({330, 330}, {350, 160}, "white");
 
    sf::Text text = configure_text(font, "Chess", {200, 20}, 210, "black");
    sf::Text text2 = configure_text(font, "Play", {450, 380}, 60, "black");
 
    sf::RectangleShape choice1_button = make_rectangle({110, 530}, {250, 110}, "white");
    sf::RectangleShape choice2_button = make_rectangle({400, 530}, {250, 110}, "white");
    sf::RectangleShape choice3_button = make_rectangle({690, 530}, {250, 110}, "white");

    sf::Text choice1_text = configure_text(font, "Play CPU as\n white", {130, 540}, 30, "black");
    sf::Text choice2_text = configure_text(font, "Play CPU as\n black", {420, 540}, 30, "black");
    sf::Text choice3_text = configure_text(font, "Two player", {710, 540}, 30, "black");

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
    window.draw(choice1_button);
    window.draw(choice2_button);
    window.draw(choice3_button);
    window.draw(text);
    window.draw(play_button);
    window.draw(text2);
    window.draw(choice1_text);
    window.draw(choice2_text);
    window.draw(choice3_text);
}

sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, const std::string& colour) {
    sf::Text text(font);
    text.setString(string);
    text.setPosition(pos);
    if (colour == "black") {
        text.setFillColor(sf::Color::Black);
    } else if (colour == "red") {
        text.setFillColor(sf::Color::Red);
    }
    text.setCharacterSize(size);
    return text;
}

sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, const std::string& colour) {
    sf::RectangleShape rectangle(size);
    rectangle.setPosition(pos);
    if (colour == "white") {
        rectangle.setFillColor(sf::Color::White);
    } else if (colour == "black") {
        rectangle.setFillColor(sf::Color::Black);
    } else if (colour == "blue") {
        rectangle.setFillColor(sf::Color::Blue);
    }
    return rectangle;
}

void handle_clicks_intro(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos) {
    int x = mouse_pos.x;
    int y = mouse_pos.y;
    if (330 <= x && x <= 680 && 160 <= y && y <= 490) {
        game.state = Gamestate::Playing;
        game.initialise();
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
}

void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos) {
    int result = select_square(mouse_pos.x, mouse_pos.y, game);

    if (result >= 0) {
        /*std::cout << game.current_move.prev_row << ' ' << game.current_move.prev_col << 
        ' ' << game.current_move.new_row << ' ' << game.current_move.new_col << '\n';*/
        make_game_move(game, game.bitboards, game.board, result, game.current_move);  
    }
    if (game.promoting_pawn) {
        draw_pawn_promotion_screen(game, window);
        game.state = Gamestate::Promoting_pawn;
        return;
    }
    print_all_bitboards(game.bitboards);
    if (result >= 0) {
        game.turn = ((game.turn == white) ? black : white);
        is_game_over(game);
    }
}

void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos) {
    bool selection_made = select_pawn_promotion(game, mouse_pos);
    if (selection_made) {
        game.state = Gamestate::Playing;
        game.turn = ((game.turn == white) ? black : white);
        is_game_over(game);
    }
    print_all_bitboards(game.bitboards);
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

void draw_reset_button(sf::RenderWindow& window) {

    sf::RectangleShape reset_button = make_rectangle({10, 10}, {44, 35}, "white");
    
    sf::Font font;
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text = configure_text(font, "Reset", {13, 13}, 15, "red");

    window.draw(reset_button);
    window.draw(text);
}

void draw_undo_button(sf::RenderWindow& window) {

    sf::RectangleShape undo_button = make_rectangle({950, 10}, {44, 35}, "white");

    sf::Font font;
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text = configure_text(font, "Undo", {953, 13}, 15, "red");

    window.draw(undo_button);
    window.draw(text);
}

void draw_return_to_home_button(sf::RenderWindow& window) {

    sf::RectangleShape return_button = make_rectangle({10, 55}, {104, 35}, "white");

    sf::Font font;
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text = configure_text(font, "Back to Home", {13, 58}, 15, "red");

    window.draw(return_button);
    window.draw(text);
}

void draw_board(Game& game, sf::RenderWindow& window) {
    float x_offset {}, y_offset {};
    
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            sf::RectangleShape cell({SQUARE_SIZE, SQUARE_SIZE});
            x_offset = 120 + j * (SQUARE_SIZE);
            y_offset = 20 + i * (SQUARE_SIZE);
            
            if (game.board[i][j].colour == "brown") {
                if (game.view == white) {
                    cell.setFillColor(sf::Color(165, 42, 42));
                } else {
                    cell.setFillColor(sf::Color::Yellow);
                }
            } else {
                if (game.view == white) {
                    cell.setFillColor(sf::Color::Yellow);
                } else {
                    cell.setFillColor(sf::Color(165, 42, 42));
                }
            }
            if (game.board[i][j].selected && game.view == white) {
                //std::cout << i << " " << j << '\n';
                //std::cout << x_offset << " " << y_offset << '\n';
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);
            } else if (game.board[7 - i][j].selected && game.view == black) {
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

            if (game.bitboards.bitboards[0][0] & mask) {
                draw_piece(game, window, rank, file, "white pawn");
            } else if (game.bitboards.bitboards[1][0] & mask) {
                draw_piece(game, window, rank, file, "black pawn");
            } else if (game.bitboards.bitboards[0][1] & mask) {
                draw_piece(game, window, rank, file, "white knight");
            } else if (game.bitboards.bitboards[1][1] & mask) {
                draw_piece(game, window, rank, file, "black knight");
            } else if (game.bitboards.bitboards[0][2] & mask) {
                draw_piece(game, window, rank, file, "white bishop");
            } else if (game.bitboards.bitboards[1][2] & mask) {
                draw_piece(game, window, rank, file, "black bishop");
            } else if (game.bitboards.bitboards[0][3] & mask) {
                draw_piece(game, window, rank, file, "white rook");
            } else if (game.bitboards.bitboards[1][3] & mask) {
                draw_piece(game, window, rank, file, "black rook");
            } else if (game.bitboards.bitboards[0][4] & mask) {
                draw_piece(game, window, rank, file, "white queen");
            } else if (game.bitboards.bitboards[1][4] & mask) {
                draw_piece(game, window, rank, file, "black queen");
            } else if (game.bitboards.bitboards[0][5] & mask) {
                draw_piece(game, window, rank, file, "white king");
            } else if (game.bitboards.bitboards[1][5] & mask) {
                draw_piece(game, window, rank, file, "black king");
            }
        }
    }
}

void draw_end_screen(Game& game, sf::RenderWindow& window) {
   
    sf::Font font;
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text(font);
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

    sf::RectangleShape end_screen = make_rectangle({250, 250}, {500, 300}, "black");

    window.draw(end_screen);
    window.draw(text);
}

void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window) {

    sf::RectangleShape pawn_promotion_screen = make_rectangle({250, 250}, {500, 300}, "blue");

    window.draw(pawn_promotion_screen);
    int row = ((game.view == white) ? 4 : 3);
    std::shared_ptr<Piece> piece1 = std::make_shared<Piece> (rook, game.turn, false, row, 2);
    std::shared_ptr<Piece> piece2 = std::make_shared<Piece> (knight, game.turn, false, row, 3);
    std::shared_ptr<Piece> piece3 = std::make_shared<Piece> (bishop, game.turn, false, row, 4);
    std::shared_ptr<Piece> piece4 = std::make_shared<Piece> (queen, game.turn, false, row, 5);
    
    std::vector<std::shared_ptr<Piece>> promotion_pieces { piece1, piece2, piece3, piece4 };
    for (auto piece: promotion_pieces) {
        //draw_piece(game, window, piece);
    }
    window.display();
}

void draw_piece(Game& game, sf::RenderWindow& window, int x, int y, std::string piece) {
    //std::string piece_type = piece->piece_type;
    sf::Texture texture;

    if (piece == "black pawn") {
        if (!texture.loadFromFile("./assets/images/Chess_pdt45.png")) {
            return;
        }
    } else if (piece == "white pawn") {
        if (!texture.loadFromFile("./assets/images/Chess_plt45.png")) {
            return;
        }
    } else if (piece == "black knight") {
        if (!texture.loadFromFile("./assets/images/Chess_ndt45.png")) {
            return;
        }
    } else if (piece == "white knight") {
        if (!texture.loadFromFile("./assets/images/Chess_nlt45.png")) {
            return;
        }
    }  else if (piece == "black bishop") {
        if (!texture.loadFromFile("./assets/images/Chess_bdt45.png")) {
            return;
        }
    } else if (piece == "white bishop") {
        if (!texture.loadFromFile("./assets/images/Chess_blt45.png")) {
            return;
        }
    } else if (piece == "black rook") {
        if (!texture.loadFromFile("./assets/images/Chess_rdt45.png")) {
            return;
        }
    } else if (piece == "white rook") {
        if (!texture.loadFromFile("./assets/images/Chess_rlt45.png")) {
            return;
        }
    } else if (piece == "black queen") {
        if (!texture.loadFromFile("./assets/images/Chess_qdt45.png")) {
            return;
        }
    } else if (piece == "white queen") {
        if (!texture.loadFromFile("./assets/images/Chess_qlt45.png")) {
            return;
        }
    } else if (piece == "black king") {
        if (!texture.loadFromFile("./assets/images/Chess_kdt45.png")) {
            return;
        }
    } else if (piece == "white king") {
        if (!texture.loadFromFile("./assets/images/Chess_klt45.png")) {
            return;
        }
    }

    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});
    float y_offset;
    float x_offset { static_cast<float>(135 + y * (SQUARE_SIZE)) };
    if (game.view == white) {
        y_offset = (30 + x * (SQUARE_SIZE));
    } else {
        y_offset = (695 - x * (SQUARE_SIZE));
    }
    if (piece == "white king") {
        if (game.white_in_check && game.turn == white) {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        } 
    } else if (piece == "black king") {
        if (game.black_in_check && game.turn == black) {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        }
    }
    sprite.setPosition({x_offset, y_offset});
    window.draw(sprite);

}

int select_square(int x, int y, Game& game) {
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    int square = 56 - 8 * row + col;
    uint64_t mask = 1ULL << square;
    if (game.view == black) {
        row = 7 - row;
    }
    
    if (row < 0 || col < 0 || row > 7 || col > 7) {
        return -2;
    }
    //std::cout << "Coords - row: " << row << " " << "col: "<< col << '\n';
    if (game.selected.row > -1 && game.selected.col > -1) {

        Move move { game.selected.row, game.selected.col, row, col, game.turn, 
            game.board[game.selected.row][game.selected.col].piece_occupying->piece_type };

        if (game.board[row][col].piece_occupying) {
            move.piece_taken = game.board[row][col].piece_occupying;
        }
        int result = validate_move(game, game.bitboards, game.board, move);
        game.board[game.selected.row][game.selected.col].selected = false;
        game.selected.row = game.selected.col = -1; 
  
        if (result >= 0) {
            game.current_move = move;
            return result;
        } 
        return -1;
    } else if (((mask & game.bitboards.white_occupied) && game.turn == white) || 
                ((mask & game.bitboards.black_occupied) && game.turn == black)) {
        game.board[row][col].selected = true;
        game.selected.row = row;
        game.selected.col = col;
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

