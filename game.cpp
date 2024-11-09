#include <pthread.h>
#include <vector>
#include <random>
#include <cmath>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <queue>
#include <fcntl.h>
#include <sys/time.h>
#include "/opt/homebrew/Cellar/sdl2/2.30.9/include/SDL2/SDL.h"

using namespace std;

// Message structure for thread communication
struct GameMessage {
    enum Type { MOVE, COLLECT, QUIT } type;
    int player_id;
    int dx;
    int dy;
    int item_index;
};

// Global flag for game state
bool running = true;

// Pipe file descriptors for each player
int player1_pipe[2];
int player2_pipe[2];

struct Player {
    int x, y;
    int score;
    char symbol;
    int priority;
    
    Player(int startX, int startY, char sym, int prio) : 
        x(startX), y(startY), score(0), symbol(sym), priority(prio) {}
};

struct Item {
    int x, y;
    bool collected;
    
    Item(int posX, int posY) : 
        x(posX), y(posY), collected(false) {}
};

class GameBoard {
public:
    int board_size;
    vector<Player> players;
    vector<Item> items;
    
    GameBoard() {
        calculateBoardSize();
        initializePlayers();
        initializeItems();
    }
    
private:
    void calculateBoardSize() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(10, 99);
        
        int random_num = dis(gen);
        int calc_1 = random_num * 3;
        int board_size_calc = calc_1 % 25;
        
        if (board_size_calc < 10) {
            board_size_calc += 15;
        }
        
        board_size = board_size_calc;
    }
    
    void initializePlayers() {
        players.emplace_back(0, 0, '1', 1);  // Player 1 with priority 1
        players.emplace_back(board_size-1, board_size-1, '2', 1);  // Player 2 with priority 1
    }
    
    void initializeItems() {
        int num_items = board_size * 2;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, board_size-1);
        
        for (int i = 0; i < num_items; ++i) {
            items.emplace_back(dis(gen), dis(gen));
        }
    }
};

GameBoard* game;
void audioCallback(void* userdata, Uint8* stream, int len);
void playBeep();

// Function declarations
bool movePlayer(Player& player, int dx, int dy);
void displayGame();
void renderGame();
void processMessages();
void* handlePlayer1Input(void* arg);
void* handlePlayer2Input(void* arg);

// Constants for graphics
const int SCREEN_SIZE = 800;
const int CELL_PADDING = 2;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Add colors for graphics
struct Color {
    Uint8 r, g, b;
    Color(Uint8 red, Uint8 green, Uint8 blue) : r(red), g(green), b(blue) {}
};

const Color PLAYER1_COLOR(255, 0, 0);    // Red
const Color PLAYER2_COLOR(0, 0, 255);    // Blue
const Color ITEM_COLOR(0, 255, 0);       // Green
const Color GRID_COLOR(200, 200, 200);   // Light Gray
const Color BG_COLOR(255, 255, 255);     // White

// Add this helper function to draw individual digits
void drawDigit(int digit, int x, int y, int width, int height) {
    // Declare rect outside switch
    SDL_Rect rect;
    
    switch(digit) {
        case 0:
            // Draw a rectangle for 0
            rect = {x, y, width, height};
            SDL_RenderDrawRect(renderer, &rect);
            break;
        case 1:
            // Vertical line for 1
            SDL_RenderDrawLine(renderer, x + width/2, y, x + width/2, y + height);
            break;
        case 2:
            // Top horizontal
            SDL_RenderDrawLine(renderer, x, y, x + width, y);
            // Middle horizontal
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            // Bottom horizontal
            SDL_RenderDrawLine(renderer, x, y + height, x + width, y + height);
            // Top right vertical
            SDL_RenderDrawLine(renderer, x + width, y, x + width, y + height/2);
            // Bottom left vertical
            SDL_RenderDrawLine(renderer, x, y + height/2, x, y + height);
            break;
        case 3:
            // Three horizontal lines
            SDL_RenderDrawLine(renderer, x, y, x + width, y);
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            SDL_RenderDrawLine(renderer, x, y + height, x + width, y + height);
            // Right vertical line
            SDL_RenderDrawLine(renderer, x + width, y, x + width, y + height);
            break;
        case 4:
            // Top vertical line
            SDL_RenderDrawLine(renderer, x, y, x, y + height/2);
            // Middle horizontal line
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            // Right vertical line
            SDL_RenderDrawLine(renderer, x + width, y, x + width, y + height);
            break;
        case 5:
            // Horizontal lines
            SDL_RenderDrawLine(renderer, x, y, x + width, y);
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            SDL_RenderDrawLine(renderer, x, y + height, x + width, y + height);
            // Top left vertical
            SDL_RenderDrawLine(renderer, x, y, x, y + height/2);
            // Bottom right vertical
            SDL_RenderDrawLine(renderer, x + width, y + height/2, x + width, y + height);
            break;
        case 6:
            // Horizontal lines
            SDL_RenderDrawLine(renderer, x, y, x + width, y);
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            SDL_RenderDrawLine(renderer, x, y + height, x + width, y + height);
            // Left vertical
            SDL_RenderDrawLine(renderer, x, y, x, y + height);
            // Bottom right vertical
            SDL_RenderDrawLine(renderer, x + width, y + height/2, x + width, y + height);
            break;
        case 7:
            // Top horizontal
            SDL_RenderDrawLine(renderer, x, y, x + width, y);
            // Right vertical
            SDL_RenderDrawLine(renderer, x + width, y, x + width, y + height);
            break;
        case 8:
            // All lines
            rect = {x, y, width, height};
            SDL_RenderDrawRect(renderer, &rect);
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            break;
        case 9:
            // Horizontal lines
            SDL_RenderDrawLine(renderer, x, y, x + width, y);
            SDL_RenderDrawLine(renderer, x, y + height/2, x + width, y + height/2);
            // Right vertical
            SDL_RenderDrawLine(renderer, x + width, y, x + width, y + height);
            // Top left vertical
            SDL_RenderDrawLine(renderer, x, y, x, y + height/2);
            break;
    }
}

// Update renderScore function to use the digit drawing
void renderScore(int x, int y, int score, const Color& color) {
    // Draw background
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_Rect scoreBackground = {
        x,
        y,
        100,
        50
    };
    SDL_RenderFillRect(renderer, &scoreBackground);
    
    // Draw label (P1 or P2)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    string label = (color.r == PLAYER1_COLOR.r) ? "P1:" : "P2:";
    SDL_Rect labelRect = {
        x + 5,
        y + 10,
        30,
        30
    };
    SDL_RenderDrawRect(renderer, &labelRect);
    
    // Draw score digits
    string scoreStr = to_string(score);
    int digitWidth = 20;
    int digitHeight = 30;
    int startX = x + 45;
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (char c : scoreStr) {
        int digit = c - '0';
        drawDigit(digit, startX, y + 10, digitWidth, digitHeight);
        startX += digitWidth + 5;
    }
}

// Add this function to check if game is over
bool isGameOver() {
    for (const auto& item : game->items) {
        if (!item.collected) {
            return false;
        }
    }
    return true;
}

void renderGame() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 255);
    SDL_RenderClear(renderer);
    
    int CELL_SIZE = SCREEN_SIZE / game->board_size;
    
    // Draw grid
    SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, 255);
    for (int i = 0; i <= game->board_size; i++) {
        SDL_RenderDrawLine(renderer, i * CELL_SIZE, 0, i * CELL_SIZE, SCREEN_SIZE);
        SDL_RenderDrawLine(renderer, 0, i * CELL_SIZE, SCREEN_SIZE, i * CELL_SIZE);
    }
    
    // Draw items
    SDL_SetRenderDrawColor(renderer, ITEM_COLOR.r, ITEM_COLOR.g, ITEM_COLOR.b, 255);
    for (const auto& item : game->items) {
        if (!item.collected) {
            SDL_Rect itemRect = {
                item.x * CELL_SIZE + CELL_PADDING,
                item.y * CELL_SIZE + CELL_PADDING,
                CELL_SIZE - 2*CELL_PADDING,
                CELL_SIZE - 2*CELL_PADDING
            };
            SDL_RenderFillRect(renderer, &itemRect);
        }
    }
    
    // Draw players
    for (const auto& player : game->players) {
        if (player.symbol == '1') {
            SDL_SetRenderDrawColor(renderer, PLAYER1_COLOR.r, PLAYER1_COLOR.g, PLAYER1_COLOR.b, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, PLAYER2_COLOR.r, PLAYER2_COLOR.g, PLAYER2_COLOR.b, 255);
        }
        
        SDL_Rect playerRect = {
            player.x * CELL_SIZE + CELL_PADDING,
            player.y * CELL_SIZE + CELL_PADDING,
            CELL_SIZE - 2*CELL_PADDING,
            CELL_SIZE - 2*CELL_PADDING
        };
        SDL_RenderFillRect(renderer, &playerRect);
    }
    
    // Draw scores at the top of the window with labels
    renderScore(10, 10, game->players[0].score, PLAYER1_COLOR);
    renderScore(SCREEN_SIZE - 110, 10, game->players[1].score, PLAYER2_COLOR);
    
    // Check for game over
    if (isGameOver()) {
        // Determine winner
        Player* winner = &game->players[0];
        if (game->players[1].score > game->players[0].score) {
            winner = &game->players[1];
        }
        
        // Draw semi-transparent overlay
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_Rect overlay = {0, 0, SCREEN_SIZE, SCREEN_SIZE};
        SDL_RenderFillRect(renderer, &overlay);
        
        // Draw winner announcement box
        SDL_SetRenderDrawColor(renderer, 
            winner->symbol == '1' ? PLAYER1_COLOR.r : PLAYER2_COLOR.r,
            winner->symbol == '1' ? PLAYER1_COLOR.g : PLAYER2_COLOR.g,
            winner->symbol == '1' ? PLAYER1_COLOR.b : PLAYER2_COLOR.b, 
            255);
        
        // Winner box
        SDL_Rect winnerBox = {
            SCREEN_SIZE/4,
            SCREEN_SIZE/3,
            SCREEN_SIZE/2,
            SCREEN_SIZE/3
        };
        SDL_RenderFillRect(renderer, &winnerBox);
        
        // Draw border for winner box
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &winnerBox);
        
        // Draw final scores
        // Player 1 score
        renderScore(SCREEN_SIZE/4 + 50, 
                   SCREEN_SIZE/3 + 30,
                   game->players[0].score, 
                   PLAYER1_COLOR);
               
        // Player 2 score
        renderScore(SCREEN_SIZE/4 + 50,
                   SCREEN_SIZE/3 + 90,
                   game->players[1].score,
                   PLAYER2_COLOR);
        
        // Draw winner text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        string winnerText = "Player " + string(1, winner->symbol) + " Wins!";
        
        // Draw winner announcement
        SDL_Rect winnerTextBox = {
            SCREEN_SIZE/4 + 50,
            SCREEN_SIZE/3 + 150,
            SCREEN_SIZE/2 - 100,
            40
        };
        SDL_RenderFillRect(renderer, &winnerTextBox);
        
        // Game will automatically close after 5 seconds
        SDL_RenderPresent(renderer);
        SDL_Delay(5000);
        running = false;
    }
    
    SDL_RenderPresent(renderer);
}

void processMessages() {
    SDL_Event event;
    fd_set read_fds;
    struct timeval tv;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
        }
        
        FD_ZERO(&read_fds);
        FD_SET(player1_pipe[0], &read_fds);
        FD_SET(player2_pipe[0], &read_fds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 16666;  // ~60 FPS
        
        int max_fd = max(player1_pipe[0], player2_pipe[0]) + 1;
        
        if (select(max_fd, &read_fds, nullptr, nullptr, &tv) > 0) {
            GameMessage msg;
            
            if (FD_ISSET(player1_pipe[0], &read_fds)) {
                read(player1_pipe[0], &msg, sizeof(GameMessage));
                if (msg.type == GameMessage::MOVE) {
                    movePlayer(game->players[0], msg.dx, msg.dy);
                }
            }
            
            if (FD_ISSET(player2_pipe[0], &read_fds)) {
                read(player2_pipe[0], &msg, sizeof(GameMessage));
                if (msg.type == GameMessage::MOVE) {
                    movePlayer(game->players[1], msg.dx, msg.dy);
                }
            }
        }
        
        renderGame();
        SDL_Delay(16);
    }
}

bool movePlayer(Player& player, int dx, int dy) {
    int new_x = player.x + dx;
    int new_y = player.y + dy;
    
    // Check if the new position is within bounds
    if (new_x >= 0 && new_x < game->board_size && 
        new_y >= 0 && new_y < game->board_size) {
        player.x = new_x;
        player.y = new_y;
        
        // Check for item collection
        vector<Item>::iterator item_it;
        for (item_it = game->items.begin(); item_it != game->items.end(); ++item_it) {
            if (!item_it->collected && item_it->x == player.x && item_it->y == player.y) {
                item_it->collected = true;
                player.score++;
                player.priority = player.score + 1; // Update priority based on score
                cout << "Player " << player.symbol << " collected an item! Score: " << player.score << endl;
                playBeep();  // Play beep sound when collecting item
                return true;
            }
        }
    }
    return false;
}

void displayGame() {
    cout << "\nGame Board:\n";
    
    // Create temporary board for display
    vector<vector<char> > board(game->board_size, 
                                        vector<char>(game->board_size, '.'));
    
    // Place items
    vector<Item>::const_iterator item_it;
    for (item_it = game->items.begin(); item_it != game->items.end(); ++item_it) {
        if (!item_it->collected) {
            board[item_it->y][item_it->x] = '*';
        }
    }
    
    // Place players
    vector<Player>::const_iterator player_it;
    for (player_it = game->players.begin(); player_it != game->players.end(); ++player_it) {
        board[player_it->y][player_it->x] = player_it->symbol;
    }
    
    // Display the board
    for (int i = 0; i < game->board_size; ++i) {
        for (int j = 0; j < game->board_size; ++j) {
            cout << board[i][j] << " ";
        }
        cout << "\n";
    }
    
    // Display scores
    cout << "\nScores:\n";
    cout << "Player 1: " << game->players[0].score 
              << " (Priority: " << game->players[0].priority << ")\n";
    cout << "Player 2: " << game->players[1].score 
              << " (Priority: " << game->players[1].priority << ")\n";
    
    // Display controls
    cout << "\n=== CONTROLS ===\n";
    cout << "Player 1: WASD keys\n";
    cout << "Player 2: Arrow keys\n";
    cout << "Close window to quit\n";
    cout << "Collect items to score points!\n\n";
}

// Update player thread function
void* playerThread(void* arg) {
    int player_id = *(int*)arg;
    int* pipe_write = (player_id == 0) ? &player1_pipe[1] : &player2_pipe[1];
    
    while (running) {
        GameMessage msg;
        msg.player_id = player_id;
        
        // Handle input based on player ID
        if (player_id == 0) {  // Player 1 (WASD)
            const Uint8* keyState = SDL_GetKeyboardState(NULL);
            if (keyState[SDL_SCANCODE_W]) { msg.dx = 0; msg.dy = -1; }
            else if (keyState[SDL_SCANCODE_S]) { msg.dx = 0; msg.dy = 1; }
            else if (keyState[SDL_SCANCODE_A]) { msg.dx = -1; msg.dy = 0; }
            else if (keyState[SDL_SCANCODE_D]) { msg.dx = 1; msg.dy = 0; }
            else continue;
        }
        else {  // Player 2 (Arrow Keys)
            const Uint8* keyState = SDL_GetKeyboardState(NULL);
            if (keyState[SDL_SCANCODE_UP]) { msg.dx = 0; msg.dy = -1; }
            else if (keyState[SDL_SCANCODE_DOWN]) { msg.dx = 0; msg.dy = 1; }
            else if (keyState[SDL_SCANCODE_LEFT]) { msg.dx = -1; msg.dy = 0; }
            else if (keyState[SDL_SCANCODE_RIGHT]) { msg.dx = 1; msg.dy = 0; }
            else continue;
        }
        
        msg.type = GameMessage::MOVE;
        write(*pipe_write, &msg, sizeof(GameMessage));
        SDL_Delay(16);  // Prevent too frequent updates
    }
    
    return nullptr;
}

// Audio constants
const int SAMPLE_RATE = 44100;
const int AMPLITUDE = 28000;
const float FREQUENCY = 800.0f;  // Frequency in Hz for the beep
SDL_AudioDeviceID audioDevice;

// Audio callback function
void audioCallback(void* userdata, Uint8* stream, int len) {
    static double phase = 0.0;
    Sint16* buffer = (Sint16*)stream;
    int length = len / 2;  // Since we're using Sint16 (2 bytes)
    
    for(int i = 0; i < length; i++) {
        buffer[i] = (Sint16)(AMPLITUDE * sin(phase));
        phase += 2.0 * M_PI * FREQUENCY / SAMPLE_RATE;
        if(phase > 2.0 * M_PI) {
            phase -= 2.0 * M_PI;
        }
    }
}

// Function to play beep sound
void playBeep() {
    SDL_PauseAudioDevice(audioDevice, 0);  // Start playing
    SDL_Delay(100);  // Play for 100ms
    SDL_PauseAudioDevice(audioDevice, 1);  // Stop playing
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        cerr << "SDL initialization failed: " << SDL_GetError() << endl;
        return 1;
    }
    
    // Initialize audio
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audioCallback;
    
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice == 0) {
        cerr << "Failed to open audio device: " << SDL_GetError() << endl;
        return 1;
    }
    
    window = SDL_CreateWindow("Multiplayer Collection Game",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            SCREEN_SIZE,
                            SCREEN_SIZE,
                            SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Window creation failed: " << SDL_GetError() << endl;
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Renderer creation failed: " << SDL_GetError() << endl;
        return 1;
    }
   
    game = new GameBoard();
    
    cout << "Welcome to Multiplayer Collection Game!\n";
    cout << "Player 1: WASD keys\n";
    cout << "Player 2: Arrow keys\n";
    cout << "Close window to quit\n";
    cout << "Collect items to score points!\n\n";
    
    // Create pipes for message passing
    if (pipe(player1_pipe) < 0 || pipe(player2_pipe) < 0) {
        cerr << "Failed to create pipes" << endl;
        return 1;
    }
    
    // Set pipes to non-blocking mode
    fcntl(player1_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(player2_pipe[0], F_SETFL, O_NONBLOCK);
    
    // Create player threads
    pthread_t thread1, thread2;
    int player1_id = 0, player2_id = 1;
    
    pthread_create(&thread1, nullptr, playerThread, &player1_id);
    pthread_create(&thread2, nullptr, playerThread, &player2_id);
    
    // Main game loop
    processMessages();
    
    // Cleanup
    pthread_join(thread1, nullptr);
    pthread_join(thread2, nullptr);
    
    close(player1_pipe[0]);
    close(player1_pipe[1]);
    close(player2_pipe[0]);
    close(player2_pipe[1]);
    
    // Add cleanup for audio before SDL_Quit()
    SDL_CloseAudioDevice(audioDevice);
    
    delete game;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
} 