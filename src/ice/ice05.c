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

#if defined(ICE05)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 05 - FreeRTOS Event Groups";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
// ADD CODE for Event Group Bit Definitions
#include "rtos_events.h"

// include the header files for the tasks we will create
#include "task_buttons.h"
#include "task_buzzer.h"

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
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* ADD CODE Initialize the buttons */
    buttons_init_gpio();

    /* ADD CODE Initialize the buzzer */
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
    /* ADD CODE Create the event group */
    ECE353_RTOS_Events = xEventGroupCreate();
    if (ECE353_RTOS_Events == NULL) {
        // handle error
        printf("Failed to create event group\n\r");
    }

    /* ADD CODE Register the tasks with FreeRTOS*/
    BaseType_t rslt = xTaskCreate(task_buttons, 
                                "Buttons Task", 
                                configMINIMAL_STACK_SIZE, 
                                NULL, 
                                tskIDLE_PRIORITY + 1, 
                                NULL);

    if (rslt != pdPASS) {
        printf("Failed to create buttons task\n\r");
    }

    rslt = xTaskCreate(task_buzzer, 
                    "Buzzer Task", 
                    configMINIMAL_STACK_SIZE, 
                    NULL, 
                    tskIDLE_PRIORITY + 1, 
                    NULL);

    if (rslt != pdPASS) {
        printf("Failed to create buzzer task\n\r");
    }

    /* ADD CODE Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif