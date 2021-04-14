/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include <driverlib.h>

#include "msp.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_Structures.h"

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*
 * Pointer to the linked list of pthreads
 */

ptcb_t * pthreads;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAX_PTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/*
 * static variable for creating threadID
 */
static uint16_t IDCounter = 0;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
    SysTick_Config(numCycles); // automatically configs SysTick to lowest priority value
    SysTick_enableInterrupt();
}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 * 	- Check for sleeping and blocked threads
 */
void G8RTOS_Scheduler()
{
    tcb_t *temp_thread = CurrentlyRunningThread->next_tcb;
    uint8_t max = 255;
    // find thread not block or asleep
    while((temp_thread->blocked != 0) || (temp_thread->asleep))
        temp_thread = temp_thread->next_tcb;

    // update max priority and CRT
    max = temp_thread->priority;
    CurrentlyRunningThread = temp_thread;

    // ensure highest priority is given to threads
    int i;
    for(i = 0; i < NumberOfThreads; i++)
    {
        temp_thread = temp_thread->next_tcb;
        if(!temp_thread->asleep && (temp_thread->blocked == 0))
        {
            if(temp_thread->priority < max)
            {
                max = temp_thread->priority;
                CurrentlyRunningThread = temp_thread;
            }
        }
    }
/*
    // from the book
    uint8_t max = 255;
    tcb_t *pt;
    tcb_t *best_pt;

    pt = CurrentlyRunningThread;
    // search for the highest thread not blocked or sleeping
    do{
        pt = pt->next_tcb; // skips at least once
        if((pt->priority < max) && ((pt->blocked) == 0) && (!pt->asleep))
        {
            max = pt->priority;
            best_pt = pt;
        }
    } while(pt != CurrentlyRunningThread); // look at all possible threads
    CurrentlyRunningThread = best_pt;
*/

/*
    // first attempt
    uint8_t next_priority = 255;
    tcb_t *original_crt = CurrentlyRunningThread;
    tcb_t *next_thread = CurrentlyRunningThread->next_tcb;
    int i = 0;
    while(next_thread != original_crt)
    {
        // check if next thread is not sleeping or blocked
        if(!next_thread->next_tcb->asleep && (next_thread->next_tcb->blocked == 0))
        {
            // check if priority is higher that current max
            if(next_thread->next_tcb->priority < next_priority)
            {
                // set CRT to be the next thread to run
                CurrentlyRunningThread = next_thread;
                next_priority = CurrentlyRunningThread->priority;
            }
        }
        next_thread = next_thread->next_tcb;
    }
*/
}


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */

void SysTick_Handler()
{
    ++SystemTime;

    // check if pthreads can run, if so reset exe_time so it can run again at it's next period

    int i;
    for(i = 0; i < NumberOfPthreads; i++)
    {
        if(Pthread[i].exe_time == SystemTime) // time for Pthread to execute
        {
            Pthread[i].exe_time = Pthread[i].period + SystemTime; // set new exe_time for when to run pthread again

            // run pthread function
            Pthread[i].Handler();
        }
    }

    // check every sleeping threads' sleep count
    // if thread's sleep_cnt = current SystemTime, wake up thread, set sleep_cnt = 0
    tcb_t * tcb_ptr = CurrentlyRunningThread;
    for(i = 0; i < NumberOfThreads; i++)
    {
        if(tcb_ptr->asleep)
            if(tcb_ptr->sleep_cnt == SystemTime)
            {
                tcb_ptr->asleep = false;
                tcb_ptr->sleep_cnt = 0;
            }
        tcb_ptr = tcb_ptr->next_tcb;
    }

    ICSR |= SCB_ICSR_PENDSVSET_Msk; // context switch enabled
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPthreads = 0;

    CurrentlyRunningThread = &threadControlBlocks[0];

    // relocate interrupt vector to SRAM
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4); // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */

int G8RTOS_Launch()
{
    // initialize SysTick and it's priority
    uint32_t currClk = ClockSys_GetSysFreq();
    uint32_t cycles = 0.001 * currClk;
    InitSysTick(cycles);

    // set PendSV and SysTick priority
    SCB->SHP[11] = 0x80;
    SCB->SHP[10] = 0xe0;

    // launch the highest priority thread
    uint8_t next_priority = 255;

    if(NumberOfThreads <= 0)
        return NO_THREADS_SCHEDULED;

    // need at least one thread to start checking priorities
    CurrentlyRunningThread = &threadControlBlocks[0];

    int i = 0;
    for(i = 0; i < NumberOfThreads; i++)
    {
        // check if priority is higher that current max
        if(CurrentlyRunningThread->next_tcb->priority < next_priority)
        {
            // set CRT to be the next thread to run
            CurrentlyRunningThread = CurrentlyRunningThread->next_tcb;
            next_priority = CurrentlyRunningThread->priority;
        }
    }

    // load CurrentlyRunningThread's context into CPU & enables interrupts
    G8RTOS_Start();

    return 0;
}

int32_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char threadName[])
{
    int32_t IBit_State = StartCriticalSection();

    if(NumberOfThreads+1 > MAX_THREADS)
    {
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;
    }

    // check for any spots open in tcb
    int tcb_index = MAX_THREADS;
    int i;
    for(i = 0; i < MAX_THREADS; i++)
    {
        if(!threadControlBlocks[i].alive)
        {
            tcb_index = i;
            break;
        }
    }

    // check if incorrectly all alive
    if(tcb_index == MAX_THREADS)
    {
        EndCriticalSection(IBit_State);
        return THREADS_INCORRECTLY_ALIVE;
    }

    else
    {
        threadStacks[tcb_index][STACKSIZE-2] = (int32_t)threadToAdd;  // PC value
        threadStacks[tcb_index][STACKSIZE-1] = THUMBBIT;              // PSR value

        threadControlBlocks[tcb_index].stack_ptr = &threadStacks[tcb_index][STACKSIZE-16]; // points to top of stack
        threadControlBlocks[tcb_index].alive = true;
        threadControlBlocks[tcb_index].priority = priority;
        threadControlBlocks[tcb_index].threadID = ((IDCounter++) << 16) | tcb_index; // this makes sense

        int src_size = sizeof(threadName);
        strncpy(threadControlBlocks[tcb_index].threadName, threadName, src_size);

        threadControlBlocks[tcb_index].asleep = false;
        threadControlBlocks[tcb_index].sleep_cnt = 0;
        threadControlBlocks[tcb_index].blocked = 0;

        if(NumberOfThreads == 0) // check if adding first thread
        {
            threadControlBlocks[tcb_index].prev_tcb = &threadControlBlocks[tcb_index];
            threadControlBlocks[tcb_index].next_tcb = &threadControlBlocks[tcb_index];
        }

        else // add using CRT
        {
            threadControlBlocks[tcb_index].prev_tcb = CurrentlyRunningThread;
            threadControlBlocks[tcb_index].next_tcb = CurrentlyRunningThread->next_tcb;
            CurrentlyRunningThread->next_tcb = &threadControlBlocks[tcb_index];
            threadControlBlocks[tcb_index].next_tcb->prev_tcb = &threadControlBlocks[tcb_index];
        }
    }
    NumberOfThreads++;
    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    int32_t IBit_State = StartCriticalSection();
    if(NumberOfPthreads+1 > MAX_PTHREADS)
    {
        EndCriticalSection(IBit_State);
        return -1;
    }

    // first thread being added
    if(NumberOfPthreads == 0)
    {
        Pthread[0].Handler = PthreadToAdd;
        Pthread[0].period = period;
        Pthread[0].exe_time = SystemTime + period;
        Pthread[0].curr_time = SystemTime;
        Pthread[0].prev_ptcb = &Pthread[0];
        Pthread[0].next_ptcb = &Pthread[0];
    }

    // more than one thread
    else
    {
        Pthread[NumberOfPthreads].Handler = PthreadToAdd;
        Pthread[NumberOfPthreads].period = period;
        Pthread[NumberOfPthreads].exe_time = SystemTime + period + NumberOfPthreads;  // initial val of exe time will be different for each pthread
        Pthread[NumberOfPthreads].curr_time = SystemTime;                               // used NumPthreads to stagger exe times

        Pthread[0].prev_ptcb = &Pthread[NumberOfPthreads];                         // first tcb's prev ptr points to new thread
        Pthread[NumberOfPthreads].prev_ptcb = &Pthread[NumberOfPthreads-1];         // new thread's prev ptr points to last added thread
        Pthread[NumberOfPthreads].next_ptcb = &Pthread[0];                         // new thread's next ptr points to first tcb
        Pthread[NumberOfPthreads-1].next_ptcb = &Pthread[NumberOfPthreads];         // previous thread's next ptr points to new thread
    }

    NumberOfPthreads++;

    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    CurrentlyRunningThread->sleep_cnt = durationMS + SystemTime;
    CurrentlyRunningThread->asleep = true;
    // yield control of this thread to allow other threads to run (start context switch)
    ICSR |= SCB_ICSR_PENDSVSET_Msk; // trigger PendSV to do context switch
}

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId)
{
    int32_t IBit_State = StartCriticalSection();

    // ensure last thread isn't being killed
    if(NumberOfThreads == 1)
    {
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }

    // start of 2nd code attempt

    // find index of thread to kill from tcb
    uint32_t to_kill = MAX_THREADS;
    bool found = false;
    int i;
    for(i = 0; i < MAX_THREADS; i++)
    {
        if(threadControlBlocks[i].threadID == threadId)
        {
            found = true;
            to_kill = i;
            break;
        }
    }

    // if thread found, kill it
    if(to_kill < MAX_THREADS)
    {
        threadControlBlocks[to_kill].alive = false;
        threadControlBlocks[to_kill].next_tcb->prev_tcb = threadControlBlocks[to_kill].prev_tcb;
        threadControlBlocks[to_kill].prev_tcb->next_tcb = threadControlBlocks[to_kill].next_tcb;
        NumberOfThreads--;

        EndCriticalSection(IBit_State);
        // check if not CRT, if so context switch
        if(threadControlBlocks[to_kill].threadID == CurrentlyRunningThread->threadID)
            ICSR |= SCB_ICSR_PENDSVSET_Msk; // context switch
    }

    // otherwise threadID doesn't exist
    else
    {
        EndCriticalSection(IBit_State);
        return THREAD_DOES_NOT_EXIST;
    }
    return NO_ERROR;

    /*// first attempt
    // search for thread with the same threadId
    int i;
    bool foundThread = false;
    bool isCRT = false;
    tcb_t * thread_ptr = CurrentlyRunningThread;

    for(i = 0; i < NumberOfThreads; i++)
    {
        if(thread_ptr->threadID = threadId)
        {
            foundThread = true;
            thread_ptr->alive = false;

            // update TCB so threads aren't pointing to dead thread
            tcb_t * temp_ptr = thread_ptr->next_tcb;
            thread_ptr->prev_tcb->next_tcb = temp_ptr;
            temp_ptr->prev_tcb = thread_ptr->prev_tcb;
            // left off thread_ptr's next and prev still pointing...

            // check if killed thread is CRT
            if(CurrentlyRunningThread->threadID = thread_ptr->threadID)
                isCRT = true;
                //ICSR |= SCB_ICSR_PENDSVSET_Msk; // context switch enabled

            NumberOfThreads--;
            break;
        }
        thread_ptr = thread_ptr->next_tcb;
    }

    // ensure threadId exists
    if(!foundThread)
    {
        EndCriticalSection(IBit_State);
        return THREAD_DOES_NOT_EXIST;
    }

    EndCriticalSection(IBit_State);

    if(isCRT)
        ICSR |= SCB_ICSR_PENDSVSET_Msk; // context switch
    return NO_ERROR;
    */
}

sched_ErrCode_t G8RTOS_KillSelf()
{
    return G8RTOS_KillThread(CurrentlyRunningThread->threadID);
    /*
    int32_t IBit_State = StartCriticalSection();

    // ensure last thread isn't being killed
    if(NumberOfThreads == 1)
        return CANNOT_KILL_LAST_THREAD;

    CurrentlyRunningThread->alive = false;

    // update TCB so threads aren't pointing to dead thread
    tcb_t * temp_ptr = CurrentlyRunningThread->next_tcb;
    CurrentlyRunningThread->prev_tcb->next_tcb = temp_ptr;
    temp_ptr->prev_tcb = CurrentlyRunningThread->prev_tcb;
    // left off thread_ptr's next and prev still pointing...

    NumberOfThreads--;

    EndCriticalSection(IBit_State);

    ICSR |= SCB_ICSR_PENDSVSET_Msk; // context switch enabled

    return NO_ERROR;
    */
}

threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->threadID;
}

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void(*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn)
{
    int32_t IBit_State = StartCriticalSection();

    // verify IRQn valid
    if(IRQn < PSS_IRQn || IRQn > PORT6_IRQn)
    {
        EndCriticalSection(IBit_State);
        return IRQn_INVALID;
    }

    // verify priority not > 6 (the greatest priority)
    if(priority > 6)
    {
        EndCriticalSection(IBit_State);
        return HWI_PRIORITY_INVALID;
    }

    // initialize NVIC registers
    __NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);
    __NVIC_SetPriority(IRQn, priority);
    __NVIC_EnableIRQ(IRQn);

    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

/*********************************************** Public Functions *********************************************************************/
