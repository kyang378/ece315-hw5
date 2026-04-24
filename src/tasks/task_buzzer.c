/**
 * @file task_buzzer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#ifdef ECE353_FREERTOS

#include "task_buzzer.h"

/**
 * @brief 
 * Task used to control the buzzer based on button events.
 * 
 * SW1 -- Turn buzzer on
 * SW2 -- Turn buzzer off
 *
 * @param arg 
 * Unused parameter
 */
void task_buzzer(void *arg)
{
    (void)arg;

    while (1)
    {
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_SW1_PRESSED | ECE353_EVENT_SW2_PRESSED,
            pdTRUE,     // clear bits on exit
            pdFALSE,    // wait for ANY
            portMAX_DELAY
        );

        if (events & ECE353_EVENT_SW1_PRESSED)
        {
            buzzer_on();
        }

        if (events & ECE353_EVENT_SW2_PRESSED)
        {
            buzzer_off();
        }
    }
}

/* Buzzer Task Initialization */
bool task_buzzer_init(void){
    BaseType_t result;

    // Create the button task
    result = xTaskCreate(
        task_buzzer, 
        "Buzzer Task", 
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