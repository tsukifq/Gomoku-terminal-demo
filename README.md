# Gomoku (Five-in-a-Row) C++ Project

## Overview
This is a C++ implementation of the Gomoku game with a command-line interface. It features:
- **Object-Oriented Design**: Modular architecture with `GameEngine`, `RuleSet`, `Board`, `Player`, and `Renderer`.
- **Gomoku Rules**:
  - Black plays first (Tengen).
  - Forbidden moves for Black: Overline (6+), Three-Three, Four-Four.
  - "Five Priority": If a move creates a Five and a Forbidden pattern, the Five wins.
  - White wins on Overline.
  - "Immediate Claim": White must claim Black's forbidden move immediately.
- **Game Modes**: Human vs Human, Human vs AI, AI vs Human.
- **Timing**: 15-second move limit with warnings (max 3).

## Build Instructions

### Prerequisites
- C++17 compatible compiler (GCC, Clang, MSVC).
- CMake (optional, but recommended).

### Using CMake
```bash
mkdir build
cd build
cmake ..
cmake --build .
./Gomoku
```

### Manual Compilation
```bash
g++ -std=c++17 -I./include src/*.cpp main.cpp -o gomoku
./gomoku
```

## How to Play
- **Input**: Enter coordinates like `H8` (Column Letter + Row Number).
- **Commands**:
  - `q` or `quit`: Resign.
  - `claim`: Claim a forbidden move (only when prompted).
  - `draw`: Offer draw.

## Architecture
- **GameEngine**: Manages the game loop, timing, and player turns.
- **RuleSet**: Abstract base class for game rules. `GomokuRuleSet` implements specific logic.
- **Board**: Manages the grid state.
- **Player**: Abstract base class. `HumanPlayer` handles input, `AIPlayer` uses heuristics.
- **Renderer**: Handles console output.

## Forbidden Move Logic
Implemented using pattern matching in `GomokuRuleSet`.
- **Overline**: Checks for >5 stones.
- **Three-Three**: Checks for >=2 "Open Threes" (patterns like `01110`).
- **Four-Four**: Checks for >=2 "Fours" (patterns that can become Five).
