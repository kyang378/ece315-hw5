/**
 * @file ice02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE03)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 03 - Timer Interrupts/Debounce Buttons";

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

    // initialize LEDs, buttons, and button timer
    if ((rslt = leds_init_gpio()) != CY_RSLT_SUCCESS) {
        printf("Error initializing LEDs\n\r");
        for (int i = 0; i < 1000000; i++); // delay for a while
        CY_ASSERT(0);
    }

    if ((rslt = buttons_init_gpio()) != CY_RSLT_SUCCESS) {
        printf("Error initializing buttons\n\r");
        for (int i = 0; i < 1000000; i++); // delay for a while
        CY_ASSERT(0);
    }

    if ((rslt = buttons_init_timer()) != CY_RSLT_SUCCESS) {
        printf("Error initializing button timer\n\r");
        for (int i = 0; i < 1000000; i++); // delay for a while
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
    uint32_t button_press_count = 0;

    // simple FSM to track button events and set LED states accordingly
    typedef enum {
        INIT,
        SW1_DET,
        SW2_DET_1,
        SW2_DET_2,
        SW3_DET
    } fsm_state_t;

    // reset to INIT state with red led ON
    fsm_state_t current_state = INIT;
    leds_set_state(LED_RED, LED_STATE_ON);

    while (1)
    {
        // state transition logic applies when at least one button event is detected
        if (ECE353_Events.sw1 || ECE353_Events.sw2 || ECE353_Events.sw3) {
            // flow: button timer triggers timer interrupt every 5ms. When we have 5 lows
            // (asserted) for a particular button, we set that button pressed event to be 1
            // thus triggering this if statement
            printf("Button press #%u\n\r", ++button_press_count);

            // The diagram does not specify what to do if multiple buttons are pressed at the same time, so we make arbitrary decisions about priority of branches here.

            // We need two switch statements, one for state transitions and one for which leds to toggle based on the state we just transitioned to. 
            
            // This also means the one for setting leds would have to be after this switch statement. We have to do this because we need to know what the current state is before we can set the leds accordingly. 

            // Setting the leds within the same switch statement would not work because the switch statement is only triggered when a button event occurs, so we may go to another state, therefore the state we are in while the switch statement is triggered may not be the "current" state we want to set the leds for.

            /// state transition logic ///
            switch (current_state) {
                case INIT:
                    if (ECE353_Events.sw1) {
                        current_state = SW1_DET;
                    } // (otherwise stay in INIT)             
                    break;
                case SW1_DET:
                    if (ECE353_Events.sw2) {
                        ECE353_Events.sw2 = 0;
                        current_state = SW2_DET_1;
                    } else {
                        current_state = INIT;
                    }
                    break;
                case SW2_DET_1:
                    if (ECE353_Events.sw2) {
                        current_state = SW2_DET_2;
                    } else {
                        current_state = INIT;
                    }
                    break;
                case SW2_DET_2:
                    if (ECE353_Events.sw3) {
                        current_state = SW3_DET;
                    } else {
                        current_state = INIT;
                    }
                    break;
                case SW3_DET:
                    current_state = INIT;
                    break;
                default:
                    printf("ICE03: Unknown State!\n\r");
                    current_state = INIT;
                    break;
            }

            // CONSUME THE SW EVENTS HERE, whichever it may have been activated
            ECE353_Events.sw1 = 0; ECE353_Events.sw2 = 0; ECE353_Events.sw3 = 0;

            /// leds setting logic ///
            switch (current_state) {
                case INIT:
                    leds_set_state(LED_RED, LED_STATE_ON);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_OFF);
                    break;
                case SW1_DET:
                    leds_set_state(LED_RED, LED_STATE_ON);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_ON);
                    break;
                case SW2_DET_1:
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_ON);
                    break;
                case SW2_DET_2:
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_ON);
                    leds_set_state(LED_BLUE, LED_STATE_ON);
                    break;
                case SW3_DET:
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_ON);
                    leds_set_state(LED_BLUE, LED_STATE_OFF);
                    break;
                default:
                    break;
            }
        }
    }
}
#endif
