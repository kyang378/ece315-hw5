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
#include "FreeRTOSConfig.h"
#if defined(EX05)

#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 05 - FreeRTOS Tasks";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
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
void task_button_sw1(void *arg)
{
    (void)arg; // Unused parameter //Not in example, delete if dysfunctional
    uint32_t button_count = 0;
    printf("SW1 Task Created\n\r");

    while(1)
    {
        //check the button
        if((PORT_BUTTON_SW1->IN & MASK_BUTTON_SW1) == 0) {
            button_count++;

            if(button_count == 2) {
                printf("SW1 Pressed - buzzer enabled\n\r");
                //set a global variable
                buzzer_enable = true;
            } 
        } else {
            button_count = 0;
        }
        //delay 15ms
        vTaskDelay(pdMS_TO_TICKS(15));

    }

}

void task_button_sw2(void *arg)
{
    (void)arg; // Unused parameter
    uint32_t button_count = 0;
    printf("SW2 Task Created\n\r");

    while (1)
    {
        //check the button
        if((PORT_BUTTON_SW2->IN & MASK_BUTTON_SW2) == 0) {
            button_count++;

            if(button_count == 2) {
                printf("SW2 Pressed - buzzer disabled\n\r");
                //set a global variable
                buzzer_enable = false;
            } 
        } else {
            button_count = 0;
        }
        //delay 15ms
        vTaskDelay(pdMS_TO_TICKS(15));

    }
}

void task_buzzer(void *arg)
{
    (void)arg; // Unused parameter
    printf("Buzzer Task Created\n\r");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        if(buzzer_enable) {
            buzzer_on(); //may need to write this method
        } else {
            buzzer_off(); //may need to write this method
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
    buzzer_init(2000);
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
    xTaskCreate(task_button_sw1, "SW1 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_button_sw2, "SW2 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_buzzer, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif