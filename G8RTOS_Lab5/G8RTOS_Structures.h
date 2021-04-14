/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include <G8RTOS_Lab5/G8RTOS.h>

#define ICSR (*((volatile unsigned int*)(0xe000ed04))) // offset of D04h
#define SHPR3 (*((volatile unsigned int*)(0xe000ed20)))
#define PENDSV_PRI (0x00 << 16)
#define SYSTICK_PRI (0x01 << 24)

#define MAX_NAME_LENGTH 16




/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

typedef struct tcb_t tcb_t;

struct tcb_t {
    int32_t * stack_ptr;
    tcb_t * prev_tcb;
    tcb_t * next_tcb;

    bool alive;
    uint8_t priority;
    threadId_t threadID;
    char threadName[MAX_NAME_LENGTH];

    bool asleep;
    uint32_t sleep_cnt;
    semaphore_t * blocked;        // will either be 0 (not blocked) or the semaphore that the thread is currently waiting on
};

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

typedef struct ptcb_t ptcb_t;

struct ptcb_t {
    void(*Handler)(void);
    uint32_t period;
    uint32_t exe_time;
    uint32_t curr_time;
    ptcb_t * prev_ptcb;
    ptcb_t * next_ptcb;
};


/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
