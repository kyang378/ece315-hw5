/**
 * @file ice04.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE04)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 04 - PWM Buzzer";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_pwm_t pwm_led_red;
cyhal_pwm_t pwm_led_green;
cyhal_pwm_t pwm_led_blue;

//Store current duty cycle for each LED
uint8_t duty_cycle_red = 0;
uint8_t duty_cycle_green = 0;
uint8_t duty_cycle_blue = 0;
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
    
    /* ADD CODE */

    rslt = leds_init_pwm(&pwm_led_red, &pwm_led_green, &pwm_led_blue);

    if (rslt != CY_RSLT_SUCCESS) {
        printf("Error initializing PWM for RGB LED: %d\n", rslt);
    }
    // //enable buttons as inputs
     buttons_init();  //NOTE buttons_init calls buttons_init_timer internally
    // //initialize button debounce timer
     buttons_init_timer();
    

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

    //Initialize duty cycles to 0
    cyhal_pwm_set_duty_cycle(&pwm_led_red,   duty_cycle_red, 1000);
    cyhal_pwm_set_duty_cycle(&pwm_led_green, duty_cycle_green, 1000);
    cyhal_pwm_set_duty_cycle(&pwm_led_blue,  duty_cycle_blue, 1000);
    
    while(1)
    {
        /* ADD CODE */
         if( ECE353_Events.sw1 || ECE353_Events.sw2 || ECE353_Events.sw3) {
            if(ECE353_Events.sw1) {
                ECE353_Events.sw1 = 0;
                if(duty_cycle_red >= 100) {
                    duty_cycle_red = 0;
                } else {
                    duty_cycle_red += 10;
                }
                cyhal_pwm_set_duty_cycle(&pwm_led_red, duty_cycle_red, 1000);
                printf("SW1 Pressed: Red Duty Cycle = %d%%\n", duty_cycle_red);
            }
            if(ECE353_Events.sw2) {
                ECE353_Events.sw2 = 0;
                if(duty_cycle_green >= 100) {
                    duty_cycle_green = 0;
                } else {
                    duty_cycle_green += 10;
                }
                cyhal_pwm_set_duty_cycle(&pwm_led_green,   duty_cycle_green, 1000);
                printf("SW2 Pressed: Green Duty Cycle = %d%%\n", duty_cycle_green);
            }
            if(ECE353_Events.sw3) {
                ECE353_Events.sw3 = 0;
                if(duty_cycle_blue >= 100) {
                    duty_cycle_blue = 0;
                } else {
                    duty_cycle_blue += 10;
                }
                cyhal_pwm_set_duty_cycle(&pwm_led_blue,   duty_cycle_blue, 1000);
                printf("SW3 Pressed: Blue Duty Cycle = %d%%\n", duty_cycle_blue);
            }
         }

        /* END ADD CODE */
    }
}
#endif
