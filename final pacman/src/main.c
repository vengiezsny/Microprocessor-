/******************************************************************************
 * Heart Chaser - Embedded Systems Game
 ******************************************************************************
 * File:    main.c
 * Course:  TU857-2 Microprocessor Systems
 * School:  Technological University Dublin
 * Term:    Semester 1, 2024-25
 * 
 * Authors: Vengie Legaspi
 *          Cristian [Last Name]
 *          John [Last Name]
 *
 * Date:    November 6, 2024
 * 
 * Description:
 *     A maze-based game where the player chases and collects hearts while
 *     avoiding enemy pumpkins. Features multiple levels, sound effects,
 *     and animated sprites. Developed for STM32F031 microcontroller.
 * 
 * Hardware:
 *     - STM32F031 MCU
 *     - LCD Display
 *     - 4 Directional Buttons
 *     - Speaker for Sound Effects
 *****************************************************************************/

/******************************************************************************
 * Include Files
 *****************************************************************************/
#include <string.h>       // Required for string operations (strlen for text centering)
#include <stm32f031x6.h>  // STM32 microcontroller specific definitions
#include "display.h"      // LCD display functions
#include <stdlib.h>       // Required for random number generation (heart movement)
#include "sound.h"        // Sound effect functions for eating hearts
#include "musical_notes.h"// Musical note definitions for sound effects
#include "serial.h"       // Serial communication functions
#include <stdio.h>        // Standard I/O (sprintf for text formatting)

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/
// Core game rendering functions
void drawBackground(void);        // Draws the maze and background
void showWinScreen(void);         // Displays the victory screen
void initEnemies(void);           // Sets up enemy positions and properties
void checkWinCondition(uint16_t x, uint16_t y);  // Checks if level is complete
void moveEnemies(uint16_t pacman_x, uint16_t pacman_y);  // Updates enemy positions
int checkEnemyCollision(uint16_t pacman_x, uint16_t pacman_y);  // Detects player-enemy collisions

// Menu and UI functions
void drawGameOverMenu(void);      // Shows game over screen
void drawMenu(void);              // Displays main menu
void showControls(void);          // Shows game controls
void showCredits(void);           // Displays game credits

/******************************************************************************
 * Game Constants
 *****************************************************************************/
// Game configuration
#define MAX_ENEMIES 6    // Number of pumpkin enemies (increased from 3 to 6)
#define MAX_HEARTS 6     // Maximum number of collectible hearts in final level

// Maze appearance constants
#define WALL_COLOR RGBToWord(0, 0, 255)  // Blue color for maze walls
#define PATH_COLOR RGBToWord(0, 0, 20)   // Dark background color for paths
#define WALL_SIZE 8                      // Size of each wall block in pixels

// Victory screen colors
#define WIN_GOLD RGBToWord(0xFF, 0xD7, 0x00)  // Golden color for victory effects
#define WIN_PINK RGBToWord(0xFF, 0x69, 0xB4)  // Pink color for victory effects
#define WIN_BLUE RGBToWord(0x00, 0xBF, 0xFF)  // Sky blue for victory effects

// Menu color scheme
#define TITLE_COLOR RGBToWord(0xff, 0x1a, 0x1a)  // Bright red for titles
#define SELECTED_COLOR RGBToWord(0xff, 0xff, 0)   // Yellow for selected items
#define UNSELECTED_COLOR RGBToWord(0, 0xff, 0)    // Green for unselected items
#define BORDER_COLOR RGBToWord(0, 0, 0xff)        // Blue for borders

/******************************************************************************
 * Sound Variables
 *****************************************************************************/
// Background music variables
const uint32_t *background_tune_notes;    // Pointer to background music notes
const uint32_t *background_tune_times;    // Pointer to note durations
uint32_t background_tune_note_count;      // Number of notes in background music
uint32_t background_repeat_tune;          // Flag to loop background music
uint32_t pumpkin_move_delay = 0;         // Timer for enemy movement

// Sound effect variables
const uint32_t my_notes[] = {A3,C5,B2,D1,F6};  // Notes for sound effects
const uint32_t my_note_times[] = {200,300,400,100,500};  // Duration of each note
uint32_t repeat_tune;  // Flag to repeat sound effect

/******************************************************************************
 * Game State Variables
 *****************************************************************************/
// Core game state
int game_won = 0;              // Tracks if player has won
int game_over = 0;             // Tracks if game is over
int show_game_over_menu = 0;   // Controls game over menu visibility
int game_over_selection = 0;   // Menu selection (0=Play Again, 1=Main Menu)

/******************************************************************************
 * System Function Prototypes
 *****************************************************************************/
void initClock(void);          // Initialize system clock
void initSysTick(void);        // Initialize system timer
void SysTick_Handler(void);    // System tick interrupt handler
void delay(volatile uint32_t dly);  // Time delay function
void setupIO();                // Initialize IO pins
int isInside(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, 
             uint16_t px, uint16_t py);  // Collision detection
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);  // GPIO pull-up config
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);  // GPIO mode config

/******************************************************************************
 * Game Object Variables
 *****************************************************************************/
// System timing
volatile uint32_t milliseconds;  // System time counter

// Heart positions and states - Level 1
uint16_t heart1_x = 40, heart1_y = 80;  // Heart 1 position
uint16_t heart2_x = 60, heart2_y = 90;  // Heart 2 position
int heart1_direction_x = 1;             // Heart 1 movement direction
int heart1_direction_y = 1;
int heart2_direction_x = 1;             // Heart 2 movement direction
int heart2_direction_y = 1;
uint32_t heart_move_delay = 0;          // Heart movement timer
int heart1_eaten = 0;                   // Heart collection status
int heart2_eaten = 0;

// Level tracking
int current_level = 1;         // Current game level (now goes to 3)
int hearts_collected = 0;      // Total hearts collected

// Heart positions and states - Level 2 & 3
uint16_t heart3_x = 80, heart3_y = 70;  // Heart 3 position
uint16_t heart4_x = 30, heart4_y = 100; // Heart 4 position
uint16_t heart5_x = 90, heart5_y = 110; // Heart 5 position (new)
uint16_t heart6_x = 20, heart6_y = 60;  // Heart 6 position (new)
int heart3_eaten = 0;                   // Heart collection status
int heart4_eaten = 0;
int heart5_eaten = 0;                   // New heart status
int heart6_eaten = 0;                   // New heart status
int heart3_direction_x = 1, heart3_direction_y = 1;  // Movement directions
int heart4_direction_x = 1, heart4_direction_y = 1;
int heart5_direction_x = 1, heart5_direction_y = 1;  // New heart movement
int heart6_direction_x = 1, heart6_direction_y = 1;  // New heart movement

// Boss enemy variables (new)
typedef struct {
    uint16_t x;
    uint16_t y;
    int active;
    int speed;
    int size;  // Boss is larger than regular enemies
} Boss;

Boss boss = {64, 80, 0, 2, 24};  // Boss initialization

/******************************************************************************
 * Sprite Definitions
 *****************************************************************************/
// Pacman sprite facing right 
const uint16_t pac1[]=
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16384,0,0,0,0,0,0,0,0,0,0,0,61507,46885,3907,52555,4668,0,0,0,0,0,61756,23062,24327,24327,24327,24327,12099,64782,40960,0,0,0,63013,24327,24327,65535,24327,24327,23566,31006,37436,0,16384,63005,24327,61507,24327,24327,40871,40871,40871,40871,40871,0,0,37940,4916,54573,24327,40871,0,0,0,0,0,0,57344,46132,13869,54324,37180,24327,40871,40871,40871,40871,40871,0,0,0,36427,38693,38693,24327,24327,24327,24327,16135,31510,0,0,0,53820,44867,23062,37940,21805,39446,63517,39446,16384,0,0,0,0,0,54068,13357,53564,61251,53315,24576,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
const uint16_t pacman2[]= 
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16384,32768,32768,32768,24576,0,0,0,0,0,0,0,53315,21549,38693,31006,62765,0,0,0,0,0,24327,23062,24327,24327,7687,40711,31510,64030,30757,0,0,0,63013,24327,24327,65535,24327,24327,24327,24487,32423,0,264,54573,24327,61507,24327,24327,24327,40871,40871,0,0,0,0,54316,4916,54573,24327,40871,40871,0,0,0,0,0,57344,46132,13869,54324,37180,24327,24327,40871,40871,0,0,0,0,0,36427,38693,38693,39446,4668,24327,7943,32679,40871,0,0,0,24327,44867,23062,37940,21805,31758,47133,47390,55325,49152,0,0,0,0,54068,13357,53564,61251,53315,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
const uint16_t pacman3top[]= 
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24576,24576,8192,0,0,0,0,0,0,0,0,6405,30733,30733,47373,8192,0,0,0,0,0,0,22789,54805,22037,30229,30477,63749,0,0,0,0,24576,22541,13596,54044,62236,29468,46108,38164,63501,8192,0,16384,63501,13845,4644,20523,12331,20772,36899,37916,21780,30477,0,24576,22285,54548,12580,4139,52779,3883,45091,54300,21780,30229,40960,16384,22285,29972,12580,36651,20267,20267,61731,13084,13845,30229,40960,0,0,29972,4644,4388,53796,4644,21028,21780,30229,0,0,0,0,0,22037,29724,5148,54548,5909,14349,0,0,0,0,0,0,0,55309,62997,14093,30981,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
const uint16_t pacmanheart[]=
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23568,23568,0,0,23568,23568,0,0,0,0,0,23568,23568,23568,0,0,23568,23568,23568,0,0,0,39952,23081,6945,23568,23568,23568,23568,23568,23568,23568,0,0,23568,6945,14906,64049,48144,23568,23568,23568,23568,23568,0,0,23568,39960,31529,14898,23568,23568,23568,23568,23568,23568,0,0,23568,23568,23568,31768,23568,23568,23568,23568,23568,23568,0,0,0,23568,23568,23568,23568,23568,23568,23568,23568,0,0,0,0,0,23568,23568,23568,23568,23568,23568,0,0,0,0,0,0,0,0,23568,23568,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

const uint16_t pacmanheart2[]=
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23568,23568,0,0,23568,23568,0,0,0,0,0,23568,23568,23568,0,0,23568,23568,23568,0,0,0,39952,23081,6945,23568,23568,23568,23568,23568,23568,23568,0,0,23568,6945,14906,64049,48144,23568,23568,23568,23568,23568,0,0,23568,39960,31529,14898,23568,23568,23568,23568,23568,23568,0,0,23568,23568,23568,31768,23568,23568,23568,23568,23568,23568,0,0,0,23568,23568,23568,23568,23568,23568,23568,23568,0,0,0,0,0,23568,23568,23568,23568,23568,23568,0,0,0,0,0,0,0,0,23568,23568,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// Pumpkin sprite - a scary 12x16 jack-o'-lantern
const uint16_t pumpkin_sprite[] = {
    0,0,0,0,2016,2016,2016,2016,0,0,0,0,    // Green stem
    0,0,2016,2016,2016,2016,2016,2016,2016,2016,0,0,
    0,64800,64800,2016,2016,2016,2016,2016,2016,64800,64800,0,  // Orange body starts
    64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,
    64800,65535,64800,65535,64800,64800,65535,64800,65535,64800,64800,64800,  // Evil eyes
    64800,65535,64800,65535,64800,64800,65535,64800,65535,64800,64800,64800,
    64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,
    64800,64800,64800,64800,0,0,64800,64800,64800,64800,64800,64800,  // Evil grin starts
    64800,64800,64800,0,65535,65535,0,64800,64800,64800,64800,64800,
    64800,64800,0,65535,65535,65535,65535,0,64800,64800,64800,64800,
    64800,64800,65535,65535,65535,65535,65535,65535,64800,64800,64800,64800,
    64800,64800,65535,0,65535,65535,0,65535,64800,64800,64800,64800,
    64800,64800,0,0,0,0,0,0,64800,64800,64800,64800,
    0,64800,64800,64800,64800,64800,64800,64800,64800,64800,64800,0,
    0,0,64800,64800,64800,64800,64800,64800,64800,64800,0,0,
    0,0,0,64800,64800,64800,64800,64800,64800,0,0,0
};

/******************************************************************************
 * Enemy Structure Definition
 *****************************************************************************/
// Structure to track enemy properties
typedef struct {
    uint16_t x;       // X position on screen
    uint16_t y;       // Y position on screen
    int active;       // Whether enemy is currently active
    int speed;        // Movement speed of enemy
} Enemy;
Enemy enemies[MAX_ENEMIES];  // Array to hold all enemy instances


// Ghost sprite - a cute 12x16 ghost shape
/*const uint16_t ghost_sprite[] = {
    0,0,0,0,63488,63488,63488,63488,0,0,0,0,
    0,0,63488,63488,63488,63488,63488,63488,63488,63488,0,0,
    0,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,0,
    63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,
    63488,63488,65535,65535,63488,63488,65535,65535,63488,63488,63488,63488,
    63488,63488,65535,65535,63488,63488,65535,65535,63488,63488,63488,63488,
    63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,
    63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,
    63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,
    63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,
    63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,63488,
    63488,0,63488,63488,0,63488,63488,0,63488,63488,0,63488,
    0,0,0,63488,0,0,63488,0,0,63488,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0
};*/

/******************************************************************************
 * Enemy Management Functions
 *****************************************************************************/
/**
 * Initializes enemies for each level
 * Sets initial positions, activation status, and speeds
 */
void initEnemies() {
    if (current_level == 1) {
        // Level 1: Single enemy setup
        enemies[0].x = 10;            // Starting X position
        enemies[0].y = 10;            // Starting Y position
        enemies[0].active = 1;        // Activate enemy
        enemies[0].speed = 1;         // Base movement speed
    } else {
        // Level 2: Three enemies with adjusted positions
        // First enemy
        enemies[0].x = 10;
        enemies[0].y = 10;
        enemies[0].active = 1;
        enemies[0].speed = 1;

        // Second enemy
        enemies[1].x = 100;
        enemies[1].y = 10;
        enemies[1].active = 1;
        enemies[1].speed = 1;

        // Third enemy
        enemies[2].x = 60;
        enemies[2].y = 140;
        enemies[2].active = 1;
        enemies[2].speed = 1;
    }

    // Draw all active enemies in their initial positions
    for(int i = 0; i < (current_level == 1 ? 1 : MAX_ENEMIES); i++) {
        if(enemies[i].active) {
            putImage(enemies[i].x, enemies[i].y, 12, 16, pumpkin_sprite, 0, 0);
        }
    }
}

/******************************************************************************
 * Sound Functions
 *****************************************************************************/
/**
 * Plays a sequence of musical notes
 * @param notes: Array of note frequencies
 * @param times: Array of note durations
 * @param len: Length of the arrays
 */
void playTune(const uint32_t notes[], const uint32_t times[], int len) {
    for (int i=0; i<len; i++) {
        playNote(notes[i]);
        delay(times[i]);
    }
    playNote(0);  // Stop sound
}

/******************************************************************************
 * Enemy Movement Functions
 *****************************************************************************/
/**
 * Updates enemy positions relative to player location
 * @param pacman_x: Player's X coordinate
 * @param pacman_y: Player's Y coordinate
 */
void moveEnemies(uint16_t pacman_x, uint16_t pacman_y) {
    if (game_won) return;  // Skip if game is won

    // Set movement delay based on level
    uint32_t delay_time = (current_level == 1) ? 30 : 65;
    
    // Control movement timing
    if (milliseconds - pumpkin_move_delay < delay_time) {
        return;
    }
    pumpkin_move_delay = milliseconds;
    
    // Update each enemy's position
    for(int i = 0; i < MAX_ENEMIES; i++) {
        if(enemies[i].active) {
            // Clear previous position
            fillRectangle(enemies[i].x, enemies[i].y, 12, 16, 0);
            
            // Calculate movement speed based on level
            int current_speed;
            if (current_level == 2) {
                // Slower movement in level 2
                current_speed = (milliseconds % 2 == 0) ? 1 : 0;
            } else {
                current_speed = enemies[i].speed;
            }
            
            // Move toward player
            if(enemies[i].x < pacman_x) enemies[i].x += current_speed;
            if(enemies[i].x > pacman_x) enemies[i].x -= current_speed;
            if(enemies[i].y < pacman_y) enemies[i].y += current_speed;
            if(enemies[i].y > pacman_y) enemies[i].y -= current_speed;
            
            // Enforce screen boundaries
            enemies[i].x = (enemies[i].x <= 0) ? 0 : 
                          (enemies[i].x >= 115) ? 115 : enemies[i].x;
            enemies[i].y = (enemies[i].y <= 0) ? 0 : 
                          (enemies[i].y >= 144) ? 144 : enemies[i].y;
            
            // Draw at new position
            putImage(enemies[i].x, enemies[i].y, 12, 16, pumpkin_sprite, 0, 0);
        }
    }
}

/**
 * Checks for collision between player and enemies
 * @param pacman_x: Player's X coordinate
 * @param pacman_y: Player's Y coordinate
 * @return: 1 if collision detected, 0 otherwise
 */
int checkEnemyCollision(uint16_t pacman_x, uint16_t pacman_y) {
    for(int i = 0; i < MAX_ENEMIES; i++) {
        if(enemies[i].active && 
           isInside(enemies[i].x, enemies[i].y, 12, 16, pacman_x, pacman_y)) {
            return 1;
        }
    }
    return 0;
}

/******************************************************************************
 * Game Logic Functions
 *****************************************************************************/
/**
 * Checks if level/game completion conditions are met
 * @param x: Player's X coordinate
 * @param y: Player's Y coordinate
 */
void checkWinCondition(uint16_t x, uint16_t y) {
    // Calculate required hearts based on level
    int required_hearts = (current_level == 1) ? 2 : 4;
    hearts_collected = heart1_eaten + heart2_eaten;
    if (current_level == 2) {
        hearts_collected += heart3_eaten + heart4_eaten;
    }

    // Check win conditions
    if (hearts_collected == required_hearts && !game_won) {
        if (current_level == 1) {
            // Advance to level 2
            current_level = 2;
            heart1_eaten = heart2_eaten = heart3_eaten = heart4_eaten = 0;
            fillRectangle(0, 0, 128, 160, 0);
            printTextX2("LEVEL 2!", 25, 60, RGBToWord(0, 0xff, 0), 0);
            delay(2000);
            initEnemies();
            drawBackground();
            x = 50; y = 50;  // Reset player position
        } else {
            // Complete game victory
            game_won = 1;
            showWinScreen();
        }
    }
}

/**
 * Resets heart positions when game ends
 * @param pacman_x: Player's X coordinate
 * @param pacman_y: Player's Y coordinate
 */
void clearHearts(int pacman_x, int pacman_y) {
    // Reset hearts not collected by player
    if (pacman_x != heart1_x || pacman_y != heart1_y) {
        heart1_x = 40; heart1_y = 80;
    }
    if (pacman_x != heart2_x || pacman_y != heart2_y) {
        heart2_x = 60; heart2_y = 90;
    }
}

/******************************************************************************
 * Maze Definitions and Drawing
 *****************************************************************************/

// The maze layout (1 = wall, 0 = path)
const uint8_t maze[20][16] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,1,1,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,1,1,0,1,1,1,1,1,0,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,1},
    {1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,1,1,1,1,0,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1},
    {1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// New maze layout for level 2
const uint8_t maze_level2[20][16] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,0,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,0,1,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,1,0,0,0,1,0,1,0,0,0,0,1},
    {1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1},
    {1,1,1,0,1,1,1,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};


/******************************************************************************
 * Background and Maze Drawing Functions
 *****************************************************************************/
/**
 * Draws the game background and maze walls
 * Handles both level 1 and level 2 maze layouts
 */
void drawBackground(void) {
    // Set entire background to path color
    fillRectangle(0, 0, 128, 160, PATH_COLOR);
    
    // Select maze layout based on current level
    const uint8_t (*current_maze)[16] = (current_level == 1) ? maze : maze_level2;
    
    // Iterate through maze array and draw walls
    for(int y = 0; y < 20; y++) {
        for(int x = 0; x < 16; x++) {
            if(current_maze[y][x] == 1) {
                // Draw wall block at calculated position
                fillRectangle(x * WALL_SIZE, y * WALL_SIZE, 
                            WALL_SIZE, WALL_SIZE, WALL_COLOR);
            }
        }
    }
}

/**
 * Checks if a given position collides with maze walls
 * @param x: X coordinate to check
 * @param y: Y coordinate to check
 * @return: 1 if collision detected, 0 if path is clear
 */
int isWallCollision(uint16_t x, uint16_t y) {
    // Convert pixel coordinates to maze grid coordinates
    int gridX = x / WALL_SIZE;
    int gridY = y / WALL_SIZE;
    
    // Check if position is within maze bounds
    if(gridX >= 0 && gridX < 16 && gridY >= 0 && gridY < 20) {
        return maze[gridY][gridX] == 1;
    }
    return 1;  // Treat out-of-bounds as wall collision
}

/******************************************************************************
 * Menu System Variables and Functions
 *****************************************************************************/
// Menu state tracking
int in_menu = 1;           // Start in menu state
int selected_option = 0;   // Currently selected menu option
#define NUM_MENU_OPTIONS 3 // Total number of menu options

/**
 * Draws decorative border around menu screens
 */
void drawMenuBorder() {
    // Draw horizontal borders
    for(int i = 0; i < 128; i++) {
        putPixel(i, 0, BORDER_COLOR);      // Top border
        putPixel(i, 159, BORDER_COLOR);    // Bottom border
    }
    // Draw vertical borders
    for(int i = 0; i < 160; i++) {
        putPixel(0, i, BORDER_COLOR);      // Left border
        putPixel(127, i, BORDER_COLOR);    // Right border
    }
}

/**
 * Draws main menu screen with animated elements
 */
void drawMenu() {
    // Create gradient background effect
    for(int y = 0; y < 160; y++) {
        uint16_t color = RGBToWord(0, 0, (y/2) % 32);
        for(int x = 0; x < 128; x++) {
            putPixel(x, y, color);
        }
    }
    
    drawMenuBorder();  // Add border
    
    // Add decorative hearts in corners
    putImage(5, 5, 12, 16, pacmanheart, 0, 0);     // Top-left
    putImage(111, 5, 12, 16, pacmanheart, 0, 0);   // Top-right
    putImage(5, 139, 12, 16, pacmanheart, 0, 0);   // Bottom-left
    putImage(111, 139, 12, 16, pacmanheart, 0, 0); // Bottom-right
    
    // Draw title with shadow effect
    printTextX2("HEART", 36, 21, 0, 0);            // Shadow
    printTextX2("HEART", 35, 20, TITLE_COLOR, 0);  // Main text
    printTextX2("CHASE", 36, 41, 0, 0);            // Shadow
    printTextX2("CHASE", 35, 40, TITLE_COLOR, 0);  // Main text
    
    // Add separator line
    for(int i = 20; i < 108; i++) {
        putPixel(i, 65, BORDER_COLOR);
    }
    
    // Menu options with animation
    const char* options[] = {"Start Game", "Controls", "Credits"};
    static int animation_offset = 0;
    animation_offset = (animation_offset + 1) % 4;
    
    // Draw each menu option
    for(int i = 0; i < NUM_MENU_OPTIONS; i++) {
        uint16_t color = (i == selected_option) ? SELECTED_COLOR : UNSELECTED_COLOR;
        int y_pos = 80 + (i * 20);
        
        // Animated selection box for current option
        if(i == selected_option) {
            int box_x = 35 - animation_offset;
            int box_width = 60 + (animation_offset * 2);
            fillRectangle(box_x, y_pos - 2, box_width, 12, RGBToWord(0, 0, 32));
        }
        
        printText(options[i], 40, y_pos, color, 0);
    }
    
    // Animated Pacman indicator
    static int pac_toggle = 0;
    pac_toggle ^= 1;
    putImage(20, 78 + (selected_option * 20), 12, 16, 
             pac_toggle ? pac1 : pacman2, 0, 0);
}

/**
 * Displays game controls screen
 */
void showControls() {
    fillRectangle(0, 0, 128, 160, 0);  // Clear screen
    drawMenuBorder();
    
    // Title with shadow
    printTextX2("CONTROLS", 21, 21, 0, 0);           // Shadow
    printTextX2("CONTROLS", 20, 20, TITLE_COLOR, 0); // Main text
    
    // Separator lines
    for(int i = 20; i < 108; i++) {
        putPixel(i, 35, BORDER_COLOR);
        putPixel(i, 120, BORDER_COLOR);
    }
    
    // Control instructions
    printText("Movement:", 20, 45, SELECTED_COLOR, 0);
    printText("↑ Up Arrow", 30, 60, UNSELECTED_COLOR, 0);
    printText("↓ Down Arrow", 30, 75, UNSELECTED_COLOR, 0);
    printText("← Left Arrow", 30, 90, UNSELECTED_COLOR, 0);
    printText("→ Right Arrow", 30, 105, UNSELECTED_COLOR, 0);
    
    // Blinking return instruction
    static int blink = 0;
    blink ^= 1;
    if(blink) {
        printText("Press RIGHT to return", 15, 130, SELECTED_COLOR, 0);
    }
}

/**
 * Displays credits screen
 */
void showCredits() {
    fillRectangle(0, 0, 128, 160, 0);  // Clear screen
    drawMenuBorder();
    
    // Title with shadow
    printTextX2("CREDITS", 31, 21, 0, 0);           // Shadow
    printTextX2("CREDITS", 30, 20, TITLE_COLOR, 0); // Main text
    
    // Separator lines
    for(int i = 20; i < 108; i++) {
        putPixel(i, 35, BORDER_COLOR);
        putPixel(i, 120, BORDER_COLOR);
    }
    
    // Credits content
    printText("Heart Chase", 30, 50, SELECTED_COLOR, 0);
    printText("Created by:", 30, 70, UNSELECTED_COLOR, 0);
    printText("V, C, J", 35, 85, SELECTED_COLOR, 0);
    
    // Blinking return instruction
    static int blink = 0;
    blink ^= 1;
    if(blink) {
        printText("Press RIGHT to return", 15, 130, SELECTED_COLOR, 0);
    }
    
    // Decorative hearts
    putImage(10, 45, 12, 16, pacmanheart, 0, 0);   // Left heart
    putImage(106, 45, 12, 16, pacmanheart, 0, 0);  // Right heart
}

/******************************************************************************
 * Game Over and Victory Screen Functions
 *****************************************************************************/

/**
 * Displays the game over or victory menu
 * Allows player to choose between replaying or returning to main menu
 */
void drawGameOverMenu() {
    // Create dark background panel
    fillRectangle(20, 50, 88, 60, RGBToWord(0, 0, 32));
    
    // Draw decorative border around panel
    for(int i = 20; i < 108; i++) {
        putPixel(i, 50, RGBToWord(0, 0, 0xff));   // Top border
        putPixel(i, 110, RGBToWord(0, 0, 0xff));  // Bottom border
    }
    for(int i = 50; i < 110; i++) {
        putPixel(20, i, RGBToWord(0, 0, 0xff));   // Left border
        putPixel(107, i, RGBToWord(0, 0, 0xff));  // Right border
    }
    
    // Display appropriate status message
    if (game_won) {
        printText("YOU WIN!", 40, 60, RGBToWord(0, 0xff, 0), 0);  // Green for win
    } else {
        printText("GAME OVER", 35, 60, RGBToWord(0xff, 0, 0), 0); // Red for loss
    }
    
    // Set colors for menu options based on selection
    uint16_t play_color = (game_over_selection == 0) ? 
        RGBToWord(0xff, 0xff, 0) : RGBToWord(0xff, 0xff, 0xff);  // Yellow/White
    uint16_t menu_color = (game_over_selection == 1) ? 
        RGBToWord(0xff, 0xff, 0) : RGBToWord(0xff, 0xff, 0xff);  // Yellow/White
    
    // Draw menu options
    printText("Play Again", 35, 80, play_color, 0);
    printText("Main Menu", 35, 95, menu_color, 0);
    
    // Draw Pacman indicator for current selection
    putImage(25, 78 + (game_over_selection * 15), 12, 16, pac1, 0, 0);
}

/**
 * Displays an animated victory celebration screen
 * Shows congratulatory messages and final score
 */
void showWinScreen() {
    // Create animated gradient background
    for(int y = 0; y < 160; y++) {
        uint16_t color = RGBToWord(0, 0, (y/2) % 64);  // Dark blue gradient
        for(int x = 0; x < 128; x++) {
            putPixel(x, y, color);
        }
    }

    // Play victory fanfare
    playNote(800);  delay(200);  // Low note
    playNote(1000); delay(200);  // Mid-low note
    playNote(1200); delay(200);  // Mid-high note
    playNote(1500); delay(400);  // High note
    playNote(0);                 // Stop sound

    // Draw golden decorative border
    for(int i = 0; i < 128; i++) {
        putPixel(i, 0, WIN_GOLD);    // Top border
        putPixel(i, 159, WIN_GOLD);  // Bottom border
    }
    for(int i = 0; i < 160; i++) {
        putPixel(0, i, WIN_GOLD);    // Left border
        putPixel(127, i, WIN_GOLD);  // Right border
    }

    // Animate hearts appearing in corners
    for(int i = 0; i < 4; i++) {
        delay(100);  // Pause between each heart
        switch(i) {
            case 0: putImage(5, 5, 12, 16, pacmanheart, 0, 0);     // Top-left
            case 1: putImage(111, 5, 12, 16, pacmanheart, 0, 0);   // Top-right
            case 2: putImage(5, 139, 12, 16, pacmanheart, 0, 0);   // Bottom-left
            case 3: putImage(111, 139, 12, 16, pacmanheart, 0, 0); // Bottom-right
        }
    }

    // Display main victory text with shadow effect
    const char* win_text = "YOU WIN!";
    int text_x = 25;
    int text_y = 40;
    
    printTextX2(win_text, text_x + 1, text_y + 1, 0, 0);      // Shadow
    printTextX2(win_text, text_x, text_y, WIN_GOLD, 0);       // Main text
    
    delay(500);  // Pause for emphasis

    // Animate separator lines
    for(int i = 20; i < 108; i++) {
        putPixel(i, 65, WIN_PINK);           // Upper line
        putPixel(107-i+20, 95, WIN_PINK);    // Lower line
        if(i % 4 == 0) delay(1);             // Slow animation
    }

    // Display congratulatory messages with fade-in effect
    const char* messages[] = {
        "CONGRATULATIONS!",
        "ALL HEARTS",
        "COLLECTED!",
        "YOU'RE Eating!"
    };
    
    // Show each message with alternating colors
    for(int i = 0; i < 4; i++) {
        delay(200);  // Pause between messages
        int y_pos = 70 + (i * 20);
        printText(messages[i], 
                 64 - (strlen(messages[i]) * 3), // Center text
                 y_pos, 
                 (i % 2) ? WIN_PINK : WIN_BLUE,  // Alternate colors
                 0);
    }

    // Display final level count
    delay(200);
    char score_text[20];
    sprintf(score_text, "LEVELS: %d", current_level);
    printText(score_text, 40, 140, WIN_GOLD, 0);

    // Log victory to serial output
    eputs("Game Won! All hearts collected in both levels!\r\n");
}
/******************************************************************************
 * Main Function - Game Initialization
 *****************************************************************************/
int main() {
    // Movement and direction control flags
    int hinverted = 0;    // Horizontal flip flag
    int vinverted = 0;    // Vertical flip flag
    int toggle = 0;       // Animation toggle
    int hmoved = 0;       // Horizontal movement flag
    int vmoved = 0;       // Vertical movement flag
    
    // Player position tracking
    uint16_t x = 50;      // Current X position
    uint16_t y = 50;      // Current Y position
    uint16_t oldx = x;    // Previous X position
    uint16_t oldy = y;    // Previous Y position

    /*** Hardware Initialization ***/
    initClock();          // Initialize system clock
    initSysTick();        // Initialize system timer
    setupIO();            // Setup input/output pins
    initSerial();         // Initialize serial communication
    initEnemies();        // Initialize enemy positions and states
    //initSound();

    /*** Initial Game Setup ***/
    // Place hearts in starting positions
    putImage(heart1_x, heart1_y, 12, 16, pacmanheart, 0, 0);   // First heart
    putImage(heart2_x, heart2_y, 12, 16, pacmanheart2, 0, 0);  // Second heart

    /*** Sound System Setup ***/
    // Play startup tune
    playTune(my_notes, my_note_times, 5);
    
    // Configure background music
    background_tune_notes = my_notes;
    background_tune_times = my_note_times;
    background_tune_note_count = 5;
    background_repeat_tune = 1;

    /*** Draw Initial Game Screen ***/
    drawBackground();     // Draw maze and game environment

/******************************************************************************
 * Main Game Loop
 *****************************************************************************/
    while (1) {
        // Handle serial input
        char serial_char = serial_available() ? egetchar() : 0;

        /*** Menu State Handling ***/
        if (in_menu) {
            // Draw menu on first entry
            static int menu_drawn = 0;
            if (!menu_drawn) {
                drawMenu();
                menu_drawn = 1;
            }

            // Menu Navigation Controls
            // Down Button
            if ((GPIOA->IDR & (1 << 11)) == 0) {
                delay(200);  // Prevent multiple inputs
                selected_option = (selected_option + 1) % NUM_MENU_OPTIONS;
                drawMenu();
            }
            // Up Button
            if ((GPIOA->IDR & (1 << 8)) == 0) {
                delay(200);
                selected_option = (selected_option - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;
                drawMenu();
            }
            // Select Button (Right/Enter)
            if ((GPIOB->IDR & (1 << 4)) == 0) {
                delay(200);
                
                switch(selected_option) {
                    case 0:  // Start New Game
                        // Reset game state and screen
                        in_menu = 0;
                        menu_drawn = 0;
                        fillRectangle(0, 0, 128, 160, 0);  // Clear screen
                        drawBackground();                   // Draw maze
                        initEnemies();                     // Setup enemies
                        
                        // Reset game flags
                        game_over = 0;
                        game_won = 0;
                        heart1_eaten = 0;
                        heart2_eaten = 0;
                        
                        // Reset positions
                        x = 50; y = 50;           // Player position
                        oldx = x; oldy = y;       // Previous position
                        heart1_x = 40; heart1_y = 80;  // Heart 1 position
                        heart2_x = 60; heart2_y = 90;  // Heart 2 position
                        
                        // Draw hearts
                        putImage(heart1_x, heart1_y, 12, 16, pacmanheart, 0, 0);
                        putImage(heart2_x, heart2_y, 12, 16, pacmanheart2, 0, 0);
                        break;
                        
                    case 1:  // Show Controls Screen
                        showControls();
                        while((GPIOB->IDR & (1 << 4)) != 0) {}  // Wait for button release
                        delay(200);
                        drawMenu();
                        break;
                        
                    case 2:  // Show Credits Screen
                        showCredits();
                        while((GPIOB->IDR & (1 << 4)) != 0) {}
                        delay(200);
                        drawMenu();
                        break;
                }
            }
            continue;  // Skip game logic while in menu
        }

        /*** Game State Updates ***/
        hmoved = vmoved = 0;           // Reset movement flags
        hinverted = vinverted = 0;     // Reset direction flags
        
        checkWinCondition(x, y);       // Check if level complete
        if(!game_won) {
            moveEnemies(x, y);         // Update enemy positions
        }

        /*** Player Movement ***/
        if (!game_over && !game_won) {
            // Right Movement
            if (((GPIOB->IDR & (1 << 4)) == 0) || (serial_char == 'r')) {
                if (x < 115) {
                    x = x + 1;
                    hmoved = 1;
                    hinverted = 0;
                }
            }
            // Left Movement
            if ((GPIOB->IDR & (1 << 5)) == 0) {
                if (x > 2) {
                    x = x - 1;
                    hmoved = 1;
                    hinverted = 1;
                }
            }
            // Down Movement
            if ((GPIOA->IDR & (1 << 11)) == 0) {
                if (y < 144) {
                    y = y + 1;
                    vmoved = 1;
                    vinverted = 0;
                }
            }
            // Up Movement
            if ((GPIOA->IDR & (1 << 8)) == 0) {
                if (y > 1) {
                    y = y - 1;
                    vmoved = 1;
                    vinverted = 1;
                }
            }
        }

        /*** Update Player Position ***/
        if (vmoved || hmoved) {
            fillRectangle(oldx, oldy, 12, 16, 0);  // Clear old position
            oldx = x;
            oldy = y;

            // Draw player with appropriate sprite
            if (hmoved) {
                // Horizontal movement animation
                putImage(x, y, 12, 16, toggle ? pac1 : pacman2, hinverted, 0);
                toggle ^= 1;  // Switch animation frame
            } else {
                // Vertical movement sprite
                putImage(x, y, 12, 16, pacman3top, 0, vinverted);
            }

            /*** Heart Collection Checks ***/
            // Heart 1 Collection
            if (!heart1_eaten && isInside(heart1_x, heart1_y, 12, 16, x, y)) {
                printTextX2("Mahal Kita", 7, 20, RGBToWord(0xff, 0xff, 0), 0);
                fillRectangle(heart1_x, heart1_y, 12, 16, 0);
                heart1_eaten = 1;
                
                // Play collection sound
                playNote(500);
                delay(500);
                playNote(0);
                
                GPIOA->ODR |= (1 << 0);  // Turn on red LED
                eputs("Pacman eats the heart 1\r\n");
                checkWinCondition(x, y);
            }

            // Heart 2 Collection
            if (!heart2_eaten && isInside(heart2_x, heart2_y, 12, 16, x, y)) {
                printTextX2("Mama", 7, 40, RGBToWord(0xff, 0xff, 0), 0);
                fillRectangle(heart2_x, heart2_y, 12, 16, 0);
                heart2_eaten = 1;
                
                // Play collection sound
                playNote(500);
                delay(500);
                playNote(0);
                
                eputs("Pacman eats the heart 2\r\n");
                checkWinCondition(x, y);
            }
        }



/******************************************************************************
 * Heart Movement and Collision System
 *****************************************************************************/
        // Update hearts every 400ms if game is active
        if (milliseconds - heart_move_delay >= 400 && !game_over) {
            
            /*** Heart 1 Movement ***/
            if (!heart1_eaten) {
                // Remove heart from old position
                fillRectangle(heart1_x, heart1_y, 12, 16, 0);

                // Calculate random movement direction
                heart1_direction_x = (rand() % 3) - 1;  // -1, 0, or 1
                heart1_direction_y = (rand() % 3) - 1;
                
                // Move heart by 3 pixels in chosen direction
                heart1_x += 3 * heart1_direction_x;
                heart1_y += 3 * heart1_direction_y;

                // Keep heart within screen boundaries
                if (heart1_x <= 0) heart1_x = 0;
                if (heart1_x >= 115) heart1_x = 115;
                if (heart1_y <= 0) heart1_y = 0;
                if (heart1_y >= 144) heart1_y = 144;

                // Draw heart in new position
                putImage(heart1_x, heart1_y, 12, 16, pacmanheart, 0, 0);
            }

            /*** Heart 2 Movement ***/
            if (!heart2_eaten) {
                // Similar movement pattern as heart 1
                fillRectangle(heart2_x, heart2_y, 12, 16, 0);
                heart2_direction_x = (rand() % 3) - 1;
                heart2_direction_y = (rand() % 3) - 1;
                heart2_x += 3 * heart2_direction_x;
                heart2_y += 3 * heart2_direction_y;

                // Boundary checks
                if (heart2_x <= 0) heart2_x = 0;
                if (heart2_x >= 115) heart2_x = 115;
                if (heart2_y <= 0) heart2_y = 0;
                if (heart2_y >= 144) heart2_y = 144;

                putImage(heart2_x, heart2_y, 12, 16, pacmanheart2, 0, 0);
            }

            /*** Level 2 Additional Hearts ***/
            if (current_level == 2) {
                // Heart 3 Movement
                if (!heart3_eaten) {
                    fillRectangle(heart3_x, heart3_y, 12, 16, 0);
                    heart3_direction_x = (rand() % 3) - 1;
                    heart3_direction_y = (rand() % 3) - 1;
                    heart3_x += 3 * heart3_direction_x;
                    heart3_y += 3 * heart3_direction_y;
                    
                    // Boundary checks
                    if (heart3_x <= 0) heart3_x = 0;
                    if (heart3_x >= 115) heart3_x = 115;
                    if (heart3_y <= 0) heart3_y = 0;
                    if (heart3_y >= 144) heart3_y = 144;
                    
                    putImage(heart3_x, heart3_y, 12, 16, pacmanheart, 0, 0);
                }

                // Heart 4 Movement
                if (!heart4_eaten) {
                    // Similar pattern as other hearts
                    fillRectangle(heart4_x, heart4_y, 12, 16, 0);
                    heart4_direction_x = (rand() % 3) - 1;
                    heart4_direction_y = (rand() % 3) - 1;
                    heart4_x += 3 * heart4_direction_x;
                    heart4_y += 3 * heart4_direction_y;
                    
                    // Boundary checks
                    if (heart4_x <= 0) heart4_x = 0;
                    if (heart4_x >= 115) heart4_x = 115;
                    if (heart4_y <= 0) heart4_y = 0;
                    if (heart4_y >= 144) heart4_y = 144;
                    
                    putImage(heart4_x, heart4_y, 12, 16, pacmanheart, 0, 0);
                }
            }

            /*** Enemy Collision Check ***/
            if(!game_won && checkEnemyCollision(x, y)) {
                game_over = 1;  // Game ends if player hits enemy
                
                // Clear entire screen
                fillRectangle(0, 0, 128, 160, 0);

                // Remove all active enemies
                for(int i = 0; i < MAX_ENEMIES; i++) {
                    if(enemies[i].active) {
                        fillRectangle(enemies[i].x, enemies[i].y, 12, 16, 0);
                        enemies[i].active = 0;
                    }
                }

                // Clear remaining hearts
                fillRectangle(heart1_x, heart1_y, 12, 16, 0);
                fillRectangle(heart2_x, heart2_y, 12, 16, 0);

                // Remove player sprite
                fillRectangle(x, y, 12, 16, 0);

                // Show game over menu
                show_game_over_menu = 1;
                game_over_selection = 0;
                drawGameOverMenu();
            }

            // Reset movement timer
            heart_move_delay = milliseconds;
        }
        /******************************************************************************
         * Level 2 Heart Collection Logic
         *****************************************************************************/
        if (current_level == 2) {
            // Check collision with Heart 3
            if (!heart3_eaten && isInside(heart3_x, heart3_y, 12, 16, x, y)) {
                printTextX2("Heart 3!", 7, 60, RGBToWord(0xff, 0xff, 0), 0);  // Show collection message
                fillRectangle(heart3_x, heart3_y, 12, 16, 0);                 // Remove heart from screen
                heart3_eaten = 1;                                             // Mark as collected
                
                // Play collection sound effect
                playNote(500);
                delay(500);
                playNote(0);
                
                eputs("Pacman eats heart 3\r\n");                            // Log to serial
                checkWinCondition(x, y);                                     // Check if level complete
            }

            // Check collision with Heart 4 (same pattern as Heart 3)
            if (!heart4_eaten && isInside(heart4_x, heart4_y, 12, 16, x, y)) {
                printTextX2("Heart 4!", 7, 80, RGBToWord(0xff, 0xff, 0), 0);
                fillRectangle(heart4_x, heart4_y, 12, 16, 0);
                heart4_eaten = 1;
                
                playNote(500);
                delay(500);
                playNote(0);
                
                eputs("Pacman eats heart 4\r\n");
                checkWinCondition(x, y);
            }
        }

        /******************************************************************************
         * Game Over/Win Menu Navigation
         *****************************************************************************/
        if (game_over || game_won) {
            // Handle menu navigation with up/down buttons
            if ((GPIOA->IDR & (1 << 11)) == 0) {  // Down pressed
                delay(200);                        // Debounce delay
                game_over_selection = !game_over_selection;  // Toggle selection
                drawGameOverMenu();                // Redraw menu
            }
            if ((GPIOA->IDR & (1 << 8)) == 0) {   // Up pressed
                delay(200);
                game_over_selection = !game_over_selection;
                drawGameOverMenu();
            }

            // Handle selection confirmation
            if ((GPIOB->IDR & (1 << 4)) == 0) {   // Right/Enter pressed
                delay(200);
                
                if (game_over_selection == 0) {    // "Play Again" selected
                    // Reset game state to level 1
                    current_level = 1;
                    game_over = 0;
                    game_won = 0;
                    heart1_eaten = 0;
                    heart2_eaten = 0;
                    heart3_eaten = 0;
                    heart4_eaten = 0;
                    show_game_over_menu = 0;
                    
                    // Reset player and heart positions
                    x = 50;
                    y = 50;
                    oldx = x;
                    oldy = y;
                    heart1_x = 40;
                    heart1_y = 80;
                    heart2_x = 60;
                    heart2_y = 90;
                    
                    // Reset game display
                    fillRectangle(0, 0, 128, 160, 0);    // Clear screen
                    drawBackground();                     // Draw maze
                    initEnemies();                       // Reset enemies
                    // Draw initial hearts
                    putImage(heart1_x, heart1_y, 12, 16, pacmanheart, 0, 0);
                    putImage(heart2_x, heart2_y, 12, 16, pacmanheart2, 0, 0);
                    
                } else {  // "Main Menu" selected
                    in_menu = 1;                         // Return to main menu
                    show_game_over_menu = 0;             // Hide game over menu
                    current_level = 1;                   // Reset to level 1
                }
            }
        }

        delay(20);  // Control game speed
    }

    return 0;  // End of main function
}


void initSysTick(void)
{
	SysTick->LOAD = 48000;
	SysTick->CTRL = 7;
	SysTick->VAL = 10;
	__asm(" cpsie i "); // enable interrupts
}
void SysTick_Handler(void)
{
	milliseconds++;
    static int index = 0;
    static int current_note_timer;
    //Background tune handler
    if (background_tune_notes !=0)
    {
        //if there is no tune
        if (current_note_timer == 0)
        {
            //. move on to the next note (if there is none)
            index ++;
            if (index >= background_tune_note_count)
            {
                // no more notes
                if (background_repeat_tune == 0)
                {
                    //dont repeat
                    background_tune_notes = 0;
                }
                else
                {
                    index = 0; //reset to first note and repeat
                }
            }
            else
            {
                current_note_timer = background_tune_times[index];
                playNote(background_tune_notes[index]);
            }
        }
        if(current_note_timer != 0)
        {
            current_note_timer --;
        }


    }
}
void initClock(void)
{
// This is potentially a dangerous function as it could
// result in a system with an invalid clock signal - result: a stuck system
        // Set the PLL up
        // First ensure PLL is disabled
        RCC->CR &= ~(1u<<24);
        while( (RCC->CR & (1 <<25))); // wait for PLL ready to be cleared
        
// Warning here: if system clock is greater than 24MHz then wait-state(s) need to be
// inserted into Flash memory interface
				
        FLASH->ACR |= (1 << 0);
        FLASH->ACR &=~((1u << 2) | (1u<<1));
        // Turn on FLASH prefetch buffer
        FLASH->ACR |= (1 << 4);
        // set PLL multiplier to 12 (yielding 48MHz)
        RCC->CFGR &= ~((1u<<21) | (1u<<20) | (1u<<19) | (1u<<18));
        RCC->CFGR |= ((1<<21) | (1<<19) ); 

        // Need to limit ADC clock to below 14MHz so will change ADC prescaler to 4
        RCC->CFGR |= (1<<14);

        // and turn the PLL back on again
        RCC->CR |= (1<<24);        
        // set PLL as system clock source 
        RCC->CFGR |= (1<<1);
}
void delay(volatile uint32_t dly)
{
	uint32_t end_time = dly + milliseconds;
	while(milliseconds != end_time)
		__asm(" wfi "); // sleep
}

void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
int isInside(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t px, uint16_t py)
{
	// checks to see if point px,py is within the rectange defined by x,y,w,h
	uint16_t x2,y2;
	x2 = x1+w;
	y2 = y1+h;
	int rvalue = 0;
	if ( (px >= x1) && (px <= x2))
	{
		// ok, x constraint met
		if ( (py >= y1) && (py <= y2))
			rvalue = 1;
	}
	return rvalue;
}

void setupIO()
{
	RCC->AHBENR |= (1 << 18) + (1 << 17); // enable Ports A and B
	display_begin();
	pinMode(GPIOB,4,0);
	pinMode(GPIOB,5,0);
	pinMode(GPIOA,8,0);
	pinMode(GPIOA,11,0);
    pinMode(GPIOA,12,0);//for the red led cant get it working
	enablePullUp(GPIOB,4);
	enablePullUp(GPIOB,5);
	enablePullUp(GPIOA,11);
	enablePullUp(GPIOA,8);
}












