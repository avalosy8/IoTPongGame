#include <stdint.h>
#include <time.h>
#include "msp.h"
#include "Threads.h"
#include "LCDLib.h"


/*
 * Ball struct
 */

typedef struct ball ball;
struct ball {
    uint16_t x;
    uint16_t y;
    int16_t x_v;
    int16_t y_v;
    bool killed;
    bool alive;
    threadId_t threadID;
    uint32_t color;
};

static ball balls[MAX_BALLS];

/*******lab 4*******/

void read_accelerometer()
{
    while(1){
    // read x and y from accelerometer, updates global vars
    BMI160_RETURN_FUNCTION_TYPE com_rslt = ERROR;

    s16 accel_x = 0x0000;
    s16 accel_y = 0x0000;

    G8RTOS_WaitSemaphore(&accel_semaphore);        // wait for sensor semaphore
    com_rslt = bmi160_read_accel_x(&accel_x);       // read from accel's x-axis and save value locally
    com_rslt = bmi160_read_accel_y(&accel_y);       // read from accel's y-axis and save value locally
    G8RTOS_SignalSemaphore(&accel_semaphore);      // release sensor semaphore

    // store in FIFOs
    writeFIFO(XACCEL, accel_x);
    writeFIFO(YACCEL, accel_y);

    // sleep some amount of time
    sleep(50);

    } // end while
}

void wait_tap()
{
    while(1) {
    // waits for a touch to occur
    if(touch)
    {
        // read touch coordinates
        G8RTOS_WaitSemaphore(&lcd_semaphore);
        Point p = TP_ReadXY();
        G8RTOS_SignalSemaphore(&lcd_semaphore);

        // determine whether to delete or add ball
        bool killed_ball = false;
        // check if (x,y) is within any of the ball's hit boxes
        int i;
        for(i = 0; i < num_balls; i++)
        {
            // check if p is within an alive ball's hit box
            if(balls[i].alive) {
            if((p.x >= (balls[i].x - HIT_BOX) && p.x <= (balls[i].x + HIT_BOX)) && (p.y >= (balls[i].y - HIT_BOX) && p.y <= (balls[i].y + HIT_BOX)))
            {
                // ball is hit
                // wait for ball_thread to finish using it's semaphores by checking isAlive
                balls[i].alive = false;
                balls[i].killed = false; // not yet killed...

                num_balls--;

                // erase it from screen
                G8RTOS_WaitSemaphore(&lcd_semaphore);
                LCD_DrawRectangle(balls[i].x, balls[i].x + BALL_SIZE, balls[i].y, balls[i].y + BALL_SIZE, 0x0000);
                G8RTOS_SignalSemaphore(&lcd_semaphore);

                killed_ball = true;
                break;
            }
            }
        }

        // (x,y) isn't hitting a ball, create a new ball at that position
        if(!killed_ball & (num_balls < MAX_BALLS)) {
        // write coordinates to a FIFO
        writeFIFO(XCOORD, p.x);
        writeFIFO(YCOORD, p.y);

        // create a ball thread
        void (*ball_thread_ptr)(void) = &ball_thread;
        G8RTOS_AddThread(ball_thread_ptr, 1, "ball");
        }

        sleep(500);
        P4->IFG &= ~BIT0;
        P4->IE |= BIT0;
        __NVIC_EnableIRQ(PORT4_IRQn);
        touch = false;
    }

    } // end while
}

// lowest priority
void idle()
{
    while(1);
}

uint16_t hex_color_code()
{
    uint16_t rand_color = rand() % 65535;
    return rand_color;
}

void ball_thread()
{
    // read ball FIFO, init coordinates
    uint32_t x = readFIFO(XCOORD);
    uint32_t y = readFIFO(YCOORD);

    // get threadID, store it (use a struct)
    uint32_t threadID = G8RTOS_GetThreadId(); // gets CRT's id

    // index for ball struct array
    int index;

    // check for next available spot in array
    int i;
    for(i = 0; i < MAX_BALLS; i++)
    {
        if(!balls[i].alive)
        {
            index = i;
            break;
        }
    }

    // this works
    balls[index].x = x;
    balls[index].y = y;
    balls[index].x_v = rand() % 10 + 1;
    balls[index].y_v = rand() % 10 + 1;
    balls[index].alive = true;
    balls[index].killed = false;
    balls[index].threadID = threadID;
    balls[index].color = hex_color_code();

    num_balls++;

    while(1)
    {
        // check if ball is alive
        if(balls[index].alive) {

        // clear ball at current position
        G8RTOS_WaitSemaphore(&lcd_semaphore);
        LCD_DrawRectangle(balls[index].x, balls[index].x + BALL_SIZE, balls[index].y, balls[index].y + BALL_SIZE, 0x0000);
        G8RTOS_SignalSemaphore(&lcd_semaphore);

        // alter position depending on velocity
        // check accel x and y values, update velocity based on that
        int32_t accel_x = readFIFO(XACCEL);
        int32_t accel_y = readFIFO(YACCEL);

        // checking x accel values, (-left +right)
        if(accel_x > 14000)
        {
            balls[index].x = balls[index].x + 10;
        }
        else if(14000 > accel_x & accel_x > 10000)
        {
            balls[index].x = balls[index].x + 8;
        }
        else if(10000 > accel_x & accel_x > 6000)
        {
            balls[index].x = balls[index].x + 6;
        }
        else if(6000 > accel_x & accel_x > 2000)
        {
            balls[index].x = balls[index].x + 2;
        }
        else if(2000 > accel_x & accel_x > 0) // center, use default velocity
        {
            balls[index].x = balls[index].x + balls[index].x_v;
        }
        else if(accel_x < -14000) //
        {
            balls[index].x = balls[index].x - 10;
        }
        else if(-14000 < accel_x & accel_x < -10000)
        {
            balls[index].x = balls[index].x - 8;
        }
        else if(-10000 < accel_x & accel_x < -6000)
        {
            balls[index].x = balls[index].x - 6;
        }
        else if(-6000 < accel_x & accel_x < -2000)
        {
            balls[index].x = balls[index].x - 2;
        }
        else if(-2000 < accel_x & accel_x < 0)
        {
            balls[index].x = balls[index].x - balls[index].x_v;
        }
        // checking y accel values (+up, -down)
        if(accel_y > 14000)
        {
            balls[index].y = balls[index].y - 10;
        }
        else if(14000 > accel_y & accel_y > 10000)
        {
            balls[index].y = balls[index].y - 8;
        }
        else if(10000 > accel_y & accel_y > 6000)
        {
            balls[index].y = balls[index].y - 6;
        }
        else if(6000 > accel_y & accel_y > 2000)
        {
            balls[index].y = balls[index].y - 2;
        }
        else if(2000 > accel_y & accel_y > 0) // center, use default velocity
        {
            balls[index].y = balls[index].y + balls[index].y_v;
        }
        else if(accel_y < -14000) //
        {
            balls[index].y = balls[index].y + 10;
        }
        else if(-14000 < accel_y & accel_y < -10000)
        {
            balls[index].y = balls[index].y + 8;
        }
        else if(-10000 < accel_y & accel_y < -6000)
        {
            balls[index].y = balls[index].y + 6;
        }
        else if(-6000 < accel_y & accel_y < -2000)
        {
            balls[index].y = balls[index].y + 2;
        }
        else if(-2000 < accel_y & y < 0)
        {
            balls[index].y = balls[index].y + balls[index].y_v;
        }

        // touches borders
        if(balls[index].x >= MAX_SCREEN_X)
            balls[index].x = MIN_SCREEN_X + 40;
        else if(balls[index].x <= MIN_SCREEN_X)
            balls[index].x = MAX_SCREEN_X - 40;
        if(balls[index].y >= MAX_SCREEN_Y)
            balls[index].y = MIN_SCREEN_Y + 40;
        else if(balls[index].y <= MIN_SCREEN_Y)
            balls[index].y = MAX_SCREEN_Y - 40;

        // update ball on screen
        G8RTOS_WaitSemaphore(&lcd_semaphore);
        LCD_DrawRectangle(balls[index].x, balls[index].x + BALL_SIZE, balls[index].y, balls[index].y + BALL_SIZE, balls[index].color);
        G8RTOS_SignalSemaphore(&lcd_semaphore);

        // sleep for ~30ms
        sleep(30);
        }

        else { // ball is not alive
            // clear previous position
            G8RTOS_WaitSemaphore(&lcd_semaphore);
            LCD_DrawRectangle(balls[index].x, balls[index].x + BALL_SIZE, balls[index].y, balls[index].y + BALL_SIZE, 0x0000);
            G8RTOS_SignalSemaphore(&lcd_semaphore);

            balls[index].killed = true; // about to kill

            G8RTOS_KillSelf();
        }
    }
}

/*******************/
