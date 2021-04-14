/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <stdint.h>
#include "msp.h"
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 3
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

typedef struct FIFO_t FIFO_t;

struct FIFO_t {
    int32_t buffer[FIFOSIZE];
    int32_t * head;
    int32_t * tail;
    uint32_t lost_data;
    semaphore_t curr_size;
    semaphore_t mutex;
};

/* Array of FIFOS */
static FIFO_t FIFOs[4];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    if(FIFOIndex >= MAX_NUMBER_OF_FIFOS)
        return -1;
    // initialize FIFO_t
    FIFOs[FIFOIndex].head = &FIFOs[FIFOIndex].buffer[0];    // beginning of buffer
    FIFOs[FIFOIndex].tail = &FIFOs[FIFOIndex].buffer[0];    // end of buffer
    FIFOs[FIFOIndex].lost_data = 0;
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].curr_size, 0);
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].mutex, 1);
    return 0;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
uint32_t readFIFO(uint32_t FIFOChoice)
{
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].curr_size); // wait for FIFO to have data

    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].mutex);     // wait for FIFO to be available

    // note: thread can become blocked waiting for FIFO to obtain data that can be read

    // read from FIFO
    uint32_t ret = *FIFOs[FIFOChoice].head;

    // update head pointer (wrap if head is pointing to the end)
    if(FIFOs[FIFOChoice].head >= &FIFOs[FIFOChoice].buffer[FIFOSIZE-1])
        FIFOs[FIFOChoice].head = &FIFOs[FIFOChoice].buffer[0]; // point to the beginning
    else
        FIFOs[FIFOChoice].head++;

    // just added
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].curr_size); // wait for FIFO to have data

    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].mutex); // signal mutex so other waiting threads can read

    // return data
    return ret;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if necessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    *FIFOs[FIFOChoice].tail = Data;     // add data to the tail

    // update tail pointer (wrap if tail is pointing to the end)
    if(FIFOs[FIFOChoice].tail >= &FIFOs[FIFOChoice].buffer[FIFOSIZE-1]) // end of buffer
    {
        FIFOs[FIFOChoice].lost_data++;       // overwriting old data
        FIFOs[FIFOChoice].tail = &FIFOs[FIFOChoice].buffer[0];
        return -1;
    }
    else
    {
        FIFOs[FIFOChoice].tail++;
        G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].curr_size);
    }
    return 0;
}

