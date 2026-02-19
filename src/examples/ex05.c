/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX05)

#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 05 - FreeRTOS Tasks";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

// set to volatile because multiple tasks will access/modify it, so we want to
// make sure we get the most up-to-date value
volatile bool buzzer_enable = false;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
void task_button_sw1(void *arg);
void task_button_sw2(void *arg);
void task_buzzer(void *arg);

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

// monitor the switch and debounce the push button every 15 ms
// if pressed, ENABLE the buzzer
void task_button_sw1(void *arg)
{
    (void)arg; // Unused parameter

    // 32 => can count to 4 billion, so no worry if we pressed it for too long
    // and have it be wrapped to zero
    uint32_t button_count = 0;

    printf("task_button_sw1 created\n\r");

    while(1)
    {
        // check the button
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) == 0) // active low
        {
            button_count++;

            if (button_count == 2)
            {
                printf("SW1 Pressed --- Enabling Buzzer\n\r");
                // set a global variable that indicates the buzzer should be turned on
                buzzer_enable = true;
            }
        }
        else
        {
            button_count = 0;
        }

        // delay for 15ms by BLOCKING the task to allow other tasks to use the CPU
        vTaskDelay(pdMS_TO_TICKS(15)); // takes in number of ticks

    }
}

// same monitor and debounce as previous task; if pressed, DISABLE the buzzer
void task_button_sw2(void *arg)
{
    (void)arg; // Unused parameter

    // 32 => can count to 4 billion, so no worry if we pressed it for too long
    // and have it be wrapped to zero
    uint32_t button_count = 0;

    printf("task_button_sw2 created\n\r");

    while(1)
    {
        // check the button
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) == 0) // active low
        {
            button_count++;

            if (button_count == 2)
            {
                printf("SW2 Pressed --- Disabling Buzzer\n\r");
                // set a global variable that indicates the buzzer should be turned off
                buzzer_enable = false;
            }
        }
        else
        {
            button_count = 0;
        }

        // delay for 15ms by BLOCKING the task to allow other tasks to use the CPU
        vTaskDelay(pdMS_TO_TICKS(15)); // takes in number of ticks

    }
}

// checks buzzer_enable and handle accordingly
void task_buzzer(void *arg)
{
    (void)arg; // Unused parameter

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        if (buzzer_enable)
        {
            buzzer_on();
        }
        else
        {
            buzzer_off();
        }
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the buttons */
    buttons_init_gpio();

    /* Initialize the buzzer */
    buzzer_init(50, 2000);
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    /* Register the tasks with FreeRTOS*/
    // create the tasks
    xTaskCreate
    (
        task_button_sw1,                // function used to implement a task
        "task_button_sw1",              // task name for debugging
        configMINIMAL_STACK_SIZE,       // stack size (each task has its own stack), so need to tell the OS how much memory to allocate for the stack. This is in words, not bytes, so 1024 means 4096 bytes (4kB)
        NULL,                           // any parameters that are passed into the task
        tskIDLE_PRIORITY + 1,           // just higher than IDLE task
        NULL                            // task handle
    );
    xTaskCreate
    (
        task_button_sw2, 
        "task_button_sw2", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );
    xTaskCreate
    (
        task_buzzer,
        "task_buzzer", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    /* Start the scheduler*/
    vTaskStartScheduler(); // will start executing these tasks in a round-robin fashion, and will never return from this function

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif