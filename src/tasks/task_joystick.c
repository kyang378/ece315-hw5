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
 #include "rtos_events.h"

 QueueHandle_t Queue_Requests_Joystick = NULL;

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
  *  Task used to monitor the joystick
  * @param arg 
  */
 void task_joystick(void *arg)
{
    (void)arg; // Unused parameter
    //uint16_t x_pos, y_pos;
    joystick_position_t previous_position = JOYSTICK_POS_CENTER;
    while(1)
    {

        vTaskDelay(pdMS_TO_TICKS(50));

        /* Read the X and Y positions of the joystick
        OLD CODE 
        x_pos = joystick_read_x();
        y_pos = joystick_read_y();

        //convert to voltage 
        float x_voltage = (x_pos / 65535.0) * 3.3; 
        float y_voltage = (y_pos / 65535.0) * 3.3;
        printf("Joystick X: (%.2f V), Y: (%.2f V)\n\r", x_voltage, y_voltage); */

        joystick_position_t position = joystick_get_pos();

        if (position == JOYSTICK_POS_CENTER) {
            previous_position = JOYSTICK_POS_CENTER;
        continue;
}
        /* Trigger correct event bits */
        if (previous_position == JOYSTICK_POS_CENTER) {
            switch(position) {
                //Anything including "Left" will be treated as pure left
                case JOYSTICK_POS_LOWER_LEFT:
                case JOYSTICK_POS_UPPER_LEFT:
                case JOYSTICK_POS_LEFT:
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_JOYSTICK_LEFT);
                    break;
                //anything including "Right" will be treated as pure right    
                case JOYSTICK_POS_LOWER_RIGHT:
                case JOYSTICK_POS_UPPER_RIGHT:
                case JOYSTICK_POS_RIGHT:
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_JOYSTICK_RIGHT);
                    break;
                case JOYSTICK_POS_DOWN:
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_JOYSTICK_DOWN);
                    break;
                case JOYSTICK_POS_UP:
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_JOYSTICK_UP);
                    break;
                default:
                    printf("Unidentified joystick position");            
            }
        }
        previous_position = position;
        
    }
}


bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/
    //Queue_Joystick = xQueueCreate(1, sizeof(joystick_position_t)); //Currently unused
    /* Create the joystick task */
    if (xTaskCreate(task_joystick, "Joystick Task", 256, NULL, 2, NULL) != pdPASS)
    {
        printf("Failed to create Joystick Task\n\r");
        return false;
    }
    
    return true;
}
#endif