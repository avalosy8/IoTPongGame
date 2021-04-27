#include "msp.h"
#include "Threads.h"
#include "Game.h"
#include <driverlib.h>

// ISR
//void PORT4_IRQHandler(void)
//{
//    // code w/ threads
//    P4->IFG &= ~BIT0;
//    touch = true;
//    __NVIC_DisableIRQ(PORT4_IRQn);
//    P4->IE &= ~BIT0;
//}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

    // initialize G8RTOS
    G8RTOS_Init();

    BSP_InitBoard();
    LCD_Init(true);

    // get player role
    playerType player = GetPlayerRole();
    if(player == Host)
        G8RTOS_AddThread(CreateGame, 5, "create");
//        CreateGame();
    else if(player == Client)
        G8RTOS_AddThread(JoinGame, 5, "join");
//        JoinGame();

    // launch OS
    int retval = G8RTOS_Launch();

	while(1);
}
