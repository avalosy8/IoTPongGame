
/*********************************************** Includes ********************************************************************/
#include "Game.h"
/*********************************************** Includes ********************************************************************/

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game, only thread for Client before launching OS
 */
void JoinGame()
{
    initCC3100(Client);

    // set up struct to be sent to Host
    clientInfo.IP_address = getLocalIP();
    clientInfo.displacement = 0;
    clientInfo.playerNumber = Client;
    clientInfo.ready = false;
    clientInfo.joined = true;
    clientInfo.acknowledge = false;


}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{

}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{

}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{

}

/*
 * End of game for the client
 */
void EndOfGameClient()
{

}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/



/*
 * Thread for the host to create a game, only thread for Host before launching OS
 */
void CreateGame()
{
    // B0 top button for host
    P4->DIR &= ~BIT4;       // set as input
    P4->IFG &= ~BIT4;       // clear IFG flag on P4.4
    P4->IE |= BIT4;         // enable interrupt on P4.4
    P4->IES |= BIT4;        // high-to-low transition, falling edge interrupt
    P4->REN |= BIT4;        // pull-up resistor
    P4->OUT |= BIT4;        // sets res to pull-up

    NVIC_EnableIRQ(PORT4_IRQn); // enable PORT4 interrupt using enable bits controlled by registers in NVIC

    // B1 bottom button for client
    P5->DIR &= ~BIT4;       // set as input
    P5->IFG &= ~BIT4;       // clear IFG flag on P5.4
    P5->IE |= BIT4;         // enable interrupt on P5.4
    P5->IES |= BIT4;        // high-to-low transition, falling edge interrupt
    P5->REN |= BIT4;        // pull-up resistor
    P5->OUT |= BIT4;        // sets res to pull-up

    NVIC_EnableIRQ(PORT5_IRQn); // enable PORT4 interrupt using enable bits controlled by registers in NVIC

    // for toggling LED2
    P2->SEL0 &= ~BIT2;  // blue
    P2->SEL1 &= ~BIT2;
    P2->DIR |= BIT2;
    P2->OUT &= ~BIT2;

    // init players
    isHost = false;
    isClient = false;

    LCD_Init(false);

    LCD_Text(100, 100, "Press a button to start!", LCD_ORANGE);

    // wait for button presses
    while(!(isHost | isClient));

    LCD_Clear(LCD_BLACK);

    // init semaphores
    G8RTOS_InitSemaphore(&wifiSemaphore, 1);
    G8RTOS_InitSemaphore(&lcdSemaphore, 1);
    G8RTOS_InitSemaphore(&ledSemaphore, 1);

    // connect with client

    // if connected, toggle
    BITBAND_PERI(P2->OUT, 2) ^= 1; // blue

    // init board (draw arena, players, scores)

    G8RTOS_KillSelf();
}

// B0 pressed
void PORT4_IRQHandler(void)
{
    int32_t IBit_State = StartCriticalSection();

    if(P4->IFG & BIT4)
    {
        isHost = true;
        P4->IFG &= ~BIT4;
    }

    EndCriticalSection(IBit_State);
}


// B1 pressed
void PORT5_IRQHandler(void)
{
    int32_t IBit_State = StartCriticalSection();

    if(P5->IFG & BIT4)
    {
        isClient = true;
        P5->IFG &= ~BIT4;
    }

    EndCriticalSection(IBit_State);
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient()
{

}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{

}

/*
 * Generate Ball thread
 */
void GenerateBall()
{

}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{

}

/*
 * Thread to move a single ball
 */
void MoveBall()
{

}

/*
 * End of game for the host
 */
void EndOfGameHost()
{

}

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
void DrawObjects()
{

}

/*
 * Thread to update LEDs based on score
 * Host is BLUE, Client is RED
 */
void MoveLEDs()
{
    G8RTOS_WaitSemaphore(&ledSemaphore);

    if(gameState.LEDScores[Host] == 0)
        LP3943_LedModeSet(BLUE, 0x0000);
    else if(gameState.LEDScores[Host] == 1)
        LP3943_LedModeSet(BLUE, 0x0010);
    else if(gameState.LEDScores[Host] == 2)
        LP3943_LedModeSet(BLUE, 0x0030);
    else if(gameState.LEDScores[Host] == 3)
        LP3943_LedModeSet(BLUE, 0x0070);
    else if(gameState.LEDScores[Host] == 4)
        LP3943_LedModeSet(BLUE, 0x00F0);
    else if(gameState.LEDScores[Host] == 5)
        LP3943_LedModeSet(BLUE, 0x00F1);
    else if(gameState.LEDScores[Host] == 6)
        LP3943_LedModeSet(BLUE, 0x00F3);
    else if(gameState.LEDScores[Host] == 7)
        LP3943_LedModeSet(BLUE, 0x00F7);
    else if(gameState.LEDScores[Host] >= 8)
        LP3943_LedModeSet(BLUE, 0x00FF);

    if(gameState.LEDScores[Client] == 0)
        LP3943_LedModeSet(RED, 0x0000);
    else if(gameState.LEDScores[Client] == 1)
        LP3943_LedModeSet(RED, 0x0100);
    else if(gameState.LEDScores[Client] == 2)
        LP3943_LedModeSet(RED, 0x0300);
    else if(gameState.LEDScores[Client] == 3)
        LP3943_LedModeSet(RED, 0x0700);
    else if(gameState.LEDScores[Client] == 4)
        LP3943_LedModeSet(RED, 0x0F00);
    else if(gameState.LEDScores[Client] == 5)
        LP3943_LedModeSet(RED, 0x1F00);
    else if(gameState.LEDScores[Client] == 6)
        LP3943_LedModeSet(RED, 0x3F00);
    else if(gameState.LEDScores[Client] == 7)
        LP3943_LedModeSet(RED, 0x7F00);
    else if(gameState.LEDScores[Client] >= 8)
        LP3943_LedModeSet(RED, 0xFF00);

    G8RTOS_SignalSemaphore(&ledSemaphore);
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
//playerType GetPlayerRole();

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player)
{

}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{

}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor)
{

}

/*
 * Initializes and prints initial game state
 */
void InitBoardState()
{

}

/*********************************************** Public Functions *********************************************************************/

