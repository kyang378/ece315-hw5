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
    (void)arg; // Unused parameter

    EventBits_t uxBits; // current event bits

    while (1)
    {
        // wait for either SW1 pressed or S2 pressed event
        uxBits = xEventGroupWaitBits(ECE353_RTOS_Events,
                                ECE353_EVENT_BUTTON_SW1_PRESSED | ECE353_EVENT_BUTTON_SW2_PRESSED,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);

        if (uxBits & ECE353_EVENT_BUTTON_SW1_PRESSED) {
            printf("SW1 Pressed --- Enabling Buzzer\n\r");
            buzzer_on();
        }

        if (uxBits & ECE353_EVENT_BUTTON_SW2_PRESSED) {
            printf("SW2 Pressed --- Disabling Buzzer\n\r");
            buzzer_off();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
#endif