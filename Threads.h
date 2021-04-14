#ifndef THREADS_H_
#define THREADS_H_

#include <G8RTOS_Lab5/G8RTOS.h>
#include "bmi160_support.h"
#include "bmi160.h"
#include "bme280_support.h"
#include "bme280.h"
#include "driverlib.h"
#include "i2c_driver.h"
#include "demo_sysctl.h"
#include "RGBLeds.h"

/*******lab 4*******/
#define XCOORD 0
#define YCOORD 1
#define XACCEL 2
#define YACCEL 3

#define MAX_BALLS 20
#define BALL_SIZE 10
#define HIT_BOX 30

semaphore_t accel_semaphore;
semaphore_t lcd_semaphore;
semaphore_t tp_semaphore;

bool touch;

int num_balls;
int num_ded;
// stores 16 bit signed for accelerometer
//s16 accel_x;
//s16 accel_y;
/*******************/


/*********************************************** Public Functions *********************************************************************/

// lab 4
void read_accelerometer();
void wait_tap();
void idle();
void ball_thread();

uint16_t hex_color_code();
/*********************************************** Public Functions *********************************************************************/

#endif /* THREADS_H_ */
