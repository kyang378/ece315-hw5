/**
 * @file task_joystick.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "main.h"

#ifdef ECE353_FREERTOS  
#include "drivers.h"
 #include "task_joystick.h"

 QueueHandle_t Queue_Joystick = NULL;
 joystick_position_t current_position = JOYSTICK_POS_CENTER; // initialized to center

/* Message lookup table for joystick positions */
const char * const joystick_pos_names[] = {
    "Center",
    "Left",
    "Right",
    "Up",
    "Down",
    "Upper Left",
    "Upper Right",
    "Lower Left",
    "Lower Right"
};

 /**
  * @brief 
  *  Task used to monitor the joystick twice a second and print out to the user
  * @param arg 
  */
 void task_joystick(void *arg)
{
    (void)arg; // Unused parameter

    // size = 1, holds joystick_position_t data
    Queue_Joystick = xQueueCreate(1, sizeof(joystick_position_t));

    char msg[50]; // buffer to hold the position

    uint16_t x, y; // stores the raw 16-bit values

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); // sample the joystick every 500 ms

        x = joystick_read_x();
        y = joystick_read_y();

        joystick_position_t position = joystick_get_pos();
        if (position != current_position) {
            current_position = position; // update current position
            xQueueSend(Queue_Joystick, &position, 0); // send position to queue if changed
        }

    }
}


bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/

    /* Create the joystick task */
    
    return true;
}
#endif