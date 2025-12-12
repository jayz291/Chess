#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <utility>
#include <algorithm>

constexpr int SQUARE_SIZE = 95;

struct Piece {
    std::string piece_type {};
    std::string colour {};
    int moves { 0 };
    int row;
    int col; 
};

struct Cell {
    std::string colour {};
    std::shared_ptr<Piece> piece_occupying { nullptr };
    bool selected { false };
};

struct Coords {
    int row { -1 };
    int col { -1 };
};

struct Move {
    int prev_row {};
    int prev_col {};
    int new_row {};
    int new_col {};
    std::string turn {};
    std::string piece {};
    std::string piece_taken { "None" };
    std::string special_move { "No" };
    int piece_captured_moves {};
};

using Chessboard = std::array<std::array<Cell, 8>, 8>;

enum class Gamestate {
    Playing,
    Promoting_pawn,
    Gameover,
    Resetting
};

class Game {
    public:
        //std::vector<Piece> pieces{32}; 
        Gamestate state {};
        Chessboard board {};
        Coords selected {};
        std::string turn {};
        bool white_in_check {};
        bool black_in_check {};
        Coords black_king_position {};
        Coords white_king_position {};
        bool game_over {};
        bool checkmate {};
        bool stalemate {};
        bool repetition {};
        bool insufficient_material {};
        std::string winner { "None" };
        bool promoting_pawn {};
        std::string piece_selected { "None" };
        std::vector<Move> move_record {};
        std::vector<std::string> board_record {};
        int plys_to_100 {};
        int value_white_pieces {};
        int value_black_pieces {};
        bool pawns_on_board {};
        Move current_move {};
        Game() {
            initialise();
        }
        void initialise() {
            state = Gamestate::Playing;
            current_move = {};
            move_record.clear();
            board_record.clear();
            pawns_on_board = true;
            value_white_pieces = value_black_pieces = 0;
            plys_to_100 = 0;
            winner = piece_selected = "None";
            white_in_check = black_in_check = game_over = false;
            turn = "white";
            checkmate = stalemate = repetition = insufficient_material = promoting_pawn = false;
            selected.col = selected.row = -1;
            black_king_position.row = 0;
            white_king_position.row = 7;
            black_king_position.col = white_king_position.col = 4;
            for (int i { 0 }; i < 8; i++) {
                for (int j { 0 }; j < 8; j++) {
                board[i][j].piece_occupying = nullptr;
                    if ((i + j) % 2 == 0) {
                        board[i][j].colour = "yellow";
                    } else {
                        board[i][j].colour = "brown";
                    }
                }
            }
            for (int i { 0 }; i < 8; i++) {
                board[1][i].piece_occupying = std::make_shared<Piece> ("pawn", "black", 0, 1, i);
            }
            for (int i { 0 }; i < 8; i++) {
                board[6][i].piece_occupying = std::make_shared<Piece> ("pawn", "white", 0, 6, i);
            }
            board[0][0].piece_occupying = std::make_shared<Piece> ("rook", "black", 0, 0, 0);
            board[0][1].piece_occupying = std::make_shared<Piece> ("knight", "black", 0, 0, 1);
            board[0][2].piece_occupying = std::make_shared<Piece> ("bishop", "black", 0, 0, 2);
            board[0][3].piece_occupying = std::make_shared<Piece> ("queen", "black", 0, 0, 3);
            board[0][4].piece_occupying = std::make_shared<Piece> ("king", "black", 0, 0, 4);
            board[0][5].piece_occupying = std::make_shared<Piece> ("bishop", "black", 0, 0, 5);
            board[0][6].piece_occupying = std::make_shared<Piece> ("knight", "black", 0, 0, 6);
            board[0][7].piece_occupying = std::make_shared<Piece> ("rook", "black", 0, 0, 7);
            board[7][0].piece_occupying = std::make_shared<Piece> ("rook", "white", 0, 7, 0);
            board[7][1].piece_occupying = std::make_shared<Piece> ("knight", "white", 0, 7, 1);
            board[7][2].piece_occupying = std::make_shared<Piece> ("bishop", "white", 0, 7, 2);
            board[7][3].piece_occupying = std::make_shared<Piece> ("queen", "white", 0, 7, 3);
            board[7][4].piece_occupying = std::make_shared<Piece> ("king", "white", 0, 7, 4);
            board[7][5].piece_occupying = std::make_shared<Piece> ("bishop", "white", 0, 7, 5);
            board[7][6].piece_occupying = std::make_shared<Piece> ("knight", "white", 0, 7, 6);
            board[7][7].piece_occupying = std::make_shared<Piece> ("rook", "white", 0, 7, 7);
        }
};

void run_game_loop();
void render(Game& game, sf::RenderWindow& window);
void draw_board(Game& game, sf::RenderWindow& window);
void draw_piece(Game& game, sf::RenderWindow& window, std::shared_ptr<Piece>& piece);
void draw_reset_button(Game& game, sf::RenderWindow& window);
void draw_undo_button(Game& game, sf::RenderWindow& window);
void draw_end_screen(Game& game, sf::RenderWindow& window);
void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window);
int select_square(int x, int y, Game& game);
bool select_pawn_promotion(Game& game, sf::Vector2i mouse_pos);
void process_move(Game& game, int result, Move& move);
void move_piece(Chessboard& board, int prev_row, int prev_col, int new_row, int new_col, bool undo = false);
int determine_possible_moves(Game& game);
int determine_repetition(Game& game);
int determine_insufficient_material(Game& game);
void end_game(Game& game);
void record_board(Game& game);
void is_game_over(Game& game);
void undo_move(Game& game);

void handle_input(Game& game, sf::RenderWindow& window);
void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos);
void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_resetting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos);

int validate_move(Game& game, Chessboard& board, Move& move, bool only_checking_checks = false);
int validate_move_pawn(Game& game, Chessboard& board, Move& move);
int validate_move_knight(Chessboard& board, Move& move);
int validate_move_bishop(Chessboard& board, Move& move);
int validate_move_rook(Chessboard& board, Move& move);
int validate_move_queen(Chessboard& board, Move& move);
int validate_move_king(Game& game, Chessboard& board, Move& move);
int check_checks(Game& game, Chessboard& copy, Move& move);
void evaluate_king_checks(Game& game);
int test_castling(Game& game, Chessboard& copy, Move& move);
void handle_pawn_promotion(Game& game);
int validate_en_passant(Game& game, Chessboard& board, Move& move);
void record_piece_points(Game& game, std::string piece_type, std::string piece_colour);


int main() {
    run_game_loop();
}

void run_game_loop() {
    Game game {};
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    
    while (window.isOpen()) {
        handle_input(game, window);
        render(game, window);
    }
}

void handle_input(Game& game, sf::RenderWindow& window) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* mouse_press = event->getIf<sf::Event::MouseButtonPressed>()) {
            //std::cout << "clicked\n";
            if (game.state == Gamestate::Playing) {
                handle_clicks_playing(game, window, mouse_press->position);
                handle_clicks_undoing(game, mouse_press->position);
            } else if (game.state == Gamestate::Promoting_pawn) {
                handle_clicks_promoting(game, mouse_press->position);
            } else if (game.state == Gamestate::Gameover) {
                game.state = Gamestate::Resetting;
            } else if (game.state == Gamestate::Resetting) {
                handle_clicks_resetting(game, mouse_press->position);
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
    draw_board(game, window);
    if (game.state != Gamestate::Gameover && game.state != Gamestate::Resetting) {
        draw_undo_button(game, window);
    }
    if (game.state == Gamestate::Gameover) {
        draw_end_screen(game, window);
    } else if (game.state == Gamestate::Resetting) {
        draw_reset_button(game, window);
    }
    if (game.state == Gamestate::Promoting_pawn) {
        draw_pawn_promotion_screen(game, window);
    }
    window.display();
}

void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos) {
    int result = select_square(mouse_pos.x, mouse_pos.y, game);

    if (result >= 0) {
        /*std::cout << game.current_move.prev_row << ' ' << game.current_move.prev_col << 
        ' ' << game.current_move.new_row << ' ' << game.current_move.new_col << '\n';*/
        process_move(game, result, game.current_move);  
    }
    if (game.promoting_pawn) {
        draw_pawn_promotion_screen(game, window);
        game.state = Gamestate::Promoting_pawn;
        return;
    }
    if (result >= 0) {
        game.turn = ((game.turn == "white") ? "black" : "white");
        is_game_over(game);
    }
}

void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos) {
    bool selection_made = select_pawn_promotion(game, mouse_pos);
    if (selection_made) {
        game.state = Gamestate::Playing;
        game.turn = ((game.turn == "white") ? "black" : "white");
        is_game_over(game);
    }
}

void handle_clicks_resetting(Game& game, sf::Vector2i mouse_pos) {
    float x, y;

    x = mouse_pos.x;
    y = mouse_pos.y;
    if (10 <= x && x <= 54 && 10 <= y && y <= 45) {
        game.initialise();
        return;
    }
}

void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos) {
    int x = mouse_pos.x;
    int y = mouse_pos.y;
    if (950 <= x && x <= 994 && 10 <= y && y <= 45) {
        undo_move(game);
    }
}

void undo_move(Game& game) {
    if (game.move_record.size() == 0) {
        return;
    }
    Move prev_move = game.move_record[game.move_record.size() - 1];
    std::string turn = (((game.move_record.size() - 1) % 2 == 0) ? "black" : "white"); 

    move_piece(game.board, prev_move.new_row, prev_move.new_col, prev_move.prev_row,
    prev_move.prev_col, true);
    if (prev_move.special_move != "en passant" && prev_move.piece_taken != "None") {
        game.board[prev_move.new_row][prev_move.new_col].piece_occupying = 
            std::make_shared<Piece> (prev_move.piece_taken, turn, prev_move.piece_captured_moves, 
                prev_move.new_row, prev_move.new_col);
    }
    int captured_row = ((turn == "black") ? prev_move.new_row + 1 : prev_move.new_row - 1);
    if (prev_move.special_move == "en passant") {
        game.board[captured_row][prev_move.new_col].piece_occupying =
        std::make_shared<Piece> (prev_move.piece_taken, turn, prev_move.piece_captured_moves, 
            captured_row, prev_move.new_col);
    }
    int castling_row = ((turn == "black") ? 7 : 0);
    if (prev_move.special_move == "castling") {
        if (prev_move.new_col - prev_move.prev_col == 2) {
            move_piece(game.board, castling_row, 5, castling_row, 7, true);
        } else {
            move_piece(game.board, castling_row, 3, castling_row, 0, true);
        }
    }
    if (prev_move.special_move == "promotion") {
        game.board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type = "pawn";
    }
    if (prev_move.piece_taken == "None" && prev_move.piece != "pawn") {
        if (game.plys_to_100 > 0) {
            game.plys_to_100--;
        }
    }
    //std::cout << game.board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type << '\n';
 
    if (game.board_record.size() > 0) {
        game.board_record.pop_back();
    }
    game.move_record.pop_back();
    game.turn = ((game.turn == "white") ? "black" : "white");
    evaluate_king_checks(game);
    //std::cout << "undo done\n";
}

void draw_reset_button(Game& game, sf::RenderWindow& window) {

    sf::RectangleShape reset_button({44, 35});
    
    reset_button.setFillColor(sf::Color::White);
    reset_button.setPosition({10, 10});
    
    sf::Font font;
    if (!font.openFromFile("./src/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text(font);
    text.setFillColor(sf::Color::Red);
    text.setCharacterSize(15);
    text.setPosition({13, 13});
    text.setString("Reset");
    window.draw(reset_button);
    window.draw(text);
}

void draw_undo_button(Game& game, sf::RenderWindow& window) {
    sf::RectangleShape undo_button({44, 35});
    undo_button.setFillColor(sf::Color::White);
    undo_button.setPosition({950, 10});
    sf::Font font;
    if (!font.openFromFile("./src/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text(font);
    text.setFillColor(sf::Color::Red);
    text.setCharacterSize(15);
    text.setPosition({953, 13});
    text.setString("Undo");
    window.draw(undo_button);
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
                cell.setFillColor(sf::Color(165, 42, 42));
            } else {
                cell.setFillColor(sf::Color::Yellow);
            }
            if (game.board[i][j].selected) {
                //std::cout << i << " " << j << '\n';
                //std::cout << x_offset << " " << y_offset << '\n';
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);
            }
            cell.setPosition({x_offset, y_offset});
            window.draw(cell);
        }
    }
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying != nullptr) {
                draw_piece(game, window, game.board[i][j].piece_occupying);
                
            }
        }
    }
}

void draw_end_screen(Game& game, sf::RenderWindow& window) {
    sf::RectangleShape end_screen({500, 300});
    sf::Font font;
    if (!font.openFromFile("./src/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text(font);
    text.setFillColor(sf::Color::Red);
    text.setCharacterSize(40);
    
    if (game.checkmate) {
        text.setPosition({280, 280}); 
        if (game.winner == "white") {
            text.setString("CHECKMATE\nWHITE WON!\n-----------------------------\nClick anywhere to \ncontinue");
        } else {
            text.setString("CHECKMATE\nBLACK WON!\n-----------------------------\nClick anywhere to \ncontinue");
        }
    } else if (game.stalemate) {
        text.setPosition({280, 280});
        text.setString("Draw by stalemate\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.repetition) {
        text.setPosition({280, 280});
        text.setString("Draw by threefold \nrepetition\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.insufficient_material) {
        text.setPosition({280, 280});
        text.setString("Draw by insufficient \nmaterial\n-----------------------------\nClick anywhere to \ncontinue");
    } else if (game.plys_to_100 == 100) {
        text.setPosition({280, 280});
        text.setString("Draw by the 50-move rule\n-----------------------------\nClick anywhere to \ncontinue");
    }

    float x_offset = 250;
    float y_offset = 250;
    end_screen.setPosition({x_offset, y_offset});
    end_screen.setFillColor(sf::Color::Black);
    window.draw(end_screen);
    window.draw(text);
}

void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window) {
    sf::RectangleShape pawn_promotion_screen({500, 300});
    float x_offset = 250;
    float y_offset = 250;
    pawn_promotion_screen.setPosition({x_offset, y_offset});
    pawn_promotion_screen.setFillColor(sf::Color::Blue);
    window.draw(pawn_promotion_screen);
    std::shared_ptr<Piece> piece1 = std::make_shared<Piece> ("rook", game.turn, false, 4, 2);
    std::shared_ptr<Piece> piece2 = std::make_shared<Piece> ("knight", game.turn, false, 4, 3);
    std::shared_ptr<Piece> piece3 = std::make_shared<Piece> ("bishop", game.turn, false, 4, 4);
    std::shared_ptr<Piece> piece4 = std::make_shared<Piece> ("queen", game.turn, false, 4, 5);
    
    std::vector<std::shared_ptr<Piece>> promotion_pieces { piece1, piece2, piece3, piece4 };
    for (auto piece: promotion_pieces) {
        draw_piece(game, window, piece);
    }
    window.display();
}

void draw_piece(Game& game, sf::RenderWindow& window, std::shared_ptr<Piece>& piece) {
    //std::string piece_type = piece->piece_type;
    sf::Texture texture;

    if (piece->piece_type == "pawn" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_pdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "pawn" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_plt45.png")) {
            return;
        }
    } else if (piece->piece_type == "knight" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_ndt45.png")) {
            return;
        }
    } else if (piece->piece_type == "knight" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_nlt45.png")) {
            return;
        }
    }  else if (piece->piece_type == "bishop" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_bdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "bishop" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_blt45.png")) {
            return;
        }
    } else if (piece->piece_type == "rook" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_rdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "rook" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_rlt45.png")) {
            return;
        }
    } else if (piece->piece_type == "queen" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_qdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "queen" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_qlt45.png")) {
            return;
        }
    } else if (piece->piece_type == "king" && piece->colour == "black") {
        if (!texture.loadFromFile("./src/Chess_kdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "king" && piece->colour == "white") {
        if (!texture.loadFromFile("./src/Chess_klt45.png")) {
            return;
        }
    }

    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});
    float x_offset { static_cast<float>(135 + piece->col * (SQUARE_SIZE)) };
    float y_offset { static_cast<float>(30 + piece->row * (SQUARE_SIZE)) };
    if (piece->piece_type == "king") {
        if (piece->colour == "white" && game.white_in_check && game.turn == "white") {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        } else if (piece->colour == "black" && game.black_in_check && game.turn == "black") {
            sprite.setColor(sf::Color(255, 0, 0, 100));
        }
    }
    sprite.setPosition({x_offset, y_offset});
    window.draw(sprite);

}

int select_square(int x, int y, Game& game) {
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);
    
    if (row < 0 || col < 0 || row > 7 || col > 7) {
        return -2;
    }
    //std::cout << "Coords - row: " << row << " " << "col: "<< col << '\n';
    if (game.selected.row > -1 && game.selected.col > -1) {

        Move move { game.selected.row, game.selected.col, row, col, game.turn, 
            game.board[game.selected.row][game.selected.col].piece_occupying->piece_type };
        if (game.board[row][col].piece_occupying) {
            move.piece_taken = game.board[row][col].piece_occupying->piece_type;
            move.piece_captured_moves = game.board[row][col].piece_occupying->moves;
        }
        int result = validate_move(game, game.board, move);
        game.board[game.selected.row][game.selected.col].selected = false;
        game.selected.row = game.selected.col = -1; 
  
        if (result >= 0) {
            game.current_move = move;
            return result;
        } 
        return -1;
    } else if (game.board[row][col].piece_occupying && game.board[row][col].piece_occupying->colour == game.turn) {
        game.board[row][col].selected = true;
        game.selected.row = row;
        game.selected.col = col;
        return -1;
    }
    return -3;
}

void process_move(Game& game, int result, Move& move) {
    //std::cout << "moving\n";
    if (move.piece == "pawn" || game.board[move.new_row][move.new_col].piece_occupying) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }
    int row = game.current_move.prev_row, col = game.current_move.prev_col;
    if ((game.turn == "black" && game.board[row][col].piece_occupying->piece_type == "pawn" && row == 6) || 
        (game.turn == "white" && game.board[row][col].piece_occupying->piece_type == "pawn" && row == 1) &&
        std::abs(move.new_row - move.prev_row) == 1) {
        game.promoting_pawn = true;
        return;
    }
    //std::cout << "plys to 100: " << game.plys_to_100 << '\n';
    
    move_piece(game.board, move.prev_row, move.prev_col, move.new_row, move.new_col);

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == "white") { 
            move_piece(game.board, 7, 7, 7, 5);
        } else if (result == 2 && game.turn == "white") {
            move_piece(game.board, 7, 0, 7, 3);
        } else if (result == 1 && game.turn == "black") {
            move_piece(game.board, 0, 7, 0, 5);
        } else if (result == 2 && game.turn == "black") {
            move_piece(game.board, 0, 0, 0, 3);
        }
        game.board_record.clear();
        move.special_move = "castling";
    } else if (result == 3) {
        int captured_row = ((game.turn == "white") ? move.new_row + 1 : move.new_row - 1);
        move.special_move = "en passant";
        move.piece_taken = "pawn";
        game.board[captured_row][move.new_col].piece_occupying = nullptr;
        game.board_record.clear();
    } 
    game.move_record.push_back(move);

    if (game.board[move.new_row][move.new_col].piece_occupying->piece_type == "king") {
        if (game.turn == "white") {
            game.white_king_position.row = move.new_row;
            game.white_king_position.col = move.new_col;
        } else {
            game.black_king_position.row = move.new_row;
            game.black_king_position.col = move.new_col;
        }
    }
}

void is_game_over(Game& game) {
    evaluate_king_checks(game);
    record_board(game);
    int repetition = determine_repetition(game);
    int insufficient_material = determine_insufficient_material(game);
    if (determine_possible_moves(game) == -1 || repetition == -1 || insufficient_material == -1 || 
        game.plys_to_100 == 100) {
        end_game(game);
    }
}

int determine_insufficient_material(Game& game) {
    if (game.value_black_pieces <= 3 && game.value_white_pieces <= 3 && !game.pawns_on_board) {
        game.insufficient_material = true;
        return -1;
    }
    return 0;
}

int determine_repetition(Game& game) {
    int occurrences { 1 };
    int latest_move { static_cast<int>(game.board_record.size() - 1)};
    for (int i { latest_move - 1 }; i >= 0; i--) {
        if (game.board_record[i] == game.board_record[latest_move]) {
            occurrences++;
        }
        if (occurrences == 3) {
            game.repetition = true;
            return -1;
        }
    }
    return 0;
}

bool select_pawn_promotion(Game& game, sf::Vector2i mouse_pos) {
    int x, y;

    x = mouse_pos.x;
    y = mouse_pos.y;
    
    int col = floor(((x - 135.f) / (SQUARE_SIZE)) + 0.1473);
    int row = floor(((y - 30.f) / (SQUARE_SIZE)) + 0.0842105);

    if (row == 4 && col == 2) {
        game.piece_selected = "rook";
    } else if (row == 4 && col == 3) {
        game.piece_selected = "knight";
    } else if (row == 4 && col == 4) {
        game.piece_selected = "bishop";
    } else if (row == 4 && col == 5) {
        game.piece_selected = "queen";
    }
    if (game.piece_selected != "None") {
        handle_pawn_promotion(game);
        return true;
    }
    return false;
}

void handle_pawn_promotion(Game& game) {
    move_piece(game.board, game.current_move.prev_row, game.current_move.prev_col, 
        game.current_move.new_row, game.current_move.new_col);
    game.current_move.special_move = "promotion";
    game.move_record.push_back(game.current_move);
    game.board[game.current_move.new_row][game.current_move.new_col].piece_occupying->piece_type = game.piece_selected;
    game.piece_selected = "None";
    game.promoting_pawn = false;
}

void evaluate_king_checks(Game& game) {
    Move move { 0, 0, 0, 0, game.turn, "None" };
    if (game.turn == "white") {
        std::string next = "black";
        int new_result = check_checks(game, game.board, move);
        if (new_result == 1) {
            game.white_in_check = true;
        } else {
            game.white_in_check = false;
        }
    } else {
        std::string next = "white";
        int new_result = check_checks(game, game.board, move);
        if (new_result == 1) {
            game.black_in_check = true;
        } else {
            game.black_in_check = false;
        }
    }
}

void end_game(Game& game) {
    //std::cout << "It is over\n";
    game.game_over = true;
    game.state = Gamestate::Gameover;
    if (game.repetition || game.insufficient_material || game.plys_to_100 == 100) {
        return;
    }
    if (game.turn == "black") {
        if (game.black_in_check) {
            game.checkmate = true;
            game.winner = "white";
        } else {
            game.stalemate = true;
        }
    } else {
        if (game.white_in_check) {
            game.checkmate = true;
            game.winner = "black";
        } else {
            game.stalemate = true;
        }
    }
}

void move_piece(Chessboard& board, int prev_row, int prev_col, int new_row, int new_col, bool undo) {
    board[new_row][new_col].piece_occupying = nullptr;
    board[new_row][new_col].piece_occupying = std::move(board[prev_row][prev_col].piece_occupying);
    if (board[new_row][new_col].piece_occupying == nullptr) {
        //std::cout << "failed\n";
        return;
    }
    board[new_row][new_col].piece_occupying->row = new_row;
    board[new_row][new_col].piece_occupying->col = new_col;
    if (!undo) {
        board[new_row][new_col].piece_occupying->moves++;
    } else {
        board[new_row][new_col].piece_occupying->moves--;
    }
}

int validate_move(Game& game, Chessboard& board, Move& move, bool only_checking_checks) {
    int result { 0 };
    if (move.new_row > 7 || move.new_col > 7 || move.new_row < 0 || move.new_col < 0) {
        return -1;
    }
    if ((move.prev_row != move.new_row || move.prev_col != move.new_col) && 
        (!board[move.new_row][move.new_col].piece_occupying || 
        board[move.new_row][move.new_col].piece_occupying->colour != move.turn)) {
        
        std::string piece = move.piece;
        if (piece == "pawn") {
            result = validate_move_pawn(game, board, move);
        } else if (piece == "knight") {
            result = validate_move_knight(board, move);
        } else if (piece == "bishop") {
            result = validate_move_bishop(board, move);
        } else if (piece == "rook") {
            result = validate_move_rook(board, move);
        } else if (piece == "queen") {
            result = validate_move_queen(board, move);
        } else if (piece == "king") {
            result = validate_move_king(game, board, move);
        } else {
            return 0;
        }
        if (result >= 0 && !only_checking_checks) {
            Chessboard copy {};
            for (int i { 0 }; i < 8; i++) {
                for (int j { 0 }; j < 8; j++) {
                    copy[i][j].piece_occupying = board[i][j].piece_occupying;
                }
            }
            //std::cout << new_result << '\n';
            if (check_checks(game, copy, move) == 0) {
                return result;
            }
            return -1;
        } else if (result != 0) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

int validate_move_pawn(Game& game, Chessboard& board, Move& move) {
    std::vector<std::pair<int, int>> no_capture_moves {};
    std::vector<std::pair<int, int>> capture_moves {};
    std::pair<int, int> new_move { move.new_row, move.new_col };
    std::vector<std::pair<int, int>>::iterator it {};

    if (move.turn == "white" && move.prev_row == 3 && move.new_row - move.prev_row == -1 && 
        std::abs(move.new_col - move.prev_col) == 1 && !board[move.new_row][move.new_col].piece_occupying) {
        return validate_en_passant(game, board, move);
    } else if (move.turn == "black" && move.prev_row == 4 && move.new_row - move.prev_row == 1 && 
        std::abs(move.new_col - move.prev_col) == 1 && !board[move.new_row][move.new_col].piece_occupying) {
        return validate_en_passant(game, board, move);  
    }

    if (move.turn == "white") {
        if (move.prev_row == 6 && !board[move.prev_row - 1][move.prev_col].piece_occupying) {
            no_capture_moves = { { move.prev_row - 1, move.prev_col }, { move.prev_row - 2, move.prev_col } };
        } else {
            no_capture_moves = { { move.prev_row - 1, move.prev_col } };
        }
        capture_moves = { { move.prev_row - 1, move.prev_col - 1 }, { move.prev_row - 1, move.prev_col + 1 } };

    } else {
        if (move.prev_row == 1 && !board[move.prev_row + 1][move.prev_col].piece_occupying) {
            no_capture_moves = { { move.prev_row + 1, move.prev_col }, { move.prev_row + 2, move.prev_col } };
        } else {
            no_capture_moves = { { move.prev_row + 1, move.prev_col } };
        }
        capture_moves = { { move.prev_row + 1, move.prev_col - 1 }, { move.prev_row + 1, move.prev_col + 1 } };
    }

    if (board[move.new_row][move.new_col].piece_occupying) {
        it = std::find(capture_moves.begin(), capture_moves.end(), new_move);
        if (it != capture_moves.end()) {
            return 0;
        } 
        return -1; 
    } else {
        it = std::find(no_capture_moves.begin(), no_capture_moves.end(), new_move);
        if (it != no_capture_moves.end()) {
            return 0;
        } 
        return -1;
    }   
}

int validate_en_passant(Game& game, Chessboard& board, Move& move) {
    //std::cout << "here\n";
    int col_position {}, required_prev_row {}, required_new_row {};
    col_position = ((move.new_col - move.prev_col == 1) ? move.prev_col + 1 : move.prev_col - 1);
    required_prev_row = ((move.turn == "white") ? 1 : 6);
    required_new_row = ((move.turn == "white") ? 3 : 4);
   
    if (board[move.prev_row][col_position].piece_occupying && 
        board[move.prev_row][col_position].piece_occupying->piece_type == "pawn") {
        Move prev_move = game.move_record[game.move_record.size() - 1];
        //std::cout << "prev" << prev_move.prev_row << ' ' << prev_move.prev_col << 
        //" Curr" << prev_move.new_row << prev_move.new_col << '\n';
        if (prev_move.piece == "pawn" && prev_move.prev_row == required_prev_row && 
            prev_move.new_row == required_new_row && prev_move.new_col == col_position) {
            return 3;
        }
        return -1;
    }
    return -1;
}

int validate_move_knight(Chessboard& board, Move& move) {
    if ((std::abs(move.new_row - move.prev_row) == 2 && std::abs(move.new_col - move.prev_col) == 1) || 
        (std::abs(move.new_row - move.prev_row) == 1 && std::abs(move.new_col - move.prev_col) == 2)) {
        return 0;
    }
    return -1;
}

int validate_move_bishop(Chessboard& board, Move& move) {
    int row_change { move.new_row - move.prev_row };
    int col_change { move.new_col - move.prev_col };

    if (std::abs(row_change) == std::abs(col_change)) {
        int row_direction = ((row_change > 0) ? 1 : -1);
        int col_direction = ((col_change > 0) ? 1 : -1);   
        for (int i { 1 }; i < std::abs(row_change); i++) {
            if (board[move.prev_row + row_direction * i][move.prev_col + col_direction * i].piece_occupying) {
                return -1;
            }
        }
        return 0;
    }
    return -1;
}

int validate_move_rook(Chessboard& board, Move& move) {
    int row_change { move.new_row - move.prev_row };
    int col_change { move.new_col - move.prev_col };

    if (row_change == 0) {
        int col_direction = ((col_change > 0) ? 1 : -1); 
        for (int i { 1 }; i < std::abs(col_change); i++) {
            if (board[move.prev_row][move.prev_col + col_direction * i].piece_occupying) {
                return -1;
            }
        } 
        return 0;
    } else if (col_change == 0) {
        int row_direction = ((row_change > 0) ? 1 : -1);
        for (int i { 1 }; i < std::abs(row_change); i++) {
            if (board[move.prev_row + row_direction * i][move.prev_col].piece_occupying) {
                return -1;
            }
        } 
        return 0;
    }
    return -1;
}

int validate_move_queen(Chessboard& board, Move& move) {
  
    //std::cout << "rook: " << rook_move << " bishop: " << bishop_move << '\n';
    if (validate_move_rook(board, move) == 0 || validate_move_bishop(board, move) == 0) {
        return 0;
    } 
    return -1;
}

int validate_move_king(Game &game, Chessboard& board, Move& move) {
    //std::cout << "Checking king move\n";

    int row_change { move.new_row - move.prev_row };
    int col_change { move.new_col - move.prev_col };
    std::vector<std::pair<int, int>> moves { { move.new_row + 1, move.new_col }, { move.new_row - 1, move.new_col }, 
    { move.new_row + 1, move.new_col + 1 }, { move.new_row - 1, move.new_col - 1 }, 
    { move.new_row - 1, move.new_col + 1 }, { move.new_row + 1, move.new_col - 1 }, 
    { move.new_row, move.new_col - 1 }, { move.new_row, move.new_col + 1 } };

    //std::cout << "Distance: " << sqrt(pow(row_change, 2) + pow(col_change, 2)) << '\n';
    if (sqrt(pow(row_change, 2) + pow(col_change, 2)) <= sqrt(2)) {
        for (auto coord: moves) {
            if (coord.first >= 0 && coord.first <= 7 && coord.second >= 0 && coord.second <= 7 &&
                board[coord.first][coord.second].piece_occupying && 
                board[coord.first][coord.second].piece_occupying->piece_type == "king" && 
                board[coord.first][coord.second].piece_occupying->colour != move.turn) {
                return -1;
            }
        }
    
        return 0;
    } else if (row_change == 0 && std::abs(col_change) == 2) {
        Chessboard copy {};
        for (int i { 0 }; i < 8; i++) {
            for (int j { 0 }; j < 8; j++) {
                copy[i][j].piece_occupying = board[i][j].piece_occupying;
            }
        }
        //std::cout << "testing..\n";
        return test_castling(game, copy, move);
    }
    return -1;
}

int check_checks(Game &game, Chessboard& copy, Move& move) {
    int test {};
    Coords king_position {};

    //std::cout << "checking checks\n";
    if (move.prev_row != move.new_row || move.prev_col != move.new_col) {
        copy[move.new_row][move.new_col].piece_occupying = nullptr;
        copy[move.new_row][move.new_col].piece_occupying = std::move(copy[move.prev_row][move.prev_col].piece_occupying);
    }
    std::string opposing_colour = ((move.turn == "white") ? "black" : "white");
   
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == move.turn &&
                copy[i][j].piece_occupying->piece_type == "king") {
                king_position.row = i;
                king_position.col = j;
                break;
            }
        }
    }   
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == opposing_colour &&
                copy[i][j].piece_occupying->piece_type != "king") {
          
                Move test_move { i, j, king_position.row, king_position.col, 
                    opposing_colour, copy[i][j].piece_occupying->piece_type };
                test = validate_move(game, copy, test_move, true);
                if (test == 0) {
                    //std::cout << "prev: " << i << ' ' << j << " new: " << new_row << ' ' << new_col << '\n';
                    return 1;
                }
            }
        }
    }
    return 0;
    
}

int test_castling(Game &game, Chessboard& copy, Move& move) {
    int castle_row = ((move.turn == "white") ? 7 : 0);
    bool in_check = ((move.turn == "white") ? game.white_in_check : game.black_in_check);
  
    if (move.prev_row == castle_row && move.prev_col == 4 && !in_check && 
        copy[castle_row][4].piece_occupying->moves == 0) {
        if (move.new_row == castle_row && move.new_col == 6 && copy[castle_row][7].piece_occupying && 
            copy[castle_row][7].piece_occupying->piece_type == "rook" && 
            copy[castle_row][7].piece_occupying->moves == 0) {
            for (int i { 5 }; i < 7; i++) {
                if (copy[castle_row][i].piece_occupying) {
                    return -1;
                } else {
                    Move trial_move { move.prev_row, move.prev_col, move.new_row, i, move.turn, "king" };
                    if (check_checks(game, copy, trial_move) == 1) {
                        //std::cout << "castlefail\n";
                        return -1;
                    }
                }
            }
            return 1;
        } else if (move.new_row == castle_row && move.new_col == 2 && copy[castle_row][0].piece_occupying && 
            copy[castle_row][0].piece_occupying->piece_type == "rook" &&
            copy[castle_row][0].piece_occupying->moves == 0) {
            for (int i { 3 }; i > 1; i--) {
                if (copy[castle_row][i].piece_occupying) {
                    return -1;
                } else {
                    Move trial_move { move.prev_row, move.prev_col, move.new_row, i, move.turn, "king" };
                    if (check_checks(game, copy, trial_move) == 1) {
                        return -1;
                    }
                }
            }
            return 2;
        }
        return -1;
    } 
    return -1;
}

int determine_possible_moves(Game& game) {
    std::string board_positions {};
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying && game.board[i][j].piece_occupying->colour == game.turn) {
                std::string piece = game.board[i][j].piece_occupying->piece_type;
                if (game.board[i][j].piece_occupying->piece_type == "pawn" && game.turn == "white") {
                    std::vector<std::pair<int, int>> possible_moves { { i - 1, j }, 
                    { i - 2, j }, { i - 1, j + 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, game.turn, piece };
                        if (validate_move(game, game.board, move) == 0) {
                            return 0;
                        }
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "pawn" && game.turn == "black") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j }, 
                    { i + 2, j }, { i + 1, j + 1 }, { i + 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, game.turn, piece};
                        if (validate_move(game, game.board, move) == 0) {
                            return 0;
                        }
                    }                   
                } else if (game.board[i][j].piece_occupying->piece_type == "knight") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 2 }, { i + 2, j + 1 }, 
                    { i - 1, j - 2 }, { i - 2, j - 1 }, { i + 1, j - 2 }, 
                    { i - 1, j + 2 }, { i - 2, j + 1 }, { i + 2, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, game.turn, piece };
                        if (validate_move(game, game.board, move) == 0) {
                            return 0;
                        }
                    }  
                } else if (game.board[i][j].piece_occupying->piece_type == "bishop") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j - k, game.turn, piece };
                        Move move2 { i, j, i + k, j + k, game.turn, piece };
                        Move move3 { i, j, i - k, j + k, game.turn, piece };
                        Move move4 { i, j, i - k, j - k, game.turn, piece };
                        std::vector<Move> moves { move1, move2, move3, move4 };
                        for (Move move: moves) {
                            if (validate_move(game, game.board, move) == 0) {
                                return 0;
                            }
                        }
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "rook") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j, game.turn, piece };
                        Move move2 { i, j, i - k, j, game.turn, piece };
                        Move move3 { i, j, i, j + k, game.turn, piece };
                        Move move4 { i, j, i, j - k, game.turn, piece };
                        std::vector<Move> moves { move1, move2, move3, move4 };
                        for (Move move: moves) {
                            if (validate_move(game, game.board, move) == 0) {
                                return 0;
                            }
                        }
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "queen") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j - k, game.turn, piece };
                        Move move2 { i, j, i + k, j + k, game.turn, piece };
                        Move move3 { i, j, i - k, j + k, game.turn, piece };
                        Move move4 { i, j, i - k, j - k, game.turn, piece };  
                        Move move5 { i, j, i + k, j, game.turn, piece };
                        Move move6 { i, j, i - k, j, game.turn, piece };
                        Move move7 { i, j, i, j + k, game.turn, piece };
                        Move move8 { i, j, i, j - k, game.turn, piece };  
                        std::vector<Move> moves { move1, move2, move3, move4, move5, move6, move7, move8 };
                        for (Move move: moves) {
                            if (validate_move(game, game.board, move) == 0) {
                                return 0;
                            }
                        }                 
                    }
                } else if (game.board[i][j].piece_occupying->piece_type == "king") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 1 }, { i + 1, j }, 
                    { i, j + 1 }, { i + 1, j - 1 }, { i - 1, j + 1 }, 
                    { i - 1, j }, { i, j - 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, game.turn, piece };
                        if (validate_move(game, game.board, move) == 0) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

void record_board(Game& game) {
    std::string board_positions {};
    bool black_en_passant { false }, white_en_passant { false };
    bool black_castling { false }, white_castling { false };
    game.value_black_pieces = game.value_white_pieces = 0;
    game.pawns_on_board = false;
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying) {
                board_positions += '[';
                board_positions += std::to_string(i);
                board_positions += std::to_string(j);
                board_positions += game.board[i][j].piece_occupying->piece_type[0];
                board_positions += game.board[i][j].piece_occupying->colour[0];
                board_positions += ']';
                record_piece_points(game, game.board[i][j].piece_occupying->piece_type, 
                    game.board[i][j].piece_occupying->colour);
            }   
            if (game.board[i][j].piece_occupying && game.board[i][j].piece_occupying->piece_type == "pawn") {
                if (i == 3 && !white_en_passant) {
                    Move move1 { i, j, i - 1, j + 1, "white", "pawn" };
                    Move move2 { i, j, i - 1, j - 1, "white", "pawn" };
                    if (validate_move(game, game.board, move1) == 3 || validate_move(game, game.board, move2) == 3) {
                        white_en_passant = true;
                    }
                }
                if (i == 4 && !black_en_passant) {
                    Move move1 { i, j, i + 1, j + 1, "black", "pawn" };
                    Move move2 { i, j, i + 1, j - 1, "black", "pawn" };
                    if (validate_move(game, game.board, move1) == 3 || validate_move(game, game.board, move2) == 3) {
                        black_en_passant = true;
                    }                   
                }
            }
            if (game.board[i][j].piece_occupying && game.board[i][j].piece_occupying->piece_type == "king") {
                if (i == 7 && j == 4 && !white_castling) {
                    Move move1 { i, j, 7, 6, "white", "king" };
                    Move move2 { i, j, 7, 2, "white", "king" };
                    if (validate_move(game, game.board, move1) == 1 || validate_move(game, game.board, move2) == 2) {
                        white_castling = true;
                    }
                }
                if (i == 0 && j == 4 && !black_castling) {
                    Move move1 { i, j, 0, 6, "black", "king" };
                    Move move2 { i, j, 0, 2, "black", "king" };
                    if (validate_move(game, game.board, move1) == 1 || validate_move(game, game.board, move2) == 2) {
                        black_castling = true;
                    }                
                }
            }
        }
    }
    if (white_castling) {
        board_positions += "[wc=t]";
    }
    if (black_castling) {
        board_positions += "[bc=t]";
    }
    if (white_en_passant) {
        board_positions += "[wep=t]";
    }
    if (black_en_passant) {
        board_positions += "[bep=t]";
    }
    //std::cout << board_positions << '\n';
    game.board_record.push_back(board_positions);
}

void record_piece_points(Game& game, std::string piece_type, std::string piece_colour) {
    int points { 0 };
    if (piece_type == "knight" || piece_type == "bishop") {
        points = 3;
    } else if (piece_type == "pawn") {
        points = 1;
        game.pawns_on_board = true;
    } else if (piece_type == "rook") {
        points = 5;
    } else if (piece_type == "queen") {
        points = 9;
    }
    if (piece_colour == "white") {
        game.value_white_pieces += points;
    } else {
        game.value_black_pieces += points;
    }
}





