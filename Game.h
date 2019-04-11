/*
 * Game.h
 *
 *  Created on: Feb 27, 2017
 *      Author: danny
 */

#ifndef GAME_H_
#define GAME_H_

/*********************************************** Includes ********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "G8RTOS/G8RTOS.h"
#include "cc3100_usage.h"
#include <G8RTOS_IPC.h>
#include "LCDLib.h"
/*********************************************** Includes ********************************************************************/

/*********************************************** Externs ********************************************************************/

/* Semaphores here */ 
semaphore_t cc3100;
semaphore_t lcd;
semaphore_t player;

/*********************************************** Externs ********************************************************************/

/*********************************************** Global Defines ********************************************************************/

#define MAX_NUM_OF_PLAYERS  2
#define MAX_NUM_OF_BALLS    8

// This game can actually be played with 4 players... a little bit more challenging, but doable! 
#define NUM_OF_PLAYERS_PLAYING 2

/* Size of game arena */
#define ARENA_MIN_X                  40
#define ARENA_MAX_X                  280
#define ARENA_MIN_Y                  0
#define ARENA_MAX_Y                  240

/* Size of objects */
#define PADDLE_LEN                   64
#define PADDLE_LEN_D2                (PADDLE_LEN >> 1)
#define PADDLE_WID                   4
#define PADDLE_WID_D2                (PADDLE_WID >> 1)
#define BALL_SIZE                    4
#define BALL_SIZE_D2                 (BALL_SIZE >> 1)

/* Centers for paddles at the center of the sides */
#define PADDLE_X_CENTER              MAX_SCREEN_X >> 1

/* Edge limitations for player's center coordinate */
#define HORIZ_CENTER_MAX_PL          (ARENA_MAX_X - PADDLE_LEN_D2)
#define HORIZ_CENTER_MIN_PL          (ARENA_MIN_X + PADDLE_LEN_D2)

/* Constant centers of each player */
#define TOP_PLAYER_CENTER_Y          (ARENA_MIN_Y + PADDLE_WID_D2)
#define BOTTOM_PLAYER_CENTER_Y       (ARENA_MAX_Y - PADDLE_WID_D2)

/* Edge coordinates for paddles */
#define TOP_PADDLE_EDGE              (ARENA_MIN_Y + PADDLE_WID)
#define BOTTOM_PADDLE_EDGE           (ARENA_MAX_Y - PADDLE_WID)

/* Amount of allowable space for collisions with the sides of paddles */
#define WIGGLE_ROOM                  2

/* Value for velocities from contact with paddles */
#define _1_3_PADDLE                  11

/* Defines for Minkowski Alg. for collision */
#define WIDTH_TOP_OR_BOTTOM          ((PADDLE_LEN + BALL_SIZE) >> 1) + WIGGLE_ROOM
#define HEIGHT_TOP_OR_BOTTOM         ((PADDLE_WID + BALL_SIZE) >> 1) + WIGGLE_ROOM

/* Edge limitations for ball's center coordinate */
#define HORIZ_CENTER_MAX_BALL        (ARENA_MAX_X - BALL_SIZE_D2)
#define HORIZ_CENTER_MIN_BALL        (ARENA_MIN_X + BALL_SIZE_D2)
#define VERT_CENTER_MAX_BALL         (ARENA_MAX_Y - BALL_SIZE_D2)
#define VERT_CENTER_MIN_BALL         (ARENA_MIN_Y + BALL_SIZE_D2)

/* Maximum ball speed */
#define MAX_BALL_SPEED               6

/* Background color - Black */
#define BACK_COLOR                   LCD_BLACK

/* Offset for printing player to avoid blips from left behind ball */
#define PRINT_OFFSET                10

/* Used as status LEDs for Wi-Fi */
#define BLUE_LED BIT2
#define RED_LED BIT0

#define CLIENT 0
#define HOST 1

/* Used for different FIFO names */
#define JOYSTICK_CLIENTFIFO CLIENT
#define JOYSTICK_HOSTFIFO HOST

/* Used for starting position of snakes */
#define SNAKESTART_CLIENTX 60
#define SNAKESTART_HOSTX 260
#define SNAKESTART_Y 120


/* Enums for player colors */
typedef enum
{
    PLAYER_RED = LCD_RED,
    PLAYER_BLUE = LCD_BLUE
}playerColor;

/* Enums for player numbers */
typedef enum
{
    BOTTOM = 0,
    TOP = 1
}playerPosition;

typedef enum
{
    CNONE = -1,
    CTOP = 0,
    CBOTTOM = 1,
    CRIGHT = 2,
    CLEFT = 3
}collisionPosition;

/*********************************************** Global Defines ********************************************************************/

/*********************************************** Data Structures ********************************************************************/
/*********************************************** Data Structures ********************************************************************/
#pragma pack ( push, 1)
/*
 * Struct to be sent from the client to the host
 */
typedef struct
{
    uint32_t IP_address;
    int16_t displacementX;
    int16_t displacementY;
    uint8_t playerNumber;
    bool ready;
    bool joined;
    bool acknowledge;
} SpecificPlayerInfo_t;

/*
 * General player info to be used by both host and client
 * Client responsible for translation
 */
typedef struct
{
    int16_t headX;
    int16_t headY;
    int16_t tailX;
    int16_t tailY;
    uint16_t color;
    int16_t size;
    int16_t currentSize;
} GeneralPlayerInfo_t;

/*
 * Struct of all the balls, only changed by the host
 */
typedef struct
{
    int16_t currentCenterX;
    int16_t currentCenterY;
} Ball_t;

/*
 * Struct to be sent from the host to the client
 */
typedef struct
{
    SpecificPlayerInfo_t player;
    GeneralPlayerInfo_t players[MAX_NUM_OF_PLAYERS];
    Ball_t snack;
    bool winner;
    bool gameDone;
} GameState_t;
#pragma pack ( pop )

/*
 * Struct of all the previous ball locations, only changed by self for drawing!
 */
typedef struct
{
    int16_t CenterX;
    int16_t CenterY;
}PrevBall_t;

/*
 * Struct of all the previous players locations, only changed by self for drawing
 */
typedef struct
{
    int16_t headX;
    int16_t headY;
}PrevPlayer_t;

typedef struct{
    int16_t height;
    int16_t width;
    int16_t centerX;
    int16_t centerY;
}CollsionPos_t;

/*********************************************** Data Structures ********************************************************************/


/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame();

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost();

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost();

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient();

/*
 * End of game for the client
 */
void EndOfGameClient();

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame();

/*
 * Thread that sends game state to client
 */
void SendDataToClient();

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient();

/*
 * Generate Ball thread
 */
void GenerateBall();

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost();

/*
 * Thread to move a single ball
 */
void MoveBall();

/*
 * End of game for the host
 */
void EndOfGameHost();

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread();

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects();

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs();

/*
 * Thread to wait for host/client selection
 */
void WaitInit();
/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player);

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer);

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor);

/*
 * Initializes and prints initial game state
 */
void InitBoardState();

collisionPosition collision(CollsionPos_t A, CollsionPos_t B);

/*********************************************** Public Functions *********************************************************************/


#endif /* GAME_H_ */
