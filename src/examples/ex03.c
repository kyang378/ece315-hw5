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

#if defined(EX03)
#include "drivers.h"
#include "cyhal_hw_types.h"
#include "cyhal_system.h"
#include "ece353-events.h"

char APP_DESCRIPTION[] = "ECE353: Example 03 - Timer Interrupts";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

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
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    if (rslt = buttons_init_gpio() != CY_RSLT_SUCCESS) { // initialize buttons GPIO as inputs
        printf("Error initializing buttons\n\r");
        for (int i = 0; i < 1000000; i++); // delay for a whilel
        CY_ASSERT(0);
    }

    // initialize the timer for button debouncing
    if (rslt = buttons_init_timer() != CY_RSLT_SUCCESS) 
    {
        printf("Error initializing button timer\n\r");
        for (int i = 0; i < 1000000; i++); // delay for a whilel
        CY_ASSERT(0);
    }
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
    while (1)
    {
        /* check ECE353_Events for button presses */
        if (ECE353_Events.sw1 == 1) {
            ECE353_Events.sw1 = 0; // clear the event
            printf("SW1 pressed\n\r");
        }
        if (ECE353_Events.sw2 == 1) {
            ECE353_Events.sw2 = 0;
            printf("SW2 pressed\n\r");
        }
        if (ECE353_Events.sw3 == 1) {
            ECE353_Events.sw3 = 0;
            printf("SW3 pressed\n\r");
        }
    }
}
#endif