#pragma once
#include "components.h"

constexpr int TIME_Y_POS = 645;
constexpr int TIME_BASE_X_POS = 218;
constexpr int TEXT_BASE_X_POS = 237;
constexpr int TIME_OFFSET = 70;
constexpr int TEXT_OFFSET = 3;
constexpr int TIME_TEXT_Y_POS = TIME_Y_POS + TEXT_OFFSET;
constexpr sf::Vector2f TIME_BUTTON_SIZE = {64, 35};

constexpr sf::Vector2f MODE_BUTTON_SIZE = {250, 110};

constexpr int NO_SQUARE_SELECTED = -1;
constexpr int OUT_OF_BOUNDS = -2;

const std::map<int, std::string> piece_images =  {
    {BLACK_PAWN, "./assets/images/Chess_pdt45.png"}, 
    {WHITE_PAWN, "./assets/images/Chess_plt45.png"},
    {BLACK_KNIGHT, "./assets/images/Chess_ndt45.png"}, 
    {WHITE_KNIGHT, "./assets/images/Chess_nlt45.png"}, 
    {BLACK_BISHOP, "./assets/images/Chess_bdt45.png"},
    {WHITE_BISHOP, "./assets/images/Chess_blt45.png"},
    {BLACK_ROOK, "./assets/images/Chess_rdt45.png"},
    {WHITE_ROOK, "./assets/images/Chess_rlt45.png"},
    {BLACK_QUEEN, "./assets/images/Chess_qdt45.png"},
    {WHITE_QUEEN, "./assets/images/Chess_qlt45.png"},
    {BLACK_KING, "./assets/images/Chess_kdt45.png"},
    {WHITE_KING, "./assets/images/Chess_klt45.png"}
};

/**
 * @class Assets 
 * @brief contains all fonts, sounds, buttons and images used within the game. 
 */
struct Assets {
    sf::Font font;
    sf::Font font2;
    sf::SoundBuffer move_sound_buffer;
    sf::SoundBuffer capture_sound_buffer;
    std::optional<sf::Sound> move_sound;
    std::optional<sf::Sound> capture_sound;
    sf::Texture array[16];
    sf::Vector2f current_mouse_pos;
    bool allow_takebacks { true };
    bool sound_on { false };
    
    /**
     * @brief loads all pictures and sounds into the game
     */
    Assets();

    Button play_black_cpu {font, "Play CPU as\n WHITE", {155, 525}, 30, sf::Color::Black, 
        {135, 515}, MODE_BUTTON_SIZE, sf::Color::White};
    Button play_white_cpu {font, "Play CPU as\n BLACK", {445, 525}, 30, sf::Color::Black, 
        {425, 515}, MODE_BUTTON_SIZE, sf::Color::White};
    Button play_two_player {font, "Two player", {735, 525}, 30, sf::Color::Black, 
        {715, 515}, MODE_BUTTON_SIZE, sf::Color::White};
    Button play_button {font, "Play", {500, 380}, 60, sf::Color::Black, 
        {380, 330}, {350, 160}, sf::Color::White};

    std::vector<Button*> gamemode_buttons {&play_black_cpu, &play_white_cpu, &play_two_player};

    Button untimed_button {font, "Untimed", {TEXT_BASE_X_POS - 15, TIME_TEXT_Y_POS}, 15, sf::Color::Black, 
        {TIME_BASE_X_POS, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button bullet0_button {font, "1|0", {TEXT_BASE_X_POS + TIME_OFFSET, TIME_TEXT_Y_POS}, 15, sf::Color::Black, 
        {TIME_BASE_X_POS + TIME_OFFSET, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button bullet1_button {font, "1|1", {TEXT_BASE_X_POS + TIME_OFFSET * 2, TIME_TEXT_Y_POS}, 15, sf::Color::Black, 
        {TIME_BASE_X_POS + TIME_OFFSET * 2, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button bullet2_button {font, "2|1", {TEXT_BASE_X_POS + TIME_OFFSET * 3, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 3, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button blitz0_button {font, "3|0", {TEXT_BASE_X_POS + TIME_OFFSET * 4, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 4, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button blitz1_button {font, "3|2", {TEXT_BASE_X_POS + TIME_OFFSET * 5, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 5, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button blitz2_button {font, "5|0", {TEXT_BASE_X_POS + TIME_OFFSET * 6, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 6, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button rapid0_button {font, "10|0", {TEXT_BASE_X_POS + TIME_OFFSET * 7 - 5, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 7, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button rapid1_button {font, "15|10", {TEXT_BASE_X_POS + TIME_OFFSET * 8 - 5, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 8, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};
    Button rapid2_button {font, "30|0", {TEXT_BASE_X_POS + TIME_OFFSET * 9 - 5, TIME_TEXT_Y_POS}, 15, sf::Color::Black,
        {TIME_BASE_X_POS + TIME_OFFSET * 9, TIME_Y_POS}, TIME_BUTTON_SIZE, sf::Color::White};

    std::vector<Button*> time_control_choices {&untimed_button, &bullet0_button, &bullet1_button, 
    &bullet2_button, &blitz0_button, &blitz1_button, &blitz2_button, &rapid0_button, &rapid1_button, 
    &rapid2_button};


    Button flip_view_button {font, "Flip view", {13, 148}, 15, sf::Color::Red, 
        {10, 145}, {74, 35}, sf::Color::White};
    Button home_button {font, "Back to Home", {13, 58}, 15, sf::Color::Red, 
        {10, 55}, {104, 35}, sf::Color::White};
    Button undo_button {font, "Undo", {13, 103}, 15, sf::Color::Red, 
        {10, 100}, {44, 35}, sf::Color::White};
    Button reset_button {font, "Reset", {13, 103}, 15, sf::Color::Red, 
        {10, 100}, {44, 35}, sf::Color::White};
    Button toggle_takebacks {font, "Allow Takebacks: Yes", {853, 753}, 15, sf::Color::Black, 
        {850, 750}, {157, 35}, sf::Color::Green};
    Button make_pgn_file {font, "Make PGN file", {13, 728}, 15, sf::Color::Red, {10, 725}, {104, 35}, sf::Color::White};
    Button toggle_audio {font, "Sound: Off", {13, 13}, 15, sf::Color::Black, {10, 10}, {104, 35}, sf::Color::Red};
    Button go_back_button {font, "<-", {920, 730}, 40, sf::Color::Black, {900, 730}, {85, 50}, sf::Color::White};
    Button go_forward_button {font, "->", {1015, 730}, 40, sf::Color::Black, {995, 730}, {85, 50}, sf::Color::White};
    TextBox fen_input {font, {60, 690}, {930, 50}, {70, 700}, 15};
    Screen pawn_promotion_screen {font, "Choose Promotion Piece", {280, 280}, 40, sf::Color::White, 
        {250, 250}, {500, 300}, sf::Color::Blue};
    Screen end_screen {font, "", {280, 280}, 40, sf::Color::Red, {250, 250}, {500, 300}, sf::Color::Black};
    Screen black_clock {font, "10:00", {16, 335}, 20, sf::Color::Black, {10, 318}, {90, 65}, sf::Color::White};
    Screen white_clock {font, "10:00", {16, 425}, 20, sf::Color::Black, {10, 408}, {90, 65}, sf::Color::White};
};

/**
 * @brief renders the screen and all its components (board, buttons, move log)
 * @param game class containing all game variables/classes
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 */
void render(Game& game, sf::RenderWindow& window, Assets& assets);

/**
 * @brief renders the initial screen
 * @param window serves as target for 2D drawing
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 * @param mouse_pos the current cursor position on the screen
 */
void draw_intro_screen(sf::RenderWindow& window, Game& game, Assets& assets, sf::Vector2i& mouse_pos);

void draw_game_mode_buttons(sf::RenderWindow& window, Game& game, Assets& assets, sf::Vector2i& mouse_pos);

void draw_time_control_buttons(sf::RenderWindow& window, Game& game, Assets& assets, sf::Vector2i& mouse_pos);

/**
 * @brief draws the chessboard
 * @param window serves as target for 2D drawing
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 */
void draw_board(Game& game, sf::RenderWindow& window, Assets& assets);

/**
 * @brief draws the piece at the appropriate location
 * @param position class containing variables concerning the chess game
 * @param ui class containing variables regarding user interaction with the chess pieces
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 * @param x the rank of the piece
 * @param y the file of the piece
 * @param piece the piece being drawn
 * @param dragging boolean flag which is true if the piece is being dragged and false otherwise
 */
void draw_piece(Position& position, UI& ui, sf::RenderWindow& window, Assets& assets, 
    int x, int y, uint8_t piece, bool dragging = false);

/**
 * @brief draws the pop-up 'game is over' message
 * @param position class containing variables concerning the chess game
 * @param result the outcome of the game 
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 */
void draw_end_screen(Position& position, Result& result, sf::RenderWindow& window, Assets& assets);

/**
 * @brief draws the pop-up to allow the player to select a promotion piece 
 * @param position class containing variables concerning the chess game
 * @param ui class containing variables regarding user interaction with the chess pieces
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 * @param premove boolean flag which is true if the screen is being drawn for a premove promotion move and
 * false otherwise 
 */
void draw_pawn_promotion_screen(Position& position, UI& ui, sf::RenderWindow& window, Assets& assets, 
    bool premove = false);

/**
 * @brief draws the panel showing the move history is algebraic chess notation
 * @param log class containing variables concerning the move log
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 */
void draw_move_history_panel(Log& log, sf::RenderWindow& window, Assets& assets);

inline void draw_scroll_bar(Log& log, sf::RenderWindow& window, int& total_pairs);

std::string convert_time_to_display(double time);

void set_clock_positions(Assets& assets, int& view);

/**
 * @brief handles user input
 * @param game class containing all game variables/classes
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 */
void handle_input(Game& game, sf::RenderWindow& window, Assets& assets);

/**
 * @brief calls the appropriate input handler function based on the click event
 * @param game class containing all game variables/classes
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 * @param world_pos position of the click
 */
void delegate_click_event(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2f& world_pos);

/**
 * @brief handles click events that occur on the intro screen
 * @param game class containing all game variables/classes
 * @param window serves as target for 2D drawing
 * @param assets class containing everything drawn in the game
 * @param mouse_pos position of the cursor on the screen
 */
void handle_clicks_intro(Game& game, sf::RenderWindow& window, Assets& assets, sf::Vector2i mouse_pos);

void handle_time_control_selection(Game& game, Assets& assets, sf::Vector2i& mouse_pos);

/**
 * @brief handles cursor movement and clicks during the game
 * @param game class containing all game variables/classes
 * @param window serves as target for 2D drawing
 * @param mouse_pos position of the cursor on the screen
 * @param assets class containing everything drawn in the game
 */
void handle_clicks_playing(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);

void handle_clicks_premoving(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);

/**
 * @brief handles what occurs when the player drops a piece (stops dragging it)
 * @param game class containing all game variables/classes
 * @param window serves as target for 2D drawing
 * @param mouse_pos position of the cursor on the screen
 * @param assets class containing everything drawn in the game
 */
void handle_drag_release(Game& game, sf::RenderWindow& window, sf::Vector2i mouse_pos, Assets& assets);

void set_piece_dragging(Game& game, Assets& assets, sf::Vector2f& world_pos);

inline int find_square_selected(UI& ui, sf::Vector2i& mouse_pos);

/**
 * @brief controls the game state on the pawn promotion screen
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 * @param mouse_pos position of the cursor on the screen
 */
void handle_clicks_promoting(Game& game, Assets& assets, sf::Vector2i mouse_pos, bool premove = false);

/**
 * @brief handles clicks for when the player resets the game (clicking the reset button)
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 * @param mouse_pos position of the cursor on the screen
 */
void handle_clicks_resetting(Game& game, Assets& assets, sf::Vector2i mouse_pos);

/** 
 * @brief handles clicks to undo a move (clicking the undo button)
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 * @param mouse_pos position of the cursor on the screen
 */
void handle_clicks_undoing(Game& game, Assets& assets, sf::Vector2i mouse_pos);

/**
 * @brief handles clicks for returning to the intro screen
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 * @param mouse_pos position of the cursor on the screen
 */
void handle_clicks_returning(Game& game, Assets& assets, sf::Vector2i mouse_pos);

/**
 * @brief handles clicks to flip the chessboard
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 * @param mouse_pos position of the cursor on the screen
 */
void handle_clicks_flip_view(Game& game, Assets& assets, sf::Vector2i mouse_pos);

/**
 * @brief handles clicks on the chessboard during gameplay
 * @param mouse_pos position of the cursor on the screen
 * @param position class containing variables concerning the chess game
 * @param ui class containing variables regarding user interaction with the chess pieces
 * @return number greater than 0 for a valid move/click, and a number less than 0 for an invalid move/click
 */
int select_square(sf::Vector2i& mouse_pos, Position& position, UI& ui); 

/**
 * @brief allows the player to select a promotion piece on the pawn promotion screen
 * @param game class containing all game variables/classes
 * @param mouse_pos position of the cursor on the screen
 * @param premove boolean flag which is true if the a piece is being selected for a premove and false otherwise
 * @returns true if a valid selection was made, and false otherwise
 */
bool select_promotion_piece(Game& game, sf::Vector2i mouse_pos, bool premove);

/**
 * @brief plays a sound within the game
 * @param game class containing all game variables/classes
 * @param assets class containing everything drawn in the game
 */
void play_sound(Assets& assets, Game& game);





