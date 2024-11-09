# Multi-Threading Game

This project is a multiplayer collection game implemented in C++ using SDL2 for graphics and audio. The game demonstrates the use of multi-threading and inter-process communication to handle player inputs and game state updates.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Game Controls](#game-controls)
- [Game Rules](#game-rules)
- [Contributing](#contributing)
- [License](#license)

## Features

- **Multiplayer Support**: Two players can play simultaneously.
- **Graphics and Audio**: Utilizes SDL2 for rendering graphics and playing audio.
- **Multi-threading**: Uses POSIX threads to handle player inputs concurrently.
- **Dynamic Game Board**: The game board size is randomly generated at the start of each game.
- **Item Collection**: Players collect items to score points.

## Requirements

- C++ Compiler (e.g., g++)
- SDL2 Library
- POSIX Threads (pthread)

## Installation

1. **Clone the Repository**:

    ```bash
    git clone https://github.com/haroonwajid/Multi-Threading-Game.git
    cd Multi-Threading-Game
    ```

2. **Install SDL2**:

   Ensure SDL2 is installed on your system. You can install it using a package manager:

   - **macOS**: `brew install sdl2`
   - **Linux**: `sudo apt-get install libsdl2-dev`
   - **Windows**: Download and install from the [SDL2 website](https://www.libsdl.org/download-2.0.php).

3. **Compile the Game**:

    ```bash
    g++ -o game game.cpp -lpthread -lSDL2
    ```

## Usage

Run the compiled game executable:
./game


## Game Controls

- **Player 1**: Use the `W`, `A`, `S`, `D` keys to move up, left, down, and right, respectively.
- **Player 2**: Use the arrow keys to move up, left, down, and right.

## Game Rules

- The objective is to collect as many items as possible.
- Each item collected increases the player's score.
- The game ends when all items are collected.
- The player with the highest score at the end of the game wins.

## Contributing

Contributions are welcome! Please fork the repository and submit a pull request for any improvements or bug fixes.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.