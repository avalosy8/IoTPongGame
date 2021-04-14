#include "msp.h"
#include "Threads.h"
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

//	// flag to indicate touch occurred
//	touch = false;
//	// keeps track of total number of dead thread (used in AddThread)
//	num_ded = 0;
//	// keep track total balls, used for array of balls
//	num_balls = 0;
//
//    // initialize G8RTOS
//    G8RTOS_Init();
//
//    // initialize semaphore
//    G8RTOS_InitSemaphore(&accel_semaphore, 1);
//	G8RTOS_InitSemaphore(&lcd_semaphore, 1);
//    G8RTOS_InitSemaphore(&tp_semaphore, 1);
//
//    BSP_InitBoard();
//    LCD_Init(true);
//
//    // initialize FIFOs
//    int ret;
//    ret = G8RTOS_InitFIFO(XCOORD);
//    ret = G8RTOS_InitFIFO(YCOORD);
//    ret = G8RTOS_InitFIFO(XACCEL);
//    ret = G8RTOS_InitFIFO(YACCEL);
//
//    // add threads
//    void (*read_accel_ptr)(void) = &read_accelerometer;
//    void (*wait_tap_ptr)(void) = &wait_tap;
//    void (*idle_ptr)(void) = &idle;
//    void (*irq_ptr)(void) = &PORT4_IRQHandler;
//
//    G8RTOS_AddThread(read_accel_ptr, 1, "accel");
//    G8RTOS_AddThread(wait_tap_ptr, 1, "wait");
//    G8RTOS_AddThread(idle_ptr, 6, "idle");
//
//    G8RTOS_AddAPeriodicEvent(irq_ptr, 5, PORT4_IRQn);
//
//    // launch OS
//    int retval = G8RTOS_Launch();

	while(1);
}
