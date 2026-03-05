/**
 * @file task_buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "main.h"
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_console.h"


 #ifdef ECE353_FREERTOS
 /**
  * @brief 
  * Task used to debounce button presses (SW1, SW2, SW3).  
  * The falling edge of the button press is detected by de-bouncing
  * the button for 30mS. Each button should be sampled every 15mS.
  *
  * When a button press is detected, the corresponding event is set in
  * in the event group ECE353_RTOS_Events.
  *
  * @param arg 
  * Unused parameter
  */
 void task_buttons(void *arg)
 {
    (void)arg; // Unused parameter

    while (1)
    {
        // Monitor button SW1
        if (buttons_get_state(BUTTON_SW1) == BUTTON_STATE_FALLING_EDGE)
        {
            task_console_printf("SW1 Pressed \n");
            xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW1_PRESSED);
        }

        // Monitor button SW2 
        if (buttons_get_state(BUTTON_SW2) == BUTTON_STATE_FALLING_EDGE)
        {
            task_console_printf("SW2 Pressed\n");
            xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW2_PRESSED);
        }

        // Monitor button SW3
        if (buttons_get_state(BUTTON_SW3) == BUTTON_STATE_FALLING_EDGE)
        {
            task_console_printf("SW3 Pressed\n");
            xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW3_PRESSED);
        }

        // Debounce delay
        vTaskDelay(pdMS_TO_TICKS(30));
    }
 }

 /* Button Task Initialization */
bool task_button_init(void){

    BaseType_t result;

    buttons_init();

    // Create the button task 
    result = xTaskCreate(
        task_buttons, 
        "Button Task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    if(result != pdPASS)
    {
        return false;
    }

    return true;
}
#endif