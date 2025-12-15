#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <utility>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <atomic>

constexpr int SQUARE_SIZE = 95;
int positions_searched { 0 };

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
    int prev_row { 0 };
    int prev_col { 0 };
    int new_row { 0 };
    int new_col { 0 };
    std::string turn {};
    std::string piece {};
    std::shared_ptr<Piece> piece_taken { nullptr };
    std::string special_move { "No" };
    int eval { 0 };
};

Move calculated_move;
std::atomic<bool> computer_turn { false };
std::atomic<bool> thinking_in_progress { false };
std::atomic<bool> finished { false };

using Chessboard = std::array<std::array<Cell, 8>, 8>;

enum class Gamestate {
    Intro,
    Playing,
    Promoting_pawn,
    Gameover,
    Resetting
};

enum class Gamemode {
    CPUwhite,
    CPUblack,
    Twoplayer
};

class Game {
    public:
        //std::vector<Piece> pieces{32}; 
        Gamestate state {};
        Gamemode mode { Gamemode::Twoplayer };
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
        std::string view { "white" };
        std::vector<Move> possible_moves {};
        Move calculated_move {};
        bool move_ready { false };
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
void draw_intro_screen(sf::RenderWindow& window, Game& game);
void draw_board(Game& game, sf::RenderWindow& window);
void draw_piece(Game& game, sf::RenderWindow& window, std::shared_ptr<Piece>& piece);
void draw_reset_button(sf::RenderWindow& window);
void draw_undo_button(sf::RenderWindow& window);
void draw_end_screen(Game& game, sf::RenderWindow& window);
void draw_pawn_promotion_screen(Game& game, sf::RenderWindow& window);
void draw_return_to_home_button(sf::RenderWindow& window);
sf::Text configure_text(const sf::Font& font, const std::string& string, sf::Vector2f pos, 
    int size, const std::string& colour);
sf::RectangleShape make_rectangle(sf::Vector2f pos, sf::Vector2f size, const std::string& colour);
int select_square(int x, int y, Game& game);
bool select_pawn_promotion(Game& game, sf::Vector2i mouse_pos);
void process_move(Game& game, Chessboard& board, int result, Move move, bool CPU_testing = false);
void make_move(Game& game, Chessboard& board, int result, Move& move);
void move_piece(Chessboard& board, int prev_row, int prev_col, int new_row, int new_col, bool undo = false);
std::vector<Move> determine_possible_moves(Game& game, Chessboard& board, std::string turn, bool CPU = false);
int determine_repetition(Game& game);
int determine_insufficient_material(Game& game);
void end_game(Game& game);
void record_board(Game& game);
void is_game_over(Game& game);
void undo_move(Game& game);
void undo_test_move(Game& game, Chessboard& board, Move& prev_move);
void generate_computer_move(Game& game);

void handle_input(Game& game, sf::RenderWindow& window);
void handle_clicks_intro(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos);
void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos);
void handle_clicks_promoting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_resetting(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos);
void handle_clicks_returning(Game& game, sf::Vector2i mouse_pos);

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
void handle_pawn_promotion(Game& game, Chessboard& board, Move& move);
int validate_en_passant(Game& game, Chessboard& board, Move& move);
void record_piece_points(Game& game, std::string piece_type, std::string piece_colour);

Move get_best_move(Game& game, Chessboard& copy, int depth);
int minimax(Game& game, Chessboard& board, int depth, bool maximising);
int evaluate(Chessboard& board);
void update_computer_move(Game& game);


std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << "Prev row and col: " << move.prev_row << ' ' << move.prev_col << '\n';
    os << "New row and col: " << move.new_row << ' ' << move.new_col << '\n';
    return os;
}

int main() {
    run_game_loop();
}

void run_game_loop() {
    Game game {};
    game.state = Gamestate::Intro;
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Chess");
    while (window.isOpen()) {
        if ((game.mode == Gamemode::CPUwhite && game.state == Gamestate::Playing && game.turn == "white") ||
            (game.mode == Gamemode::CPUblack && game.state == Gamestate::Playing && game.turn == "black")) {
            
            if (!finished) {
                generate_computer_move(game);
            } else {
                std::cout << "Searched: " << positions_searched << '\n';
                update_computer_move(game);
            }
        } 
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
            if (game.state == Gamestate::Intro) {
                handle_clicks_intro(game, window, mouse_press->position);
            } else if (game.state == Gamestate::Playing && game.mode == Gamemode::Twoplayer) {
               
                handle_clicks_playing(game, window, mouse_press->position);
                handle_clicks_undoing(game, mouse_press->position);
            } else if (game.mode == Gamemode::CPUwhite && game.state == Gamestate::Playing) {
                if (game.turn == "black") {
                    handle_clicks_playing(game, window, mouse_press->position);
                    handle_clicks_undoing(game, mouse_press->position);                     
                } 
            } else if (game.mode == Gamemode::CPUblack && game.state == Gamestate::Playing) {
                if (game.turn == "white") {
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
        game.view = "white";
        game.mode = Gamemode::CPUblack;
    } else if (400 <= x && x <= 650 && 530 <= y && y <= 640) {
        game.view = "black";
        game.mode = Gamemode::CPUwhite;
    } else if (690 <= x && x <= 940 && 530 <= y && y <= 640) {
        game.view = "white";
        game.mode = Gamemode::Twoplayer;
    }
}

void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos) {
    int result = select_square(mouse_pos.x, mouse_pos.y, game);

    if (result >= 0) {
        /*std::cout << game.current_move.prev_row << ' ' << game.current_move.prev_col << 
        ' ' << game.current_move.new_row << ' ' << game.current_move.new_col << '\n';*/
        process_move(game, game.board, result, game.current_move);  
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
    if (10 <= mouse_pos.x && mouse_pos.x <= 54 && 10 <= mouse_pos.y && mouse_pos.y <= 45) {
        game.initialise();
        return;
    }
}

void handle_clicks_undoing(Game& game, sf::Vector2i mouse_pos) {
    if (950 <= mouse_pos.x && mouse_pos.x <= 994 && 10 <= mouse_pos.y && mouse_pos.y <= 45) {
        if (game.mode == Gamemode::Twoplayer) {
            undo_move(game);
        } else {
            undo_move(game);
            undo_move(game);
        }
    }
}

void handle_clicks_returning(Game& game, sf::Vector2i mouse_pos) {
    if (10 <= mouse_pos.x && mouse_pos.x <= 114 && 55 <= mouse_pos.y && mouse_pos.y <= 90) {
        game.state = Gamestate::Intro;
    }
}

void undo_move(Game& game) {
    if (game.move_record.size() == 0) {
        return;
    }
    //std::cout << "size: " << game.move_record.size() << '\n';
    Move prev_move = game.move_record[game.move_record.size() - 1];
    std::string turn = (((game.move_record.size() - 1) % 2 == 0) ? "black" : "white"); 

    move_piece(game.board, prev_move.new_row, prev_move.new_col, prev_move.prev_row,
    prev_move.prev_col, true);
    if (prev_move.special_move != "en passant" && prev_move.piece_taken != nullptr) {
        game.board[prev_move.new_row][prev_move.new_col].piece_occupying = prev_move.piece_taken;
    }
    int captured_row = ((turn == "black") ? prev_move.new_row + 1 : prev_move.new_row - 1);
    if (prev_move.special_move == "en passant") {
        game.board[captured_row][prev_move.new_col].piece_occupying = prev_move.piece_taken;
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
    if (prev_move.piece_taken == nullptr && prev_move.piece != "pawn") {
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

void undo_test_move(Game& game, Chessboard& board, Move& prev_move) {

    std::string turn = ((prev_move.turn == "black") ? "white" : "black");

    move_piece(board, prev_move.new_row, prev_move.new_col, prev_move.prev_row,
    prev_move.prev_col, true);
    if (prev_move.special_move != "en passant" && prev_move.piece_taken != nullptr) {
        board[prev_move.new_row][prev_move.new_col].piece_occupying = prev_move.piece_taken;
    }
    int captured_row = ((turn == "black") ? prev_move.new_row + 1 : prev_move.new_row - 1);
    if (prev_move.special_move == "en passant") {
        board[captured_row][prev_move.new_col].piece_occupying = prev_move.piece_taken;
    }
    int castling_row = ((turn == "black") ? 7 : 0);
    if (prev_move.special_move == "castling") {
        if (prev_move.new_col - prev_move.prev_col == 2) {
            move_piece(board, castling_row, 5, castling_row, 7, true);
        } else {
            move_piece(board, castling_row, 3, castling_row, 0, true);
        }
    }
    if (prev_move.special_move == "promotion") {
        board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type = "pawn";
    }
    game.turn = ((game.turn == "black") ? "white" : "black");
    //std::cout << game.board[prev_move.prev_row][prev_move.prev_col].piece_occupying->piece_type << '\n';

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
                if (game.view == "white") {
                    cell.setFillColor(sf::Color(165, 42, 42));
                } else {
                    cell.setFillColor(sf::Color::Yellow);
                }
            } else {
                if (game.view == "white") {
                    cell.setFillColor(sf::Color::Yellow);
                } else {
                    cell.setFillColor(sf::Color(165, 42, 42));
                }
            }
            if (game.board[i][j].selected && game.view == "white") {
                //std::cout << i << " " << j << '\n';
                //std::cout << x_offset << " " << y_offset << '\n';
                cell.setOutlineThickness(-3.0f);
                cell.setOutlineColor(sf::Color::Black);
            } else if (game.board[7 - i][j].selected && game.view == "black") {
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
   
    sf::Font font;
    if (!font.openFromFile("./assets/fonts/Roboto-SemiBold.ttf")) {
        return;
    }
    sf::Text text(font);
    text.setFillColor(sf::Color::Red);
    text.setCharacterSize(40);
    text.setPosition({280, 280}); 

    if (game.checkmate) {
        if (game.winner == "white") {
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
    int row = ((game.view == "white") ? 4 : 3);
    std::shared_ptr<Piece> piece1 = std::make_shared<Piece> ("rook", game.turn, false, row, 2);
    std::shared_ptr<Piece> piece2 = std::make_shared<Piece> ("knight", game.turn, false, row, 3);
    std::shared_ptr<Piece> piece3 = std::make_shared<Piece> ("bishop", game.turn, false, row, 4);
    std::shared_ptr<Piece> piece4 = std::make_shared<Piece> ("queen", game.turn, false, row, 5);
    
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
        if (!texture.loadFromFile("./assets/images/Chess_pdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "pawn" && piece->colour == "white") {
        if (!texture.loadFromFile("./assets/images/Chess_plt45.png")) {
            return;
        }
    } else if (piece->piece_type == "knight" && piece->colour == "black") {
        if (!texture.loadFromFile("./assets/images/Chess_ndt45.png")) {
            return;
        }
    } else if (piece->piece_type == "knight" && piece->colour == "white") {
        if (!texture.loadFromFile("./assets/images/Chess_nlt45.png")) {
            return;
        }
    }  else if (piece->piece_type == "bishop" && piece->colour == "black") {
        if (!texture.loadFromFile("./assets/images/Chess_bdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "bishop" && piece->colour == "white") {
        if (!texture.loadFromFile("./assets/images/Chess_blt45.png")) {
            return;
        }
    } else if (piece->piece_type == "rook" && piece->colour == "black") {
        if (!texture.loadFromFile("./assets/images/Chess_rdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "rook" && piece->colour == "white") {
        if (!texture.loadFromFile("./assets/images/Chess_rlt45.png")) {
            return;
        }
    } else if (piece->piece_type == "queen" && piece->colour == "black") {
        if (!texture.loadFromFile("./assets/images/Chess_qdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "queen" && piece->colour == "white") {
        if (!texture.loadFromFile("./assets/images/Chess_qlt45.png")) {
            return;
        }
    } else if (piece->piece_type == "king" && piece->colour == "black") {
        if (!texture.loadFromFile("./assets/images/Chess_kdt45.png")) {
            return;
        }
    } else if (piece->piece_type == "king" && piece->colour == "white") {
        if (!texture.loadFromFile("./assets/images/Chess_klt45.png")) {
            return;
        }
    }

    sf::Sprite sprite(texture);
    sprite.setScale({0.1f, 0.1f});
    float y_offset;
    float x_offset { static_cast<float>(135 + piece->col * (SQUARE_SIZE)) };
    if (game.view == "white") {
        y_offset = (30 + piece->row * (SQUARE_SIZE));
    } else {
        y_offset = (695 - piece->row * (SQUARE_SIZE));
    }
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
    if (game.view == "black") {
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

void process_move(Game& game, Chessboard& board, int result, Move move, bool CPU) {

    
    //std::cout << "moving\n";
    if (move.piece == "pawn" || board[move.new_row][move.new_col].piece_occupying) {
        game.plys_to_100 = 0;
    } else {
        game.plys_to_100++;
    }
    int row = move.prev_row, col = move.prev_col;

    if ((game.turn == "black" && board[row][col].piece_occupying->piece_type == "pawn" && row == 6) || 
        (game.turn == "white" && board[row][col].piece_occupying->piece_type == "pawn" && row == 1) &&
        std::abs(move.new_row - move.prev_row) == 1) {
    
        if (!CPU) {
            game.promoting_pawn = true;
            return;
        } else {
            std::cout << "promoting for computer\n";
            move_piece(board, move.prev_row, move.prev_col, move.new_row, move.new_col);
            move.special_move = "promotion";
            board[move.new_row][move.new_col].piece_occupying->piece_type = "queen";
            game.move_record.push_back(move);
            return;    
        }
    }
    //std::cout << "plys to 100: " << game.plys_to_100 << '\n';
    if (board[move.prev_row][move.prev_col].piece_occupying == nullptr) {
        std::cout << "error before\n";
    }
    if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
        move_piece(board, move.prev_row, move.prev_col, move.new_row, move.new_col);
    }   
    if (board[move.new_row][move.new_col].piece_occupying == nullptr) {
        std::cout << "error after\n";
    }

    if (result > 0 && result < 3) {
        if (result == 1 && game.turn == "white") { 
            move_piece(board, 7, 7, 7, 5);
        } else if (result == 2 && game.turn == "white") {
            move_piece(board, 7, 0, 7, 3);
        } else if (result == 1 && game.turn == "black") {
            move_piece(board, 0, 7, 0, 5);
        } else if (result == 2 && game.turn == "black") {
            move_piece(board, 0, 0, 0, 3);
        }

        game.board_record.clear();
        move.special_move = "castling";
    } else if (result == 3) {
        int captured_row = ((game.turn == "white") ? move.new_row + 1 : move.new_row - 1);
        move.special_move = "en passant";
        move.piece_taken = board[captured_row][move.new_col].piece_occupying;
        board[captured_row][move.new_col].piece_occupying = nullptr;
        
        game.board_record.clear();
        
    } 
   
    game.move_record.push_back(move);
    


    if (board[move.new_row][move.new_col].piece_occupying->piece_type == "king") {
        if (game.turn == "white") {
            game.white_king_position.row = move.new_row;
            game.white_king_position.col = move.new_col;
        } else {
            game.black_king_position.row = move.new_row;
            game.black_king_position.col = move.new_col;
        }
    }
}

void make_move(Game& game, Chessboard& board, int result, Move& move) {
    //std::cout << "processing move\n";
    if (board[move.prev_row][move.prev_col].piece_occupying == nullptr) {
        std::cout << "error before\n";
    }
    //std::cout << move.prev_row << ' ' << move.prev_col << ' ' << move.new_row << ' ' << move.new_col << '\n';
 
    int row = move.prev_row, col = move.prev_col;
    if (!board[row][col].piece_occupying) {
        std::cout << "failed here\n";
    }
    if ((move.turn == "black" && board[row][col].piece_occupying->piece_type == "pawn" && row == 6) || 
        (move.turn == "white" && board[row][col].piece_occupying->piece_type == "pawn" && row == 1) &&
        std::abs(move.new_row - move.prev_row) == 1) {
        if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
            move_piece(board, move.prev_row, move.prev_col, move.new_row, move.new_col);
            move.special_move = "promotion";
            board[move.new_row][move.new_col].piece_occupying->piece_type = "queen";
        }
        return;
    }
    if (move.prev_row != move.new_row || move.prev_col != move.new_col ) {
        move_piece(board, move.prev_row, move.prev_col, move.new_row, move.new_col);
    }  

 
    if (board[move.new_row][move.new_col].piece_occupying == nullptr) {
        std::cout << "error after\n";
    }
    //std::cout << "reached here\n";

    if (result > 0 && result < 3) {
        if (result == 1 && move.turn == "white") { 
            move_piece(board, 7, 7, 7, 5);
        } else if (result == 2 && move.turn == "white") {
            move_piece(board, 7, 0, 7, 3);
        } else if (result == 1 && move.turn == "black") {
            move_piece(board, 0, 7, 0, 5);
        } else if (result == 2 && move.turn == "black") {
            move_piece(board, 0, 0, 0, 3);
        }
        move.special_move = "castling";
    } else if (result == 3) {
        int captured_row = ((game.turn == "white") ? move.new_row + 1 : move.new_row - 1);
        move.special_move = "en passant";
        move.piece_taken = board[captured_row][move.new_col].piece_occupying;
        board[captured_row][move.new_col].piece_occupying = nullptr;

    } 
    game.turn = ((game.turn == "black") ? "white" : "black");

}

void generate_computer_move(Game& game) {
    game.move_ready = false;
    if (thinking_in_progress) {
        return;
    }
    thinking_in_progress = true;

    auto copy = std::make_shared<Chessboard>();

    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (game.board[i][j].piece_occupying) {
                (*copy)[i][j].piece_occupying = std::make_shared<Piece>(*game.board[i][j].piece_occupying);
            } else {
                (*copy)[i][j].piece_occupying = nullptr;
            }
        }
    }
    positions_searched = 0;
    Game game_copy = game;
    std::thread computer_thread([copy, game_copy]() mutable {
        Move chosen_move = get_best_move(game_copy, *copy, 4);
        computer_turn = false;
        thinking_in_progress = false;
        finished = true;
        calculated_move = chosen_move;
    });
    computer_thread.detach();


}

void update_computer_move(Game& game) {
    if (finished) {
        Move chosen_move = calculated_move;
        std::cout << "best moved calculated\n";
        std::cout << chosen_move.prev_row << ' ' << chosen_move.prev_col << ' ' 
        << chosen_move.new_row << ' ' << chosen_move.new_col << '\n';
        //std::cout << "nr: " << chosen_move.new_row << "nc: " << chosen_move.new_col << " piece:" << chosen_move.piece << '\n';
        if (game.board[chosen_move.new_row][chosen_move.new_col].piece_occupying) {
            chosen_move.piece_taken = game.board[chosen_move.new_row][chosen_move.new_col].piece_occupying;
        }
        int result = validate_move(game, game.board, chosen_move);
        process_move(game, game.board, result, chosen_move, true);

        game.turn = ((game.turn == "white") ? "black" : "white");
        is_game_over(game);
    }
    game.move_ready = false;
    finished = false;
}

Move get_best_move(Game& game, Chessboard& copy, int depth) {
    int best_score = (game.turn == "white") ? -500000 : 500000;
    Move best_move {};
    std::string turn = game.turn;
    //std::cout << "generating\n";
    std::vector<Move> possible_moves = determine_possible_moves(game, copy, turn, true);
    //std::cout << "here2\n";

    for (auto move: possible_moves) {
        if (copy[move.new_row][move.new_col].piece_occupying) {
            move.piece_taken = copy[move.new_row][move.new_col].piece_occupying;
        }

        int result = validate_move(game, copy, move);
        if (result >= 0) {
            make_move(game, copy, result, move);
            //std::cout << "here3\n";
            bool maximising = ((turn == "white") ? true : false);
            
            int move_eval = minimax(game, copy, depth - 1, !maximising);
            //std::cout << "e: " << move_eval << '\n';
            if (turn == "white") {
                if (move_eval > best_score) {
                    best_score = move_eval;
                    best_move = move;
                }
            } else {
                if (move_eval < best_score) {
                    best_score = move_eval;
                    best_move = move;
                }
            }
            undo_test_move(game, copy, move);
            turn = game.turn;
        }
    }
    return best_move;
}

int minimax(Game& game, Chessboard& board, int depth, bool maximising) {
    //std::cout << "minimaxing\n";
    if (depth == 0) {
        //std::cout << "reached depth 0\n";
        positions_searched++;
        return evaluate(board);
    }
    //std::cout << "depth: " << depth << '\n';
    std::string turn = ((maximising == true) ? "white" : "black");

    std::vector<Move> possible_moves = determine_possible_moves(game, board, turn, true);
    //std::cout << "size: " << possible_moves.size() << '\n';
    if (possible_moves.size() == 0) {
        if (maximising) {
            return -600000;
        } else {
            return 600000;
        }
    }
    if (maximising && turn == "white") {
        int max_eval = -500000;
        
        for (Move possible_move: possible_moves) {

            if (board[possible_move.new_row][possible_move.new_col].piece_occupying) {
                possible_move.piece_taken = board[possible_move.new_row][possible_move.new_col].piece_occupying;
            }
            //game.turn = "white";
            int result = validate_move(game, board, possible_move);
            if (result >= 0) {
                make_move(game, board, result, possible_move);
    
                //turn = ((turn == "white") ? "black" : "white");
                int eval = minimax(game, board, depth - 1, false);
                turn = ((turn == "white") ? "black" : "white");
                undo_test_move(game, board, possible_move);
                
                max_eval = std::max(eval, max_eval);
            }
        }
        return max_eval;
    } else if (!maximising && turn == "black") {
        //game.turn = "black";
        int min_eval = 500000;
        for (Move possible_move: possible_moves) {
            if (board[possible_move.new_row][possible_move.new_col].piece_occupying) {
                possible_move.piece_taken = board[possible_move.new_row][possible_move.new_col].piece_occupying;
            }
            int result = validate_move(game, board, possible_move);
            if (result >= 0) {
                make_move(game, board, result, possible_move);

                //turn = ((turn == "white") ? "black" : "white");
                int eval = minimax(game, board, depth - 1, true);
                //turn = ((turn == "white") ? "black" : "white");
                undo_test_move(game, board, possible_move);
                
                min_eval = std::min(eval, min_eval);
            }
        } 

        return min_eval;    
    } else {
        std::cout << "a. bug occurred\n";
    }
}

int evaluate(Chessboard& board) {
    int eval { 0 };
    int points { 0 };
    int pawns, knight_bishops, rooks, queens;
    pawns = knight_bishops = rooks = queens = 0;
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (board[i][j].piece_occupying) {
                if (board[i][j].piece_occupying->piece_type == "pawn") {
                    points = 1;
                    pawns++;
                } else if (board[i][j].piece_occupying->piece_type == "knight" || 
                    board[i][j].piece_occupying->piece_type == "bishop") {
                    points = 3;
                    knight_bishops++;
                } else if (board[i][j].piece_occupying->piece_type == "rook") {
                    points = 5;
                    rooks++;
                } else if (board[i][j].piece_occupying->piece_type == "queen") {
                    points = 9;
                    queens++;
                }
                if (board[i][j].piece_occupying->colour == "white") {
                    eval += points;
                } else {
                    eval -= points;
                }
            }
        }
    }
    //std::cout << pawns << ' ' << knight_bishops << ' ' << rooks << ' ' << queens << '\n';
    //std::cout << "eval: " << eval << '\n';
    return eval;
}

void is_game_over(Game& game) {
    evaluate_king_checks(game);
    record_board(game);
    std::vector<Move> moves = determine_possible_moves(game, game.board, game.turn);
    if (moves.size() == 0 || determine_repetition(game) == -1 || 
        determine_insufficient_material(game) == -1 || game.plys_to_100 == 100) {
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
        handle_pawn_promotion(game, game.board, game.current_move);
        return true;
    }
    return false;
}

void handle_pawn_promotion(Game& game, Chessboard& board, Move& move) {
    move_piece(board, move.prev_row, move.prev_col, 
        move.new_row, move.new_col);
    move.special_move = "promotion";
    game.move_record.push_back(move);
    board[move.new_row][move.new_col].piece_occupying->piece_type = game.piece_selected;
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
    //std::cout << board[prev_row][prev_col].piece_occupying->piece_type << '\n';
    board[new_row][new_col].piece_occupying = std::move(board[prev_row][prev_col].piece_occupying);
    if (board[new_row][new_col].piece_occupying == nullptr) {
        std::cout << prev_row << ' ' << prev_col << '\n';
        std::cout << new_row << ' ' << new_col << '\n';
        std::cout << "failed\n";
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
    //std::cout << move.prev_row << ' ' << move.prev_col << '\n';
    //std::cout << move.new_row << ' ' << move.new_col << '\n';
    //std::cout << std::boolalpha << (move.prev_row != move.new_row) || (move.prev_col != move.new_col) << '\n';
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
        //std::cout << "ruledinvalid\n";
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
  
    if (validate_move_rook(board, move) == 0 || validate_move_bishop(board, move) == 0) {
        return 0;
    } 
    return -1;
}

int validate_move_king(Game &game, Chessboard& board, Move& move) {

    //std::cout << "Distance: " << sqrt(pow(row_change, 2) + pow(col_change, 2)) << '\n';
    if (sqrt(pow(move.new_row - move.prev_row, 2) + pow(move.new_col - move.prev_col, 2)) <= sqrt(2)) {
        return 0;
    } else if (move.new_row - move.prev_row == 0 && std::abs(move.new_col - move.prev_col) == 2) {
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
            if (copy[i][j].piece_occupying && copy[i][j].piece_occupying->colour == opposing_colour) {
          
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

std::vector<Move> determine_possible_moves(Game& game, Chessboard& board, std::string turn, bool CPU) {
    //std::string board_positions {};
    std::vector<Move> moves {};
    for (int i { 0 }; i < 8; i++) {
        for (int j { 0 }; j < 8; j++) {
            if (board[i][j].piece_occupying && board[i][j].piece_occupying->colour == turn) {
                std::string piece = board[i][j].piece_occupying->piece_type;
                if (board[i][j].piece_occupying->piece_type == "pawn" && turn == "white") {
                    std::vector<std::pair<int, int>> possible_moves { { i - 1, j }, 
                    { i - 2, j }, { i - 1, j + 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece };
                        if (validate_move(game, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == "pawn" && turn == "black") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j }, 
                    { i + 2, j }, { i + 1, j + 1 }, { i + 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece};
                        if (validate_move(game, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }                   
                } else if (board[i][j].piece_occupying->piece_type == "knight") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 2 }, { i + 2, j + 1 }, 
                    { i - 1, j - 2 }, { i - 2, j - 1 }, { i + 1, j - 2 }, 
                    { i - 1, j + 2 }, { i - 2, j + 1 }, { i + 2, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece };
                        if (validate_move(game, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }  
                } else if (board[i][j].piece_occupying->piece_type == "bishop") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j - k, turn, piece };
                        Move move2 { i, j, i + k, j + k, turn, piece };
                        Move move3 { i, j, i - k, j + k, turn, piece };
                        Move move4 { i, j, i - k, j - k, turn, piece };
                        std::vector<Move> candidates { move1, move2, move3, move4 };
                        for (Move move: candidates) {
                            if (validate_move(game, board, move) >= 0) {
                                moves.push_back(move);
                                if (!CPU) {
                                    return moves;
                                }
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == "rook") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j, turn, piece };
                        Move move2 { i, j, i - k, j, turn, piece };
                        Move move3 { i, j, i, j + k, turn, piece };
                        Move move4 { i, j, i, j - k, turn, piece };
                        std::vector<Move> candidates { move1, move2, move3, move4 };
                        for (Move move: candidates) {
                            if (validate_move(game, board, move) >= 0) {
                                moves.push_back(move);
                                if (!CPU) {
                                    return moves;
                                }
                            }
                        }
                    }
                } else if (board[i][j].piece_occupying->piece_type == "queen") {
                    for (int k { 0 }; k < 8; k++) {
                        Move move1 { i, j, i + k, j - k, turn, piece };
                        Move move2 { i, j, i + k, j + k, turn, piece };
                        Move move3 { i, j, i - k, j + k, turn, piece };
                        Move move4 { i, j, i - k, j - k, turn, piece };  
                        Move move5 { i, j, i + k, j, turn, piece };
                        Move move6 { i, j, i - k, j, turn, piece };
                        Move move7 { i, j, i, j + k, turn, piece };
                        Move move8 { i, j, i, j - k, turn, piece };  
                        std::vector<Move> candidates { move1, move2, move3, move4, move5, move6, move7, move8 };
                        for (Move move: candidates) {
                            if (validate_move(game, board, move) >= 0) {
                                moves.push_back(move);
                                if (!CPU) {
                                    return moves;
                                }
                            }
                        }                 
                    }
                } else if (board[i][j].piece_occupying->piece_type == "king") {
                    std::vector<std::pair<int, int>> possible_moves { { i + 1, j + 1 }, { i + 1, j }, 
                    { i, j + 1 }, { i + 1, j - 1 }, { i - 1, j + 1 }, 
                    { i - 1, j }, { i, j - 1 }, { i - 1, j - 1 } };
                    for (auto pair: possible_moves) {
                        Move move { i, j, pair.first, pair.second, turn, piece };
                        if (validate_move(game, board, move) >= 0) {
                            moves.push_back(move);
                            if (!CPU) {
                                return moves;
                            }
                        }
                    }
                }
            }
        }
    }
    return moves;
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





