/*
 * Game.c
 *
 *
 *      Author: Scarlett
 */
#include "Game.h"
#include "BSP.h"
#include "LCDLib.h"
#include <time.h>
#include <stdlib.h>


volatile SpecificPlayerInfo_t client_info;
volatile SpecificPlayerInfo_t host_info[2];
SpecificPlayerInfo_t received_player_info;

volatile uint8_t Num_of_current_balls = 0;
volatile uint8_t Num_current_players = 0;

volatile GameState_t game_state_clt;
volatile GameState_t game_state;
int32_t received_game_state;
int32_t received_game_state2;
int32_t received_game_state_cltEnd;

bool game_done_var;

volatile uint16_t game_is_done = false;

volatile GeneralPlayerInfo_t gen_players_info[MAX_NUM_OF_PLAYERS];

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
    P2->OUT &= ~BLUE_LED;
    game_state.gameDone = false;
    //game_is_done = false;

    while(1)
    {
        client_info.IP_address = getLocalIP();
        client_info.displacement = 0;
        client_info.playerNumber = Client; // #0
        client_info.ready = false;
        client_info.joined = false;
        client_info.acknowledge = false;

        while(!client_info.joined)
        {
            SendData(&client_info, HOST_IP_ADDR, sizeof(client_info));

            //received_game_state = ReceiveData(&game_state, sifeof(game_state));

            received_game_state = -1;
            while(received_game_state < 0)
            {
                received_game_state = ReceiveData(&game_state, sifeof(game_state));
            }

            if(client_info.joined)
            {
                client_info.acknowledge = true;
            }

            SendData(&client_info, HOST_IP_ADDR, sizeof(client_info));
        }

        P2->OUT |= BLUE_LED;
        InitBoardState();

        G8RTOS_InitSemaphore(&wifiSemaphore, 1);
        G8RTOS_InitSemaphore(&lcdSemaphore, 1);
        G8RTOS_InitSemaphore(&ledSemaphore, 1);

        G8RTOS_AddThread(ReadJoystickClient, 3, "joystickC");
        G8RTOS_AddThread(SendDataToHost, 3, "dataToHost");
        G8RTOS_AddThread(ReceiveDataFromHost, 3, "recvDataHost");
        G8RTOS_AddThread(DrawObjects, 3, "drawObj");
        G8RTOS_AddThread(MoveLEDs, 4, "leds");
        G8RTOS_AddThread(IdleThread, 100, "idle");

        G8RTOS_KillSelf();
    }
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
    while(1)
    {
        /*G8RTOS_WaitSemaphore(&wifiSemaphore);
        received_game_state2 = ReceiveData(&game_state, sizeof(game_state));
        G8RTOS_SignalSemaphore(&wifiSemaphore);*/

        received_game_state2 = -1;
        while(received_game_state2 < 0)
        {
            G8RTOS_WaitSemaphore(&wifiSemaphore);
            received_game_state2 = ReceiveData(&game_state, sizeof(game_state));
            G8RTOS_SignalSemaphore(&wifiSemaphore);
        }

        G8RTOS_WaitSemaphore(&lcdSemaphore);
        game_done_var = game_state.gameDone;//game_is_done;
        G8RTOS_SignalSemaphore(&lcdSemaphore);

        if(game_done_var)
        {
            G8RTOS_AddThread(EndOfGameClient, 1, "endOfGameClient");
        }

        sleep(5);
    }

}
/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&lcdSemaphore);

        G8RTOS_WaitSemaphore(&wifiSemaphore);
        SendData(&client_info, HOST_IP_ADDR, sizeof(client_info));
        G8RTOS_SignalSemaphore(&wifiSemaphore);

        client_info.displacement = 0;

        G8RTOS_SignalSemaphore(&lcdSemaphore);
        sleep(2);
    }
}
/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
    int16_t x_coord = 0;
    int16_t y_coord = 0;

    while(1)
    {
        GetJoystickCoordinates(&x_coord, &Y_coordinate);

        G8RTOS_WaitSemaphore(&lcdSemaphore);

        if (x_coord < 0)
        {
            client_info.displacement = MAX_SCREEN_X - 73 - (6805 + x_crd) / 87;
        }

        else if(x_coord > 0)
        {
            client_info.displacement = (8173 - x_coord)/160;
        }

        else
        {
            client_info.displacement = PADDLE_X_CENTER;
        }

        //X_coord
        //Y_coord
        G8RTOS_SignalSemaphore(&lcdSemaphore);

        sleep(10);
    }
}
/*
 * End of game for the client
 */
void EndOfGameClient();
{
    G8RTOS_WaitSemaphore(&wifiSemaphore);
    G8RTOS_WaitSemaphore(&lcdSemaphore);
    G8RTOS_WaitSemaphore(&ledSemaphore);
    G8RTOS_KillOtherThreads();
    G8RTOS_InitSemaphore(&wifiSemaphore, 1);
    G8RTOS_InitSemaphore(&lcdSemaphore, 1);
    G8RTOS_InitSemaphore(&ledSemaphore, 1);

    LP3943_LedModeSet(BLUE, game_state.LEDScores[TOP]);
    LP3943_LedModeSet(RED, game_state.LEDScores[BOTTOM]);

    if(game_state.winner = TOP)
    {
        LCD_DrawRectangle(MIN_SCREEN_X, MAX_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_Y, LCD_BLUE);
        LCD_Text(0, 100, "Blue player is the winner.", LCD_WHITE);
    }
    else
    {
        LCD_DrawRectangle(MIN_SCREEN_X, MAX_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_Y, LCD_RED);
        LCD_Text(0, 100, "Red player is the winner.", LCD_WHITE);
    }

    LCD_Text(0, 100, "Please wait for host to restart game.", LCD_WHITE);

    while(1)
    {
        received_game_state_cltEnd = -1;
        while(received_game_state_cltEnd < 0)
        {
            received_game_state_cltEnd = ReceiveData(&game_state, sizeof(game_state));
        }

        if (game_state.gameDone == false)    //game_is_done == false)
        {
            InitBoardState();

            G8RTOS_AddThread(ReadJoystickClient, 3, "joystickC");
            G8RTOS_AddThread(SendDataToHost, 3, "dataToHost");
            G8RTOS_AddThread(ReceiveDataFromHost, 3, "recvDataHost");
            G8RTOS_AddThread(DrawObjects, 3, "drawObj");
            G8RTOS_AddThread(MoveLEDs, 4, "leds");
            G8RTOS_AddThread(IdleThread, 100, "idle");
            G8RTOS_KillSelf();
        }
    }
}

/*********************************************** Client Threads *********************************************************************/
//SpecificPlayerInfo_t host_info;

/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame()
{
    initCC3100(Host);
    //initCC3100(Client);
    //GeneralPlayerInfo_t gen_players_info;
    P2DIR |= BLUE_LED;
    int32_t = received_info;
    //P2OUT &= BLUE_LED;

    //G8RTOS_Init();

    while(1)
    {
       G8RTOS_WaitSemaphore(&player_Sem);

       P2OUT &= BLUE_LED;
       Num_of_current_balls = 0;
       //game_is_done = false;
       game_state.gameDone = false;

       host_info[0].IP_address = HOST_IP_ADDR;
       host_info[0].displacement = 0;
       host_info[0].playerNumber = 0; //host
       host_info[0].ready = false;
       //host_info[0].joined = 0;
       host_info[0].acknowledge = true;

       host_info[1].acknowledge = false;

       gen_players_info[0].currentCenter = PADDLE_X_CENTER;
       gen_players_info[0].color = PLAYER_RED;
       gen_players_info[0].position = BOTTOM;
       gen_players_info[1].currentCenter = PADDLE_X_CENTER;
       gen_players_info[1].color = PLAYER_BLUE;
       gen_players_info[1].position = TOP;

       //Num_current_players++;
       Num_current_players = 1;

       while(!(host_info[1].acknowledge))  //client
       {
           received_info = ReceiveData(&received_player_info, sizeof(received_player_info));

           while(received_info < 0)
           {
               received_info = ReceiveData(&received_player_info, sizeof(received_player_info));
           }

           if(!(received_player_info.joined))
           {
               host_info[1].IP_address = received_player_info.IP_address;
               host_info[1].displacement = 0;
               host_info[1].playerNumber = 1; //1 client
               //host_info[1].ready = false;
               host_info[1].joined = true;

               SendData(&game_state_clt, host_info[1].IP_address, sizeof(game_state_clt));

               while(received_info < 0)
               {
                   received_info = ReceiveData(&received_player_info, sizeof(received_player_info));
               }

               if((received_player_info.acknowledge == 1) && (received_player_info.playerNumber == 1)) //client
               {
                   host_info[1].acknowledge = true;
               }
           }
       }

       Num_current_players++;

       G8RTOS_AddThread(GenerateBall, 3, "GenerateBall");
       G8RTOS_AddThread(DrawObjects, 3, "DrawObjects");
       G8RTOS_AddThread(ReadJoystickHost, 3, "DrawObjects");
       G8RTOS_AddThread(SendDataToHost, 3, "dataToHost");
       G8RTOS_AddThread(ReceiveDataFromHost, 3, "recvDataHost");
       G8RTOS_AddThread(DrawObjects, 3, "drawObj");
       G8RTOS_AddThread(MoveLEDs, 4, "leds");
       G8RTOS_AddThread(IdleThread, 100, "idle");

    }
}
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
void IdleThread()
{
    while(1);
}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects();

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs();

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole();

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



