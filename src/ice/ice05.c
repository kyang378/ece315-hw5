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
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_buzzer.h"

#if defined(ICE05)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 05 - FreeRTOS Event Groups";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
// ADD CODE for Event Group Bit Definitions

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
// ADD CODE for Event Group Handle
EventGroupHandle_t ECE353_RTOS_Events;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    //cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* ADD CODE Initialize the buttons */
    buttons_init();

    /* ADD CODE Initialize the buzzer */
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
    /* ADD CODE Create the event group */
        ECE353_RTOS_Events = xEventGroupCreate();
        if (ECE353_RTOS_Events == NULL) {
            printf("Failed to create event group\n\r");
            while (1); // Loop forever if event group creation fails
        }

    /* ADD CODE Register the tasks with FreeRTOS*/
    if (!task_button_init()) {
        printf("Failed to create button task\n\r");
        while (1); // Loop forever if task creation fails
    }
    if (!task_buzzer_init()) {
        printf("Failed to create buzzer task\n\r");
        while (1); // Loop forever if task creation fails
    }

    /* ADD CODE Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif