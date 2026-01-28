/**
 * @file ice01.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE01)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 01 - Memory Mapped IO - GPIO";

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
    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    buttons_init_gpio(); // initialize the buttons
    leds_init_gpio(); // initialize the leds
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

    while(1)
    {
        // Read the state of each button
        button_state_t state_SW1 = buttons_get_state(BUTTON_SW1);
        button_state_t state_SW2 = buttons_get_state(BUTTON_SW2);
        button_state_t state_SW3 = buttons_get_state(BUTTON_SW3);

        // note our buttons are active low, so 1 -> 0 (falling edge) means pressed
        if (state_SW1 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW1 has been pressed.\n\r");
            leds_set_state(LED_RED, LED_STATE_ON);
        }

        if (state_SW1 == BUTTON_STATE_RISING_EDGE) {
            printf("SW1 has been released.\n\r");
            leds_set_state(LED_RED, LED_STATE_OFF);
        }

        if (state_SW2 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW2 has been pressed.\n\r");
            leds_set_state(LED_GREEN, LED_STATE_ON);
        }

        if (state_SW2 == BUTTON_STATE_RISING_EDGE) {
            printf("SW2 has been released.\n\r");
            leds_set_state(LED_GREEN, LED_STATE_OFF);
        }

        if (state_SW3 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW3 has been pressed.\n\r");
            leds_set_state(LED_BLUE, LED_STATE_ON);
        }

        if (state_SW3 == BUTTON_STATE_RISING_EDGE) {
            printf("SW3 has been released.\n\r");
            leds_set_state(LED_BLUE, LED_STATE_OFF);
        }

        /* Delay for 100mS */
        cyhal_system_delay_ms(50);

    }
}
#endif
