/**
 * @file buzzer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "buzzer.h"

cyhal_pwm_t buzzer_pwm;

cy_rslt_t buzzer_init(float duty_cycle, uint32_t frequency)
{
    cy_rslt_t rslt;

    // Initialize the buzzer pwm 
    rslt = cyhal_pwm_init(&buzzer_pwm, PIN_BUZZER, NULL);
    if (rslt != CY_RSLT_SUCCESS)    {
        printf("Failed to initialize buzzer PWM\n\r");
        CY_ASSERT(0);
    }

    // set the period and duty cycle of the PWM signal; 1000000/frequency converts frequency in Hz to period in microseconds
    rslt = cyhal_pwm_set_period(&buzzer_pwm, 1000000/frequency, duty_cycle);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to set buzzer PWM period and duty cycle\n\r");
        CY_ASSERT(0);
    }
    return rslt;
}

void buzzer_on(void)
{
    cy_rslt_t rslt;

    rslt = cyhal_pwm_start(&buzzer_pwm);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start buzzer PWM\n\r");
        CY_ASSERT(0);
    }
}

void buzzer_off(void)
{
    cy_rslt_t rslt;

    rslt = cyhal_pwm_stop(&buzzer_pwm);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to stop buzzer PWM\n\r");
        CY_ASSERT(0);
    }
}
