# Chess

Chess in C++, with a basic frontend built using SFML and with a (seemingly) UCI-compliant chess engine.

## Details
* **Board Representation:** Bitboards (uint64_t) are used to represent each chess piece. There are 12 bitboards (6 unique pieces for each side * 2 sides). A 1D array of length 64 is also used to store the position of the pieces (each unique piece is assigned a number). Each position also has a zobrist hash assigned (mostly for use in a transposition table).
* **Move Representation:** Uses uint32_t to represent each move. Bitwise operations are used to retrieve/add data to each move.
* **Move Generation:** Movegen is mostly done with precomputed attack tables and bitwise operations. For the sliding pieces, magic bitboards are used. 
* **Search:** Uses negamax with alpha-beta pruning, transposition tables, check extensions, quiescence search, move ordering, null move pruning and principal variation search. 
* **Evaluation:** Hand-crafted evaluation using piece square tables, material value, and a basic pawn structure evaluation function.

## Prerequisites
* **C++ Compiler:** GCC or Clang supporting C++20 (or higher). 
* **CMake:** Version 3.15 or higher.
* **Build System:** Make.
* **SFML:** Version 3.0 or higher.

## Installation
### 1. Install Dependencies:

**macOS (Homebrew):**
```bash
brew install sfml
```

### 2. Clone the repository:
```bash
git clone https://github.com/jayz291/Chess.git
cd Chess
```

### 3. Create a build directory:
```bash
mkdir build
cd build
```

### 4.  Configure the project:
```bash
cmake ..
```
*(Note: Use `cmake -DCMAKE_BUILD_TYPE=Release ..` to build in Release mode, for max speed).*
*Otherwise, it will be built in debug mode by default.*

### 5.  Compile:
```bash
cmake --build .
```

## Usage
Run the exectuable from the `build` folder.

To use the SFML interface:
```bash
./chess
```
You can play the computer as white, black, or have a 2-player game.

To use the UCI protocol:
```bash
./chess uci
```
Basic UCI commands are supported (position/stop/quit/isready/ucinewgame, as well as go wtime/btime/winc/binc/movetime/depth).
Additionally, run ```perft [depth]``` to run a perft test on the current position (by default, there is no current position. You need to set it first - ```position startpos``` or ```position fen [fen string]```).