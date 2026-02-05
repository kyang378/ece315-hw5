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

 #include "leds.h"

 //define function to initialize the LEDs
 void leds_init(void) {
    leds_init_gpio();
}

cy_rslt_t leds_init_gpio(void) {
    cy_rslt_t rslt = CY_RSLT_SUCCESS;
    
    uint32_t led_pins[3] = {PIN_LED_GREEN, PIN_LED_RED, PIN_LED_BLUE};
    
    for (int i = 0; i < 3; i++) {
        rslt = cyhal_gpio_init(led_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
        if (rslt != CY_RSLT_SUCCESS) {
            return rslt;
        }
    }
    
    return rslt;
}

cy_rslt_t leds_init_pwm(cyhal_pwm_t *pwm_obj_red, cyhal_pwm_t *pwm_obj_green, cyhal_pwm_t *pwm_obj_blue) {
    cy_rslt_t rslt = CY_RSLT_SUCCESS;

    //Initialize PWM LED pins
    rslt = cyhal_pwm_init(pwm_obj_red, PIN_LED_RED, NULL); //If error, try changing to &pwm_obj_red
    if (rslt != CY_RSLT_SUCCESS) {
        printf("Error initializing RED PWM: %d\n", rslt);
        return rslt;
    }
    
    rslt = cyhal_pwm_init(pwm_obj_green, PIN_LED_GREEN, NULL);
    if (rslt != CY_RSLT_SUCCESS) {
        printf("Error initializing GREEN PWM: %d\n", rslt);
    }

    rslt = cyhal_pwm_init(pwm_obj_blue, PIN_LED_BLUE, NULL);
    if (rslt != CY_RSLT_SUCCESS) {
        printf("Error initializing BLUE PWM: %d\n", rslt);
    }
    
    // Set frequency and duty cycle
    cyhal_pwm_set_duty_cycle(pwm_obj_red,   50.0f, 1000);   // 1 kHz, 50%
    cyhal_pwm_set_duty_cycle(pwm_obj_green, 50.0f, 1000);
    cyhal_pwm_set_duty_cycle(pwm_obj_blue,  50.0f, 1000);

    // Start PWM
    cyhal_pwm_start(pwm_obj_red);
    cyhal_pwm_start(pwm_obj_blue);
    cyhal_pwm_start(pwm_obj_green);



    return rslt;
}


//define function to set the state of a specific LED
void leds_set_state(ece353_led_t led, ece353_led_state_t state) {
    switch (led) {
        case LED_GREEN:
            cyhal_gpio_write(PIN_LED_GREEN, (state == LED_STATE_ON) ? 1 : 0);
            break;
        case LED_RED:
            cyhal_gpio_write(PIN_LED_RED, (state == LED_STATE_ON) ? 1 : 0);
            break;
        case LED_BLUE:
            cyhal_gpio_write(PIN_LED_BLUE, (state == LED_STATE_ON) ? 1 : 0);
            break;
        default:
            // Invalid LED, do nothing
            break;
    }
}