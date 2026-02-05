/**
 * @file leds.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
// Green and red LEDs live on the same GPIO_PRT_9_OUT register; blue lives on GPIO_PRT_8_OUT
#define REG_OUT_LED_GREEN (*(volatile uint32_t *)0x40310480)
#define REG_OUT_LED_RED   (*(volatile uint32_t *)0x40310480)
#define REG_OUT_LED_BLUE  (*(volatile uint32_t *)0x40310400)

#include "leds.h"

/*****************************************************************************/
/* Functions                                                                   */
/*****************************************************************************/

// Initialize the LEDs to low
cy_rslt_t leds_init_gpio(void) {
    if (cyhal_gpio_init(PIN_LED_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0) != CY_RSLT_SUCCESS) {
        printf("Error initializing RED LED\n\r");
        CY_ASSERT(0);
    }

    if (cyhal_gpio_init(PIN_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0) != CY_RSLT_SUCCESS) {
        printf("Error initializing GREEN LED\n\r");
        CY_ASSERT(0);
    }

    if (cyhal_gpio_init(PIN_LED_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0) != CY_RSLT_SUCCESS) {
        printf("Error initializing BLUE LED\n\r");
        CY_ASSERT(0);
    }

    return CY_RSLT_SUCCESS;
}

// direct access and manipulation of led pins optimizes for speed
void leds_set_state(ece353_led_t led, ece353_led_state_t state) {
    switch (led) {
        case LED_RED:
            if (state == LED_STATE_ON) {
                REG_OUT_LED_RED |= MASK_LED_RED; // set bit
            }
            else {
                REG_OUT_LED_RED &= ~MASK_LED_RED; // clear bit
            }
            break; // otherwise will fall through no matter what
        case LED_GREEN:
            if (state == LED_STATE_ON) {
                GPIO_PRT9->OUT_SET = MASK_LED_GREEN; // this also works; sets bit three of the out register
            }
            else {
                GPIO_PRT9->OUT_CLR = MASK_LED_GREEN; // clear bit
            }
            break; 
        case LED_BLUE:
            if (state == LED_STATE_ON) {
                REG_OUT_LED_BLUE |= MASK_LED_BLUE; // set bit
            }
            else {
                REG_OUT_LED_BLUE &= ~MASK_LED_BLUE; // clear bit
            }
            break;
    }
}

// initialize the IO pins for red, green and blue LEDs to be controlled by
// a PWM peripheral (this function also initializes these pwm peripherals)
cy_rslt_t leds_init_pwm(cyhal_pwm_t *pwm_obj_red, cyhal_pwm_t *pwm_obj_green, cyhal_pwm_t *pwm_obj_blue) {
    cy_rslt_t rslt;

    /* red */
    if ((rslt = cyhal_pwm_init(pwm_obj_red, PIN_LED_RED, NULL) != CY_RSLT_SUCCESS)) {
        printf("Error initializing LED red to PWM peripheral.");
        CY_ASSERT(0);
    }

    /* green */
    if ((rslt = cyhal_pwm_init(pwm_obj_green, PIN_LED_GREEN, NULL) != CY_RSLT_SUCCESS)) {
        printf("Error initializing LED green to PWM peripheral.");
        CY_ASSERT(0);
    }

    /* blue */
    if ((rslt = cyhal_pwm_init(pwm_obj_blue, PIN_LED_BLUE, NULL) != CY_RSLT_SUCCESS)) {
        printf("Error initializing LED blue to PWM peripheral.");
        CY_ASSERT(0);
    }

    return CY_RSLT_SUCCESS;
}

// keep track of how many times we've pressed each button
static uint8_t button_count[3] = {0, 0, 0};

// function to increment the intensity of an LED by 10% each time the corresponding button is
// pressed, thorugh PWM peripheral. if the intensity would > 100%, reset the intensity to 0%
cy_rslt_t leds_set_frequency_pwm (cyhal_pwm_t* pwm_obj_red, cyhal_pwm_t *pwm_obj_green, cyhal_pwm_t *pwm_obj_blue) {
    cy_rslt_t rslt;

    if (ECE353_Events.sw1) {
        // consume the event
        ECE353_Events.sw1 = 0;

        button_count[0]++;

        if (button_count[0] < 11) {
            // 1KHz so no visible flickering; duty cycle starts at 0
            if ((rslt = cyhal_pwm_set_duty_cycle(pwm_obj_red, button_count[0]*10, 1000)!= CY_RSLT_SUCCESS)) {
                printf("Error setting pwm duty cycle for LED red.");
                CY_ASSERT(0);
            }
        } else {
            button_count[0] = 0;
            if ((rslt = cyhal_pwm_set_duty_cycle(pwm_obj_red, button_count[0], 1000)!= CY_RSLT_SUCCESS)) {
                printf("Error setting pwm duty cycle for LED red.");
                CY_ASSERT(0);
            }
        }

        printf("Current intensity (RED): %d%%\n\r", button_count[0]*10);
    }

    // do the same for green and blue
    if (ECE353_Events.sw2) {
        // consume the event
        ECE353_Events.sw2 = 0;

        button_count[1]++;
        
        if (button_count[1] < 11) {
            // 1KHz so no visible flickering; duty cycle starts at 0
            if ((rslt = cyhal_pwm_set_duty_cycle(pwm_obj_green, button_count[1]*10, 1000)!= CY_RSLT_SUCCESS)) {
                printf("Error setting pwm duty cycle for LED green.");
                CY_ASSERT(0);
            }
        } else {
            button_count[1] = 0;
            if ((rslt = cyhal_pwm_set_duty_cycle(pwm_obj_green, button_count[1], 1000)!= CY_RSLT_SUCCESS)) {
                printf("Error setting pwm duty cycle for LED green.");
                CY_ASSERT(0);
            }
        }

        printf("Current intensity (GREEN): %d%%\n\r", button_count[1]*10);
    }

    if (ECE353_Events.sw3) {
        // consume the event
        ECE353_Events.sw3 = 0;

        button_count[2]++;
        
        if (button_count[2] < 11) {
            // 1KHz so no visible flickering; duty cycle starts at 0
            if ((rslt = cyhal_pwm_set_duty_cycle(pwm_obj_blue, button_count[2]*10, 1000)!= CY_RSLT_SUCCESS)) {
                printf("Error setting pwm duty cycle for LED blue.");
                CY_ASSERT(0);
            }
        } else {
            button_count[2] = 0;
            if ((rslt = cyhal_pwm_set_duty_cycle(pwm_obj_blue, button_count[2], 1000)!= CY_RSLT_SUCCESS)) {
                printf("Error setting pwm duty cycle for LED blue.");
                CY_ASSERT(0);
            }
        }
        
        printf("Current intensity (BLUE): %d%%\n\r", button_count[2]*10);
    }

    return CY_RSLT_SUCCESS;
}

