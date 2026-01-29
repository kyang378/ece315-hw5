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
    //initialize buttons
    buttons_init();

    //initialize LEDs
    leds_init();

    console_init();
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
void app_main(void)
{

    while(1)
    {

        /* Sleep for 50mS */
        cyhal_system_delay_ms(50);

        //detect and print button states
        //SW1
        //get state of button 1
        button_state_t SW1_State = buttons_get_state(BUTTON_SW1);
        if (SW1_State == BUTTON_STATE_FALLING_EDGE) {
            printf("SW1 Pressed\n\r");
            //turn on Red LED
            leds_set_state(LED_RED, LED_STATE_ON);
        }
        if (SW1_State  == BUTTON_STATE_RISING_EDGE) {
            printf("SW1 Released\n\r");
            //turn off Red LED
            leds_set_state(LED_RED, LED_STATE_OFF);
        }
        //SW2
        button_state_t SW2_State = buttons_get_state(BUTTON_SW2);
        if (SW2_State == BUTTON_STATE_FALLING_EDGE) {
            printf("SW2 Pressed\n\r");
            //turn on Green LED
            leds_set_state(LED_GREEN, LED_STATE_ON);
        }
        if (SW2_State  == BUTTON_STATE_RISING_EDGE) {
            printf("SW2 Released\n\r");
            //turn off Green LED
            leds_set_state(LED_GREEN, LED_STATE_OFF);
        }
        //SW3
        button_state_t SW3_State = buttons_get_state(BUTTON_SW3);
        if (SW3_State == BUTTON_STATE_FALLING_EDGE) {
            printf("SW3 Pressed\n\r");
            //turn on Blue LED
            leds_set_state(LED_BLUE, LED_STATE_ON);
        }
        if (SW3_State  == BUTTON_STATE_RISING_EDGE) {
            printf("SW3 Released\n\r");
            //turn off Blue LED
            leds_set_state(LED_BLUE, LED_STATE_OFF);
        }

        
        cyhal_system_delay_ms(100);

    }
}
#endif
