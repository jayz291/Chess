# Chess

Chess in C++, with a basic frontend built using SFML and with a (seemingly) UCI-compliant chess engine.

## Details
* **Board Representation:** Bitboards (uint64_t) to represent each chess piece (1 for occupied, 0 for not occupied), as well as a 1D array of length 64. 
* **Move Representation:** Uses uint32_t to represent each move.
* **Search:** Uses negamax with alpha-beta pruning, transposition tables, check extensions, quiescence search, move ordering, and null move pruning. 
* **Evaluation:** Uses piece square tables and material value.

## Prerequisites
* **C++ Compiler:** GCC or Clang supporting C++20 (or higher).
* **CMake:** Version 3.15 or higher.
* **Build System:** Make, Ninja, or Visual Studio.
* **SFML:** Version 3.0 or higher.

## Installation
### 1. Install Dependencies

**macOS (Homebrew):**
```bash
brew install sfml
```

### 2. Clone the repository
```bash
git clone https://github.com/jayz291/Chess.git
cd Chess
```

### 3. Create a build directory
```bash
mkdir build
cd build
```

### 4.  Configure the project:
```bash
cmake ..
```
*(Note: Use `cmake -DCMAKE_BUILD_TYPE=Release ..` to build in Release mode, for max speed)*

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

To use the UCI protocol:
```bash
./chess uci
```