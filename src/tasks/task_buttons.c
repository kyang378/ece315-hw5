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
  * the event group ECE353_RTOS_Events.
  *
  * @param arg 
  * Unused parameter
  */
 void task_buttons(void *arg)
 {
    (void)arg; // Unused parameter

    // keeps track of how many times we've counted a button being pressed
    uint32_t button_count[3] = {0, 0, 0};

    while (1)
    {
        // Monitor button SW1
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) == 0) // active low buttons
        {
            button_count[0]++;

            if (button_count[0] == 2) // so a debouncing of 30ms
            {
                // this is really probably also true for other exercises...
                // we're not creating the console_task in hw02.c
                #if !defined(HW02)
                task_console_printf("SW1 pressed\n\r");
                #endif
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_BUTTON_SW1_PRESSED);
            }
        }
        else {
            button_count[0] = 0; // keeps looking for 2 consecutive falling edges
        }
        

        // Monitor button SW2
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) == 0)
        {
            button_count[1]++;

            if (button_count[1] == 2)
            {
                #if !defined(HW02)
                task_console_printf("SW2 pressed\n\r");
                #endif
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_BUTTON_SW2_PRESSED);
            }
        }
        else {
            button_count[1] = 0;
        }


        // Monitor button SW3
        if ((PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3) == 0)
        {
            button_count[2]++;

            if (button_count[2] == 2)
            {
                #if !defined(HW02)
                task_console_printf("SW3 pressed\n\r");
                #endif
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_BUTTON_SW3_PRESSED);
            }
        }
        else {
            button_count[2] = 0;
        }
  

        // Debounce delay = 15ms
        vTaskDelay(pdMS_TO_TICKS(15));

    }
 }

 /* Button Task Initialization */
bool task_button_init(void){

    BaseType_t result;

    // Initialize the buttons
    cy_rslt_t rslt = buttons_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

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
