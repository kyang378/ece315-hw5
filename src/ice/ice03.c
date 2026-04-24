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
#include "leds.h"
#include "buttons.h"
#include "ece353-events.h"
#include "ece353-pins.h"
#include "timer.h"


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
void app_init_hw(void) {
    cy_rslt_t rslt;


    //CONSOLE INIT COMES FIRST
    console_init();
    printf("Initializing ICE03 Hardware...\n");

    // //enable RGB LED pins as outputs
     leds_init();
    // //enable buttons as inputs
     buttons_init();  //NOTE buttons_init calls buttons_init_timer internally
    // //initialize button debounce timer
     buttons_init_timer();

    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void) {
    printf("Starting ICE03 Main Application...\n");
    typedef enum{
        STATE_INIT = 0,
        STATE_SW1_DET = 1,
        STATE_SW2_DET_1 = 2,
        STATE_SW2_DET_2 = 3,
        STATE_SW3_DET = 4
    } ice03_state_t;

    //initialize FSM
    ice03_state_t current_state = STATE_INIT;
    printf("Entering while loop");

    //initialize LEDs to match initial state
    leds_set_state(LED_RED, LED_STATE_ON); 
    leds_set_state(LED_GREEN, LED_STATE_OFF);
    leds_set_state(LED_BLUE, LED_STATE_OFF); 
    
    while(1) {
        if( ECE353_Events.sw1 || ECE353_Events.sw2 || ECE353_Events.sw3) {
            switch(current_state) {
                // Code to implement the FSM
                case STATE_INIT: //SW1 sends to next state, else stays
                    if (ECE353_Events.sw1) {
                        ECE353_Events.sw1 = 0;
                        current_state = STATE_SW1_DET;
                    } else if (ECE353_Events.sw2) {
                        ECE353_Events.sw2 = 0;
                        current_state = STATE_INIT;
                    } else if (ECE353_Events.sw3) {
                        ECE353_Events.sw3 = 0;
                        current_state = STATE_INIT;
                    }
                    break;
                case STATE_SW1_DET: //SW2 sends to next state, SW1 and SW3 back to init
                    if (ECE353_Events.sw2) {
                        ECE353_Events.sw2 = 0;
                        current_state = STATE_SW2_DET_1;
                    } else if (ECE353_Events.sw1) {
                        ECE353_Events.sw1 = 0;
                        current_state = STATE_INIT;
                    } else if (ECE353_Events.sw3) {
                        ECE353_Events.sw3 = 0;
                        current_state = STATE_INIT;
                    }
                    break;
                case STATE_SW2_DET_1: //SW2 sends to next state, SW1 and SW3 back to init
                    if (ECE353_Events.sw2) {
                        ECE353_Events.sw2 = 0;
                        current_state = STATE_SW2_DET_2;
                    } else if (ECE353_Events.sw1) {
                        ECE353_Events.sw1 = 0;
                        current_state = STATE_INIT;
                    } else if (ECE353_Events.sw3) {
                        ECE353_Events.sw3 = 0;
                        current_state = STATE_INIT;
                    }
                    break;
                case STATE_SW2_DET_2: //SW3 sends to next state, SW1 and SW2 back to init
                    if (ECE353_Events.sw3) {
                        ECE353_Events.sw3 = 0;
                        current_state = STATE_SW3_DET;
                    } else if (ECE353_Events.sw1) {
                        ECE353_Events.sw1 = 0;
                        current_state = STATE_INIT;
                    } else if (ECE353_Events.sw2) {
                        ECE353_Events.sw2 = 0;
                        current_state = STATE_INIT;
                    }
                    break;
                case STATE_SW3_DET: //Any button sends back to init
                    if (ECE353_Events.sw1) {
                        ECE353_Events.sw1 = 0;
                        current_state = STATE_INIT;
                    } else if (ECE353_Events.sw2) {
                        ECE353_Events.sw2 = 0;
                        current_state = STATE_INIT;
                    } else if (ECE353_Events.sw3) {
                        ECE353_Events.sw3 = 0;
                        current_state = STATE_INIT;
                    }
                    break;
                default:
                    printf("ICE03: Unknown State!\n");
                    current_state = STATE_INIT;
            }


            /* Update the LEDs based on the current state */
            switch(current_state) {
                // Code that determines which LEDs should be on/off
                case STATE_INIT:
                    leds_set_state(LED_RED, LED_STATE_ON);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_OFF);
                    break;
                case STATE_SW1_DET:
                    leds_set_state(LED_RED, LED_STATE_ON);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_ON);
                    break;
                case STATE_SW2_DET_1:
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_ON);
                    break;
                case STATE_SW2_DET_2:
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_ON);
                    leds_set_state(LED_BLUE, LED_STATE_ON);
                    break;
                case STATE_SW3_DET:
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_ON);
                    leds_set_state(LED_BLUE, LED_STATE_OFF);
                    break;
                default:
                    printf("ICE03: Unknown State when updating LEDs!\n");
                    leds_set_state(LED_RED, LED_STATE_OFF);
                    leds_set_state(LED_GREEN, LED_STATE_OFF);
                    leds_set_state(LED_BLUE, LED_STATE_OFF);
                    break;
            }
        }
    }

}
#endif
