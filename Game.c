
/*********************************************** Includes ********************************************************************/
#include "Game.h"
/*********************************************** Includes ********************************************************************/
//Testing
/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game, only thread for Client before launching OS
 */
void JoinGame()
{
    //volatile GameState_t game_state;
    int32_t received_game_state;
    
    //initCC3100(TOP);
    P2->OUT &= ~BLUE_LED;
    gameState.gameDone = false;
    
    //while(1)
    //{
    // set up struct to be sent to Host

    //initCC3100(Client);

    gameState.player.IP_address = getLocalIP();//HOST_IP_ADDR;
    gameState.player.acknowledge = false; //false;
    gameState.player.displacement = 0;
    gameState.player.joined = false;
    gameState.player.playerNumber= 1; //Client;
    gameState.player.ready = true;


    //send player info to host
    SendData(&gameState.player, HOST_IP_ADDR, sizeof(gameState.player));
    //SendData(&clientInfo, HOST_IP_ADDR, sizeof(clientInfo));

    // wait for server response
    while(ReceiveData(&gameState.player.acknowledge, sizeof(gameState.player.acknowledge)) < 0);

    // if joined game, light LED to show connection
    gameState.player.joined = true;
    //clientInfo.joined = true;
    //clientInfo.acknowledge = true;

    SendData(&gameState.player, HOST_IP_ADDR, sizeof(gameState.player));
    //SendData(&clientInfo, HOST_IP_ADDR, sizeof(clientInfo));


    P2->OUT |= BLUE_LED;
    // init board state, semaphores, thread
    InitBoardState();

    /*G8RTOS_InitSemaphore(&wifiSemaphore, 1);
    G8RTOS_InitSemaphore(&lcdSemaphore, 1);
    G8RTOS_InitSemaphore(&ledSemaphore, 1);*/

    G8RTOS_AddThread(ReadJoystickClient, 10, "joystickC");
    G8RTOS_AddThread(SendDataToHost, 10, "dataToHost");
    G8RTOS_AddThread(ReceiveDataFromHost, 10, "recvDataHost");
    G8RTOS_AddThread(DrawObjects, 10, "drawObj");
    G8RTOS_AddThread(MoveLEDs, 14, "leds");
    G8RTOS_AddThread(IdleThread, 50, "idle");

    G8RTOS_KillSelf();
    //}
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
    //int32_t received_game_state2;
    GameState_t game_state2;
    bool game_done_var;
    
    while(1)
    {
        G8RTOS_WaitSemaphore(&wifiSemaphore);
        while(ReceiveData(&gameState, sizeof(gameState)) < 0)
        {
            G8RTOS_SignalSemaphore(&wifiSemaphore);
            sleep(1);
            G8RTOS_WaitSemaphore(&wifiSemaphore);
        }
        G8RTOS_SignalSemaphore(&wifiSemaphore);

      /*  //received_game_state2 = -1;
        while(ReceiveData(&gameState, sizeof(gameState)) < 0)   //(received_game_state2 < 0)
        {
            G8RTOS_WaitSemaphore(&wifiSemaphore);
            ReceiveData(&gameState, sizeof(gameState));
           // received_game_state2 = ReceiveData(&gameState, sizeof(gameState));
            G8RTOS_SignalSemaphore(&wifiSemaphore);
        }*/

        G8RTOS_WaitSemaphore(&playerSemaphore);
        game_done_var = gameState.gameDone;//game_is_done;
        G8RTOS_SignalSemaphore(&playerSemaphore);

        if(game_done_var)
        {
            G8RTOS_AddThread(EndOfGameClient, 10, "endOfGameClient");
        }

        sleep(5);
    }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    SpecificPlayerInfo_t game_state3;

    while(1)
    {
       /* G8RTOS_WaitSemaphore(&playerSemaphore);
        game_state3 = gameState.player;   //clientInfo; //gameState;//clientInfo;
        G8RTOS_SignalSemaphore(&playerSemaphore);
        //G8RTOS_WaitSemaphore(&lcdSemaphore);*/

        G8RTOS_WaitSemaphore(&wifiSemaphore);

        //SendData(&game_state3, HOST_IP_ADDR, sizeof(game_state3));

        SendData(&gameState.player, HOST_IP_ADDR, sizeof(gameState.player));

        //clientInfo.displacement = 0;                                    //this
        G8RTOS_SignalSemaphore(&wifiSemaphore);
        //clientInfo.displacement = 0;
        //G8RTOS_SignalSemaphore(&lcdSemaphore);

        sleep(2);
    }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{

    int16_t x_coord2 = 0;
    int16_t y_coord2 = 0;
    while(1)
    {
        GetJoystickCoordinates(&x_coord2, &y_coord2);

        // change displacement
        G8RTOS_WaitSemaphore(&playerSemaphore);
        gameState.player.displacement = (x_coord2 >> JOYSTICK_DIV);
        //clientInfo.displacement = (x_coord2 >> JOYSTICK_DIV);
        G8RTOS_SignalSemaphore(&playerSemaphore);

        /*G8RTOS_WaitSemaphore(&wifiSemaphore);
        SendData(&gameState.player.displacement, HOST_IP_ADDR, sizeof(gameState.player.displacement));
        //clientInfo.displacement = (x_coord2 >> JOYSTICK_DIV);
        G8RTOS_SignalSemaphore(&wifiSemaphore);*/

        sleep(2);

    }

}
/*
 * End of game for the client
 */
void EndOfGameClient()
{
    int32_t received_game_state_cltEnd;
    // wait for all semaphores
    G8RTOS_WaitSemaphore(&playerSemaphore);
    G8RTOS_WaitSemaphore(&wifiSemaphore);
    //G8RTOS_WaitSemaphore(&lcdSemaphore);
    //G8RTOS_KillOtherThreads();
    G8RTOS_KillOthers();
    G8RTOS_InitSemaphore(&playerSemaphore, 1);
    G8RTOS_InitSemaphore(&wifiSemaphore, 1);
    //G8RTOS_InitSemaphore(&ledSemaphore, 1);

    LP3943_LedModeSet(BLUE, gameState.LEDScores[TOP]);
    LP3943_LedModeSet(RED, gameState.LEDScores[BOTTOM]);

    if(gameState.winner == TOP)
    {
        LCD_Clear(LCD_RED);
        //LCD_DrawRectangle(MIN_SCREEN_X, MAX_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_Y, LCD_RED);
        LCD_Text(50, 50, "Red player is the winner!", LCD_WHITE);

        client_overallScore[0] += 1;
        LP3943_LedModeSet(RED, 0xFFFF);

        if(client_overallScore[0] >= 8)
        {
            client_overallScore[0] = 0;
            client_overallScore[1] += 1;
        }

    }

    else if(gameState.winner == BOTTOM)
    {
        LCD_Clear(LCD_BLUE);
        //LCD_DrawRectangle(MIN_SCREEN_X, MAX_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_Y, LCD_BLUE);
        LCD_Text(50, 50, "Blue player is the winner!", LCD_WHITE);

        gameState.overallScores[0] += 1;
        LP3943_LedModeSet(BLUE, 0xFFFF);

        if(gameState.overallScores[0] >= 8)
        {
            gameState.overallScores[0] = 0;
            gameState.overallScores[1] += 1;
        }
    }

    //LCD_Clear(LCD_BLACK);
    LCD_Text(10, 100, "Please wait for host to restart game.", LCD_WHITE);

    //while(1)
    //{
        while(ReceiveData(&gameState.player.ready, sizeof(gameState.player.ready)) < 0);

        InitBoardState();

        char score[4];
        LCD_Text(MIN_SCREEN_X+4, MIN_SCREEN_Y, "Red:", LCD_RED);
        sprintf(score, "%d", client_overallScore[1]);
        sprintf(score, "%d", client_overallScore[0]);
        LCD_Text(0, ARENA_MIN_Y + 15, score, gameState.players[TOP].color);
        LCD_Text(MIN_SCREEN_X, (MAX_SCREEN_Y-28), "Blue:", LCD_BLUE);
        sprintf(score, "%d", gameState.overallScores[1]);
        sprintf(score, "%d", gameState.overallScores[0]);
        LCD_Text(0, ARENA_MAX_Y - 15, score, gameState.players[BOTTOM].color);


        G8RTOS_AddThread(ReadJoystickClient, 10, "joystickC");
        G8RTOS_AddThread(SendDataToHost, 10, "dataToHost");
        G8RTOS_AddThread(ReceiveDataFromHost, 10, "recvDataHost");
        G8RTOS_AddThread(DrawObjects, 10, "drawObj");
        G8RTOS_AddThread(MoveLEDs, 14, "leds");
        G8RTOS_AddThread(IdleThread, 50, "idle");

        G8RTOS_KillSelf();
    


}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/

void InitPins()
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

    NVIC_EnableIRQ(PORT5_IRQn); // enable PORT5 interrupt using enable bits controlled by registers in NVIC

    // for toggling LED2
    P2->SEL0 &= ~BIT2;  // blue
    P2->SEL1 &= ~BIT2;
    P2->DIR |= BIT2;
    P2->OUT &= ~BIT2;

    P2->SEL0 &= ~BIT0;  // red
    P2->SEL1 &= ~BIT0;
    P2->DIR |= BIT0;
    P2->OUT &= ~BIT0;
}

void InitGameVariablesHost()
{
    // Host variables
   /* gameState.player.IP_address = HOST_IP_ADDR;
    gameState.player.acknowledge = true; //false;
    gameState.player.displacement = 0;
    gameState.player.joined = false;
    gameState.player.ready = false;
    gameState.player.playerNumber= 0;*/

    // general player info
   /* gameState.players[BOTTOM].color = PLAYER_BLUE;
    gameState.players[BOTTOM].currentCenter = PADDLE_X_CENTER;
    gameState.players[BOTTOM].position = 0;  //changed to bottom // Host; //TOP
    gameState.players[TOP].color = PLAYER_RED;
    gameState.players[TOP].currentCenter = PADDLE_X_CENTER;
    gameState.players[TOP].position = 1; //change to top  // Client;  //BOTTOM*/

    // balls
    int i;
    for(i = 0; i < MAX_NUM_OF_BALLS; i++)
        gameState.balls[i].alive = false;

    // game variables
    //gameState.numberOfBalls = 0;
    //gameState.winner = false;
    //gameState.gameDone = false;
    //gameState.LEDScores[Host] = 0;
    //gameState.LEDScores[Client] = 0;
    //gameState.overallScores[Host] = 0;
    //gameState.overallScores[Client] = 0;
}

/*
 * Thread for the host to create a game, only thread for Host before launching OS
 */
void CreateGame()
{
    GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN4);
    GPIO_disableInterrupt(GPIO_PORT_P5, GPIO_PIN4);


    //InitGameVariablesHost();
    //initCC3100(BOTTOM);

    // wait for client to join, try receive packet from them
    while(ReceiveData(&gameState.player, sizeof(gameState.player)) < 0);
    //while(ReceiveData(&clientInfo, sizeof(clientInfo)) < 0);

    InitBoardState();

    InitGameVariablesHost();

    // send ack to client, wait for client to ack back
    gameState.player.acknowledge = true;

    SendData(&gameState.player.acknowledge, gameState.player.IP_address, sizeof(gameState.player.acknowledge));
    //SendData(&gameState.player.acknowledge, clientInfo.IP_address, sizeof(gameState.player.acknowledge));


    // if connected, toggle blue LED2
    BITBAND_PERI(P2->OUT, 2) ^= 1; // blue

    // init semaphores
    G8RTOS_InitSemaphore(&wifiSemaphore, 1);
    G8RTOS_InitSemaphore(&lcdSemaphore, 1);
    G8RTOS_InitSemaphore(&ledSemaphore, 1);
    G8RTOS_InitSemaphore(&playerSemaphore, 1);

    // init board (draw arena, players, scores)
    //InitBoardState();

    // add threads
    G8RTOS_AddThread(GenerateBall, 10, "gen_ball");
    G8RTOS_AddThread(DrawObjects, 10, "drawObj");
    G8RTOS_AddThread(ReadJoystickHost, 10, "joystickH");
    G8RTOS_AddThread(SendDataToClient, 10, "sendDataClient");
    G8RTOS_AddThread(ReceiveDataFromClient, 10, "recvDataClient");
    G8RTOS_AddThread(MoveLEDs, 14, "leds");
    G8RTOS_AddThread(IdleThread, 50, "idle");

    G8RTOS_KillSelf();

}

// B0 pressed
void PORT4_IRQHandler(void)
{
    int32_t IBit_State = StartCriticalSection();

    if(P4->IFG & BIT4)
    {
        isHost = true;
        gameState.player.ready = true;    //this one
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
    GameState_t game_Statex;
    while(1)
    {
        //fill packet for client
       G8RTOS_WaitSemaphore(&playerSemaphore);
       game_Statex = gameState;
       G8RTOS_SignalSemaphore(&playerSemaphore);

       //send packet
       G8RTOS_WaitSemaphore(&wifiSemaphore);
       SendData(&game_Statex, game_Statex.player.IP_address, sizeof(game_Statex));
       //SendData(&gameState, gameState.player.IP_address, sizeof(gameState));
       G8RTOS_SignalSemaphore(&wifiSemaphore);

       //sleep(5);

       //check if game is done
       if(game_Statex.gameDone)//(gameState.gameDone)
       {
           /*G8RTOS_WaitSemaphore(&wifiSemaphore);
           SendData(&game_Statex, game_Statex.player.IP_address, sizeof(game_Statex));
           //SendData(&gameState, clientInfo.IP_address, sizeof(gameState));
           G8RTOS_SignalSemaphore(&wifiSemaphore);*/
           G8RTOS_AddThread(EndOfGameHost, 10, "endHost");
       }

       sleep(7);

    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    SpecificPlayerInfo_t client_info;
    while(1)
    {
        G8RTOS_WaitSemaphore(&wifiSemaphore);
        //while(ReceiveData(&gameState.player, sizeof(gameState.player)) < 0)
        while(ReceiveData(&client_info, sizeof(client_info)) < 0)
        {
            G8RTOS_SignalSemaphore(&wifiSemaphore);
            sleep(1);
            G8RTOS_WaitSemaphore(&wifiSemaphore);
        }

        G8RTOS_SignalSemaphore(&wifiSemaphore);

        G8RTOS_WaitSemaphore(&playerSemaphore);
        //gameState.players[TOP].currentCenter -= gameState.player.displacement;
        gameState.players[TOP].currentCenter -= client_info.displacement;                                   //woooo
        if(gameState.players[TOP].currentCenter > HORIZ_CENTER_MAX_PL)
        {
            gameState.players[TOP].currentCenter = HORIZ_CENTER_MAX_PL;
        }
        else if(gameState.players[TOP].currentCenter < HORIZ_CENTER_MIN_PL)
        {
            gameState.players[TOP].currentCenter = HORIZ_CENTER_MIN_PL;
        }

        G8RTOS_SignalSemaphore(&playerSemaphore);

        sleep(2);
    }
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
    uint8_t ballCount;
    uint16_t sleepDuration = 5000;
    char ballName[3];
    while(1)
    {
        G8RTOS_WaitSemaphore(&playerSemaphore);

        if(gameState.numberOfBalls < MAX_NUM_OF_BALLS)
        {
            //sprintf(ballName, "%d", gameState.numberOfBalls); // int to string, stores in char array
            G8RTOS_AddThread(MoveBall, 14, "ball");
            //G8RTOS_WaitSemaphore(&playerSemaphore);
            gameState.numberOfBalls++;
            //G8RTOS_SignalSemaphore(&playerSemaphore);
        }

        //    if(sleepDuration < 0)
        //        sleepDuration = 5000;
        //    else
        //        sleepDuration -= 200;

        ballCount = gameState.numberOfBalls;
        G8RTOS_SignalSemaphore(&playerSemaphore);

        sleep(ballCount * 100);
    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    int16_t temp;
    int16_t x_coord = 0;
    int16_t y_coord = 0;
    while(1)
    {
        //int16_t temp = 0;
        GetJoystickCoordinates(&x_coord, &y_coord);

        // change displacement
     /*   G8RTOS_WaitSemaphore(&playerSemaphore);
        //clientInfo.displacement = (x_coord >> JOYSTICK_DIV);
        gameState.player.displacement = (x_coord >> JOYSTICK_DIV);
        G8RTOS_SignalSemaphore(&playerSemaphore);*/

        sleep(10);
        // then add to bottom player
        G8RTOS_WaitSemaphore(&playerSemaphore);
        gameState.players[BOTTOM].currentCenter -= (x_coord >> JOYSTICK_DIV);

        // check bounds, change center accordingly
        if(gameState.players[BOTTOM].currentCenter < HORIZ_CENTER_MIN_PL)
            gameState.players[BOTTOM].currentCenter = HORIZ_CENTER_MIN_PL;
        else if(gameState.players[BOTTOM].currentCenter > HORIZ_CENTER_MAX_PL)
            gameState.players[BOTTOM].currentCenter = HORIZ_CENTER_MAX_PL;
        G8RTOS_SignalSemaphore(&playerSemaphore);

        //sleep(10);
    }
}


bool CollisionDetect(playerPosition position, int ballIndex)
{
    //tried implementing algo
    int32_t w = WIDTH_TOP_OR_BOTTOM;
    int32_t h = HEIGHT_TOP_OR_BOTTOM;
    int32_t dx;
    int32_t dy;

    if(position == TOP)
    {
        dx = gameState.balls[ballIndex].currentCenterX - gameState.players[TOP].currentCenter;
        dy = gameState.balls[ballIndex].currentCenterY - TOP_PADDLE_EDGE;
    }
    else
    {
        dx = gameState.balls[ballIndex].currentCenterX - gameState.players[BOTTOM].currentCenter;
        dy = gameState.balls[ballIndex].currentCenterY - BOTTOM_PADDLE_EDGE;
    }

    if(abs(dx) <= w && abs(dy) <= h)
    {
        int32_t wy = w*dy;
        int32_t hx = h*dx;
        if(wy>hx)
        {
            if(wy>-hx)
            {
                // collision at the red paddle
                int a = 1;
                return true;
            }
            else
            {
                // on the left
                int a = 1;
            }
        }
        else
        {
            if(wy>-hx)
            {
                // on the right
                int a = 1;
            }
            else
            {
                // should be collision at the blue paddle
                int a = 1;
                return true;
            }
        }
    }

    return false;
}




/*
 * Thread to move a single ball
 */
void MoveBall()
{
    Ball_t *balls;
    // index for ball struct array
    int index;
    // velocities
    uint8_t ballCount1;

    int8_t xVelocity;
    int8_t yVelocity;
    uint8_t random_velocity = rand() % 2; //MAX_BALL_SPEED + 1;
    //int8_t yVelocity = rand() % MAX_BALL_SPEED + 1;

    // check for next available spot in array
    G8RTOS_WaitSemaphore(&playerSemaphore);

    int i;
    for(i = 0; i < MAX_NUM_OF_BALLS; i++)
    {
        if(!gameState.balls[i].alive)
        {
            index = i;
            break;
        }
    }

    gameState.balls[index].currentCenterX = (rand() % (HORIZ_CENTER_MAX_BALL-HORIZ_CENTER_MIN_BALL-5)) + HORIZ_CENTER_MIN_BALL + BALL_SIZE+5;
    gameState.balls[index].currentCenterY = (rand() % (VERT_CENTER_MAX_BALL-20)) + 20;

    if(random_velocity == 0)
    {
        xVelocity = 3;
    }
    else
    {
        xVelocity = -3;
    }

    if((rand() % 2) == 0)
    {
        yVelocity = 3;
    }
    else
    {
        yVelocity = -3;
    }

    gameState.balls[index].color = LCD_WHITE;
    gameState.balls[index].alive = true;

    G8RTOS_SignalSemaphore(&playerSemaphore);

    while(1)
    {
        G8RTOS_WaitSemaphore(&playerSemaphore);

        gameState.balls[index].currentCenterX += xVelocity;
        gameState.balls[index].currentCenterY += yVelocity;

        //client paddle
        if (   ((gameState.balls[index].currentCenterY - BALL_SIZE_D2) <= TOP_PADDLE_EDGE) &&
            (   ( ((gameState.balls[index].currentCenterX + BALL_SIZE_D2) > (gameState.players[TOP].currentCenter - PADDLE_LEN_D2))   &&
                 ((gameState.balls[index].currentCenterX - BALL_SIZE_D2) < (gameState.players[TOP].currentCenter + PADDLE_LEN_D2)) ) ||
             ( ((gameState.balls[index].currentCenterX + BALL_SIZE_D2) > (gameState.players[TOP].currentCenter - PADDLE_LEN_D2))   &&
              ((gameState.balls[index].currentCenterX - BALL_SIZE_D2) < (gameState.players[TOP].currentCenter + PADDLE_LEN_D2)) )    ) )
        {


            if(abs(yVelocity) < MAX_BALL_SPEED)
            {
                yVelocity--;
            }

            yVelocity *= -1;

            if (gameState.balls[index].currentCenterX - xVelocity > (gameState.players[TOP].currentCenter + _1_3_PADDLE))
            {
                xVelocity = abs(yVelocity);
            }
            else if (gameState.balls[index].currentCenterX - xVelocity < (gameState.players[TOP].currentCenter - _1_3_PADDLE))
            {
                xVelocity = (-1)*yVelocity;
            }
            else
            {
                xVelocity = 0;
            }

            int16_t displacement = TOP_PADDLE_EDGE - (gameState.balls[index].currentCenterY - BALL_SIZE_D2  - 1); //hurrr

            gameState.balls[index].currentCenterY = gameState.balls[index].currentCenterY + displacement;

            gameState.balls[index].color = gameState.players[TOP].color;

            if (xVelocity < 0)
            {
                gameState.balls[index].currentCenterX = gameState.balls[index].currentCenterX + displacement;
            }
            else if (xVelocity > 0)
            {
                gameState.balls[index].currentCenterX = gameState.balls[index].currentCenterX - displacement;
            }

        }

        else if (   ((gameState.balls[index].currentCenterY + BALL_SIZE_D2) >= BOTTOM_PADDLE_EDGE) &&
                 (   ( ((gameState.balls[index].currentCenterX + BALL_SIZE_D2) > (gameState.players[BOTTOM].currentCenter - PADDLE_LEN_D2))   &&
                      ((gameState.balls[index].currentCenterX - BALL_SIZE_D2) < (gameState.players[BOTTOM].currentCenter + PADDLE_LEN_D2)) ) ||
                  ( ((gameState.balls[index].currentCenterX  + BALL_SIZE_D2) > (gameState.players[BOTTOM].currentCenter - PADDLE_LEN_D2))   &&
                   ((gameState.balls[index].currentCenterX  - BALL_SIZE_D2) < (gameState.players[BOTTOM].currentCenter + PADDLE_LEN_D2)) )    ) )
        {

            if(abs(yVelocity) < MAX_BALL_SPEED)
            {
                yVelocity++;
            }

            yVelocity *= -1;

            if (gameState.balls[index].currentCenterX - xVelocity> (gameState.players[BOTTOM].currentCenter + _1_3_PADDLE))
            {
                xVelocity = (-1)*yVelocity;
            }
            else if (gameState.balls[index].currentCenterX - xVelocity < (gameState.players[BOTTOM].currentCenter  - _1_3_PADDLE))
            {
                xVelocity = yVelocity;
            }
            else
            {
                xVelocity = 0;
            }

            int16_t displacement = (gameState.balls[index].currentCenterY + BALL_SIZE_D2) - BOTTOM_PADDLE_EDGE;

            gameState.balls[index].currentCenterY = gameState.balls[index].currentCenterY - displacement - 1;    ///hurrr

            gameState.balls[index].color = gameState.players[BOTTOM].color;

            if (xVelocity < 0)
            {
                gameState.balls[index].currentCenterX =  gameState.balls[index].currentCenterX + displacement + 1;  ///hurrr
            }
            else if (xVelocity > 0)
            {
                gameState.balls[index].currentCenterX = gameState.balls[index].currentCenterX - displacement - 1;   ///hurrr
            }

        }

        else if (gameState.balls[index].currentCenterX <= HORIZ_CENTER_MIN_BALL)
        {
            xVelocity *= -1;

            int16_t displacement = HORIZ_CENTER_MIN_BALL - gameState.balls[index].currentCenterX;

            gameState.balls[index].currentCenterX = gameState.balls[index].currentCenterX + displacement + 1;   ///hurrr

            if (yVelocity < 0)
            {
                gameState.balls[index].currentCenterY = gameState.balls[index].currentCenterY + displacement + 1; ///hurrr
            }
            else if (yVelocity > 0)
            {
                gameState.balls[index].currentCenterY = gameState.balls[index].currentCenterY - displacement - 1; ///hurrr
            }
        }

        else if (gameState.balls[index].currentCenterX >= HORIZ_CENTER_MAX_BALL)
        {
            xVelocity *= -1;

            int16_t displacement = gameState.balls[index].currentCenterX - HORIZ_CENTER_MAX_BALL;

            gameState.balls[index].currentCenterX = gameState.balls[index].currentCenterX - displacement - 1; ///hurrr

            if (yVelocity < 0)
            {
                gameState.balls[index].currentCenterY = gameState.balls[index].currentCenterY + displacement + 1; ///hurrr
            }
            else if (yVelocity > 0)
            {
                gameState.balls[index].currentCenterY = gameState.balls[index].currentCenterY - displacement - 1; ///hurrr
            }
        }

        if ( ((gameState.balls[index].currentCenterY + BALL_SIZE_D2) < ARENA_MIN_Y)  || ((gameState.balls[index].currentCenterY - BALL_SIZE_D2) > ARENA_MAX_Y) )
        {
            gameState.balls[index].alive = false;

            gameState.numberOfBalls--;

            if((gameState.balls[index].currentCenterY + BALL_SIZE_D2) < ARENA_MIN_Y && (gameState.balls[index].color == PLAYER_BLUE ))
            {
                if (gameState.LEDScores[BOTTOM] == 0)
                {
                    gameState.LEDScores[BOTTOM] = 1;
                }
                else
                {
                    gameState.LEDScores[BOTTOM] += 1;//
                }

                if(gameState.LEDScores[BOTTOM] >= 15)
                {
                    gameState.winner = BOTTOM;
                    gameState.gameDone = true;
                    //G8RTOS_AddThread(EndOfGameClient, 1, "endOfGameClient");
                    //G8RTOS_AddThread(EndOfGameHost, 1, "endHost");

                }
            }

            else if((gameState.balls[index].currentCenterY - BALL_SIZE_D2) > ARENA_MAX_Y && gameState.balls[index].color == PLAYER_RED)
            {
                if (gameState.LEDScores[TOP] == 0)
                {
                    gameState.LEDScores[TOP] = 1;

                }
                else
                {
                    gameState.LEDScores[TOP] += 1;
                }
                if (gameState.LEDScores[TOP] >= 15)
                {

                    gameState.winner = TOP;
                    gameState.gameDone = true;
                }
            }

            G8RTOS_SignalSemaphore(&playerSemaphore);

            // kill ball thread
            G8RTOS_KillSelf();
        }

        G8RTOS_SignalSemaphore(&playerSemaphore);

        sleep(35);

    }
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
    // wait for all semaphores
   //    G8RTOS_WaitSemaphore(&ledSemaphore);
       G8RTOS_WaitSemaphore(&wifiSemaphore);
       G8RTOS_WaitSemaphore(&playerSemaphore);

       // kill all threads
       G8RTOS_KillOthers();

       // re-init semaphores
   //    G8RTOS_InitSemaphore(&ledSemaphore, 1);
       G8RTOS_InitSemaphore(&wifiSemaphore, 1);
       G8RTOS_InitSemaphore(&playerSemaphore, 1);

       if(gameState.winner == TOP)
       {
           LCD_Clear(LCD_RED);
           //LCD_DrawRectangle(MIN_SCREEN_X, MAX_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_Y, LCD_RED);
           LCD_Text(50, 50, "Red player is the winner!", LCD_WHITE);

           client_overallScore[0] += 1;
           LP3943_LedModeSet(RED, 0xFFFF);

           if(client_overallScore[0] > 8)
           {
               client_overallScore[0] = 0;
               client_overallScore[1] += 1;
           }

       }
       else if (gameState.winner == BOTTOM)
       {
           LCD_Clear(LCD_BLUE);
           //LCD_DrawRectangle(MIN_SCREEN_X, MAX_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_Y, LCD_BLUE);
           LCD_Text(50, 50, "Blue player is the winner!", LCD_WHITE);

           gameState.overallScores[0] += 1;
           LP3943_LedModeSet(BLUE, 0xFFFF);

           if(gameState.overallScores[0] > 8)
           {
               gameState.overallScores[0] = 0;
               gameState.overallScores[1] += 1;
           }
       }

       isHost = false;

       InitGameVariablesHost();

       // wait for Host action to restart game
       //LCD_Clear(LCD_BLACK);
       LCD_Text(60, 100, "Wait to restart game", LCD_WHITE); //?

       // enable interrupts for button presses again
       //GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN4);
   //    GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN4);
       //P4->IFG &= ~BIT4;
       P4->DIR &= ~BIT4;       // set as input
       P4->IFG &= ~BIT4;       // clear IFG flag on P4.4
       P4->IE |= BIT4;         // enable interrupt on P4.4
       P4->IES |= BIT4;        // high-to-low transition, falling edge interrupt
       P4->REN |= BIT4;        // pull-up resistor
       P4->OUT |= BIT4;        // sets res to pull-up

       GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN4);
       NVIC_EnableIRQ(PORT4_IRQn);


       while(!(isHost));

       gameState.player.ready = true;
       SendData(&gameState.player.ready, gameState.player.IP_address, sizeof(gameState.player.ready));

       InitBoardState();

       char score[4];
       LCD_Text(MIN_SCREEN_X+4, MIN_SCREEN_Y, "Red:", LCD_RED);
       sprintf(score, "%d", client_overallScore[1]);
       sprintf(score, "%d", client_overallScore[0]);
       LCD_Text(0, ARENA_MIN_Y + 15, score, gameState.players[TOP].color);
       LCD_Text(MIN_SCREEN_X, (MAX_SCREEN_Y-28), "Blue:", LCD_BLUE);
       sprintf(score, "%d", gameState.overallScores[1]);
       sprintf(score, "%d", gameState.overallScores[0]);
       LCD_Text(0, ARENA_MAX_Y - 15, score, gameState.players[BOTTOM].color);



       // add threads
       G8RTOS_AddThread(GenerateBall, 10, "gen_ball");
       G8RTOS_AddThread(DrawObjects, 10, "drawObj");
       G8RTOS_AddThread(ReadJoystickHost, 10, "joystickH");
       G8RTOS_AddThread(SendDataToClient, 10, "sendDataClient");
       G8RTOS_AddThread(ReceiveDataFromClient, 10, "recvDataClient");
       G8RTOS_AddThread(MoveLEDs, 14, "leds");
       G8RTOS_AddThread(IdleThread, 50, "idle");

       G8RTOS_KillSelf();
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
    static bool delete_ball[MAX_NUM_OF_BALLS];
    static bool create_object[MAX_NUM_OF_BALLS];
    static PrevBall_t previous_balls[MAX_NUM_OF_BALLS];
    static PrevPlayer_t previous_players[MAX_NUM_OF_PLAYERS];

    previous_players[TOP].Center = PADDLE_X_CENTER;
    previous_players[BOTTOM].Center = PADDLE_X_CENTER;

    for (int i = 0; i< MAX_NUM_OF_BALLS; i++)
    {
        create_object[i] = true;
        delete_ball[i] = false;
    }

    while (1)
    {
        G8RTOS_WaitSemaphore(&playerSemaphore);

        for (uint8_t i = 0; i < MAX_NUM_OF_BALLS; i++)
        {
            if (gameState.balls[i].alive)
            {
                if(create_object[i])
                {
                    LCD_DrawRectangle((gameState.balls[i].currentCenterX)- BALL_SIZE_D2, (gameState.balls[i].currentCenterX)+BALL_SIZE_D2,
                                          (gameState.balls[i].currentCenterY)-BALL_SIZE_D2, (gameState.balls[i].currentCenterY)+BALL_SIZE_D2,
                                          (gameState.balls[i].color));
                    create_object[i] = false;
                }
                else
                {
                    LCD_DrawRectangle((previous_balls[i].CenterX)-BALL_SIZE_D2,
                                         (previous_balls[i].CenterX)+BALL_SIZE_D2,
                                         (previous_balls[i].CenterY)-BALL_SIZE_D2,
                                         (previous_balls[i].CenterY)+BALL_SIZE_D2,
                                         LCD_BLACK);

                    LCD_DrawRectangle((gameState.balls[i].currentCenterX)- BALL_SIZE_D2, (gameState.balls[i].currentCenterX)+BALL_SIZE_D2,
                                          (gameState.balls[i].currentCenterY)-BALL_SIZE_D2, (gameState.balls[i].currentCenterY)+BALL_SIZE_D2,
                                          (gameState.balls[i].color));
                }

                previous_balls[i].CenterX = gameState.balls[i].currentCenterX;
                previous_balls[i].CenterY = gameState.balls[i].currentCenterY;
            }
            else
            {
                if (delete_ball[i])
                {
                    LCD_DrawRectangle((previous_balls[i].CenterX)-BALL_SIZE_D2,
                                         (previous_balls[i].CenterX)+BALL_SIZE_D2,
                                         (previous_balls[i].CenterY)-BALL_SIZE_D2,
                                         (previous_balls[i].CenterY)+BALL_SIZE_D2,
                                         LCD_BLACK);
                    delete_ball[i] = false;

                    gameState.balls[i].currentCenterX = -1;
                    gameState.balls[i].currentCenterY = -1;

                    previous_balls[i].CenterX = gameState.balls[i].currentCenterX;
                    previous_balls[i].CenterY = gameState.balls[i].currentCenterY;
                }
                else
                {
                    delete_ball[i] = true;
                }
            }
        }

        for (uint16_t i = 0; i < MAX_NUM_OF_PLAYERS; i++)
        {
            UpdatePlayerOnScreen(&previous_players[i], &gameState.players[i]);
        }

        G8RTOS_SignalSemaphore(&playerSemaphore);

        sleep(10);
    }

}

/*
 * Thread to update LEDs based on score
 * Host is BLUE, Client is RED
 */
void MoveLEDs()
{

    while(1)
    {
        //G8RTOS_WaitSemaphore(&ledSemaphore);

        if(gameState.LEDScores[BOTTOM] == 0)
            //LP3943_LedModeSet(BLUE, 0x0010);
            LP3943_LedModeSet(BLUE, 0x8000);
        else if(gameState.LEDScores[BOTTOM] == 1)
            //LP3943_LedModeSet(BLUE, 0x0030);
            LP3943_LedModeSet(BLUE, 0xC000);
        else if(gameState.LEDScores[BOTTOM] == 2)
            //LP3943_LedModeSet(BLUE, 0x0070);
            LP3943_LedModeSet(BLUE, 0xE000);
        else if(gameState.LEDScores[BOTTOM] == 3)
            LP3943_LedModeSet(BLUE, 0xF000);
        else if(gameState.LEDScores[BOTTOM] == 4)
            //LP3943_LedModeSet(BLUE, 0x00F1);
            LP3943_LedModeSet(BLUE, 0xF800);
        else if(gameState.LEDScores[BOTTOM] == 5)
            //LP3943_LedModeSet(BLUE, 0x00F3);
            LP3943_LedModeSet(BLUE, 0xFC00);
        else if(gameState.LEDScores[BOTTOM] == 6)
            LP3943_LedModeSet(BLUE, 0xFE00);
            //LP3943_LedModeSet(BLUE, BOTTOM);
        else if(gameState.LEDScores[BOTTOM] == 7)
            LP3943_LedModeSet(BLUE, 0xFF00);
        else if(gameState.LEDScores[BOTTOM] == 8)
           LP3943_LedModeSet(BLUE, 0xFF80);
        else if(gameState.LEDScores[BOTTOM] == 9)
           //LP3943_LedModeSet(BLUE, 0x0010);
           LP3943_LedModeSet(BLUE, 0xFFC0);
        else if(gameState.LEDScores[BOTTOM] == 10)
           //LP3943_LedModeSet(BLUE, 0x0030);
           LP3943_LedModeSet(BLUE, 0xFFE0);
        else if(gameState.LEDScores[BOTTOM] == 11)
           //LP3943_LedModeSet(BLUE, 0x0070);
           LP3943_LedModeSet(BLUE, 0xFFF0);
        else if(gameState.LEDScores[BOTTOM] == 12)
           LP3943_LedModeSet(BLUE, 0xFFF8);
        else if(gameState.LEDScores[BOTTOM] == 13)
           //LP3943_LedModeSet(BLUE, 0x00F1);
           LP3943_LedModeSet(BLUE, 0xFFFC);
        else if(gameState.LEDScores[BOTTOM] == 14)
           //LP3943_LedModeSet(BLUE, 0x00F3);
           LP3943_LedModeSet(BLUE, 0xFFFE);
        else if(gameState.LEDScores[BOTTOM] == 15)
           LP3943_LedModeSet(BLUE, 0xFFFF);


        if(gameState.LEDScores[TOP] == 0)
            LP3943_LedModeSet(RED, 0x0001);
        else if(gameState.LEDScores[TOP] == 1)
            LP3943_LedModeSet(RED, 0x0003);
        else if(gameState.LEDScores[TOP] == 2)
            LP3943_LedModeSet(RED, 0x0007);
        else if(gameState.LEDScores[TOP] == 3)
            LP3943_LedModeSet(RED, 0x000F);
        else if(gameState.LEDScores[TOP] == 4)
            LP3943_LedModeSet(RED, 0x001F);
        else if(gameState.LEDScores[TOP] == 5)
            LP3943_LedModeSet(RED, 0x003F);
        else if(gameState.LEDScores[TOP] == 6)
            LP3943_LedModeSet(RED, 0x007F);
        else if(gameState.LEDScores[TOP] == 7)
            LP3943_LedModeSet(RED, 0x00FF);
        else if(gameState.LEDScores[TOP] == 8)
            LP3943_LedModeSet(RED, 0x01FF);
        else if(gameState.LEDScores[TOP] == 9)
            LP3943_LedModeSet(RED, 0x03FF);
        else if(gameState.LEDScores[TOP] == 10)
            LP3943_LedModeSet(RED, 0x07FF);
        else if(gameState.LEDScores[TOP] == 11)
            LP3943_LedModeSet(RED, 0x0FFF);
        else if(gameState.LEDScores[TOP] == 12)
            LP3943_LedModeSet(RED, 0x1FFF);
        else if(gameState.LEDScores[TOP] == 13)
            LP3943_LedModeSet(RED, 0x3FFF);
        else if(gameState.LEDScores[TOP] == 14)
            LP3943_LedModeSet(RED, 0x7FFF);
        else if(gameState.LEDScores[TOP] == 15)
            LP3943_LedModeSet(RED, 0xFFFF);

        //return;
        //G8RTOS_SignalSemaphore(&ledSemaphore);
        sleep(500); // idk if needed?
    }
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole()
{
    // init players
    isHost = false;
    isClient = false;

    LCD_Clear(LCD_BLACK);

    LCD_Text(60, 100, "PRESS A BUTTON TO START!", LCD_RED);

    InitPins();

    // wait for button presses
    while(!(isHost | isClient));

    LCD_Clear(LCD_BLACK);

    if(isHost)
        return Host;
        //return Client;
    else if(isClient)
        return Client;
        //return Host;
}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player)
{
    if(player->position == BOTTOM)
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2, BOTTOM_PLAYER_CENTER_Y - PADDLE_WID_D2, BOTTOM_PLAYER_CENTER_Y + PADDLE_WID_D2, player->color);
    else if(player->position == TOP)
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2, TOP_PLAYER_CENTER_Y - PADDLE_WID_D2, TOP_PLAYER_CENTER_Y + PADDLE_WID_D2, player->color);
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{
    int16_t front_padd = prevPlayerIn->Center - PADDLE_LEN_D2 + 1;
    int16_t back_padd = prevPlayerIn->Center + PADDLE_LEN_D2 - 1;
    int16_t change_dis = outPlayer->currentCenter - prevPlayerIn->Center;

    //right
    if(change_dis > 0)
    {
        if(outPlayer->position == BOTTOM)
        {
            LCD_DrawRectangle(back_padd - 4*WIGGLE_ROOM, back_padd + change_dis, ARENA_MAX_Y-PADDLE_WID, ARENA_MAX_Y, outPlayer->color);
            LCD_DrawRectangle(front_padd, front_padd + change_dis, ARENA_MAX_Y-PADDLE_WID, ARENA_MAX_Y, LCD_BLACK);
        }
        else
        {
            LCD_DrawRectangle(back_padd - 4*WIGGLE_ROOM, back_padd + change_dis, ARENA_MIN_Y, ARENA_MIN_Y+PADDLE_WID, outPlayer->color);
            LCD_DrawRectangle(front_padd, front_padd + change_dis, ARENA_MIN_Y, ARENA_MIN_Y+PADDLE_WID, LCD_BLACK);
        }
    }
    //left
    else if(change_dis < 0)
    {
        change_dis *= -1;
        if(outPlayer->position == BOTTOM)
        {
            LCD_DrawRectangle(back_padd - change_dis, back_padd, ARENA_MAX_Y-PADDLE_WID, ARENA_MAX_Y, LCD_BLACK);
            LCD_DrawRectangle(front_padd - change_dis, front_padd + 4*WIGGLE_ROOM, ARENA_MAX_Y-PADDLE_WID, ARENA_MAX_Y, outPlayer->color);
        }
        else
        {
              LCD_DrawRectangle(back_padd - change_dis, back_padd, ARENA_MIN_Y, (ARENA_MIN_Y+PADDLE_WID), LCD_BLACK);
              LCD_DrawRectangle(front_padd - change_dis, front_padd + 4*WIGGLE_ROOM, ARENA_MIN_Y, (ARENA_MIN_Y+PADDLE_WID), outPlayer->color);
        }

    }
    prevPlayerIn->Center = outPlayer->currentCenter;

    return;

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
    // initialize necessary game semaphores

    G8RTOS_InitSemaphore(&playerSemaphore, 1);
    G8RTOS_InitSemaphore(&wifiSemaphore,1);

    //G8RTOS_WaitSemaphore(&lcdSemaphore);

    gameState.players[BOTTOM].position = BOTTOM;

    gameState.players[TOP].position = TOP;

    gameState.players[TOP].currentCenter = PADDLE_X_CENTER;
    gameState.players[BOTTOM].currentCenter = PADDLE_X_CENTER;

    gameState.players[TOP].color = PLAYER_RED;
    gameState.players[BOTTOM].color = PLAYER_BLUE;

    gameState.LEDScores[BOTTOM] = 0;
    gameState.LEDScores[TOP] = 0;

    gameState.gameDone = false;
    gameState.numberOfBalls = 0;
    gameState.winner = 0;

    for (uint8_t i = 0; i < MAX_NUM_OF_BALLS; i++)
    {
        gameState.balls[i].alive = false;
    }

    LCD_Clear(LCD_BLACK);

    LCD_DrawRectangle(ARENA_MIN_X-1, ARENA_MIN_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
    LCD_DrawRectangle(ARENA_MAX_X, ARENA_MAX_X+1, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);

    LCD_DrawRectangle(128, 192, ARENA_MIN_Y, (ARENA_MIN_Y+PADDLE_WID), PLAYER_RED);
    LCD_DrawRectangle(128, 192, (ARENA_MAX_Y-PADDLE_WID), ARENA_MAX_Y, PLAYER_BLUE);

    char score[4];
    LCD_Text(MIN_SCREEN_X+4, MIN_SCREEN_Y, "Red:", LCD_RED);
    sprintf(score, "%d", client_overallScore[1]);
    sprintf(score, "%d", client_overallScore[0]);
    LCD_Text(0, ARENA_MIN_Y + 15, score, gameState.players[TOP].color);
    LCD_Text(MIN_SCREEN_X, (MAX_SCREEN_Y-28), "Blue:", LCD_BLUE);
    sprintf(score, "%d", gameState.overallScores[1]);
    sprintf(score, "%d", gameState.overallScores[0]);
    LCD_Text(0, ARENA_MAX_Y - 15, score, gameState.players[BOTTOM].color);


    //G8RTOS_SignalSemaphore(&lcdSemaphore);

    return;


}

/*********************************************** Public Functions *********************************************************************/

