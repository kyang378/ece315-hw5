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

cy_rslt_t leds_init_gpio(void) { // Initialize the LEDs to low
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
