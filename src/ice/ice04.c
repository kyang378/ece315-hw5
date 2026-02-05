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
cyhal_pwm_t pwm_obj_red, pwm_obj_green, pwm_obj_blue;

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
    
    // initialize the buttons and button timer
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

    // initialize the PWM peripherals used to control the LEDs
    if ((rslt = leds_init_pwm(&pwm_obj_red, &pwm_obj_green, &pwm_obj_blue))!= CY_RSLT_SUCCESS){
        printf("Error initializing LEDs through PWM peripheral\n\r");
        CY_ASSERT(0);
    }

    // start the pwm outputs for these LEDs
    if ((rslt = cyhal_pwm_start(&pwm_obj_red))!= CY_RSLT_SUCCESS) {
        printf("Error starting the PWM output for LED red.");
        CY_ASSERT(0);
    }

    if ((rslt = cyhal_pwm_start(&pwm_obj_green))!= CY_RSLT_SUCCESS) {
        printf("Error starting the PWM output for LED green.");
        CY_ASSERT(0);
    }

    if ((rslt = cyhal_pwm_start(&pwm_obj_blue))!= CY_RSLT_SUCCESS) {
        printf("Error starting the PWM output for LED blue.");
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
    while(1)
    {
        leds_set_frequency_pwm(&pwm_obj_red, &pwm_obj_green, &pwm_obj_blue);
    }
}
#endif
