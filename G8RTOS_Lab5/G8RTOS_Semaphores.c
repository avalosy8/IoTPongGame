/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Semaphores.h"
#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Structures.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    int32_t IBit_State = StartCriticalSection(); // disable interrupts
    *s = value;
    EndCriticalSection(IBit_State); // enable interrupts
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread is sempahore is unavalible
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    int32_t IBit_State = StartCriticalSection();    // disable interrupts

	(*s)--;     // semaphore available, decrement it

	if((*s) < 0)    // semaphore is unavailable
	{
	    CurrentlyRunningThread->blocked = s;        // block thread

	    ICSR |= SCB_ICSR_PENDSVSET_Msk;             // yield to allow another thread to run
	}
    EndCriticalSection(IBit_State);                 // enable interrupts
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    int32_t IBit_State = StartCriticalSection();

    (*s)++;
    if((*s) <= 0)   // semaphore is unavailable, thread is waiting on it
    {
        tcb_t *temp_tcb = CurrentlyRunningThread->next_tcb;
        while(temp_tcb->blocked != s)   // look for next thread that is blocked by the same semaphore
            temp_tcb = temp_tcb->next_tcb;
        if(temp_tcb->blocked == s)          // making sure we're unblocking the right thread
            temp_tcb->blocked = 0;          // unblock the thread that's waiting on the same semaphore
    }

    EndCriticalSection(IBit_State);
}

/*********************************************** Public Functions *********************************************************************/


