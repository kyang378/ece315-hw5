/**
 * @file buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "buttons.h"

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

// Where to find the addresses of the in registers for the buttons?

 int previous_state_SW1 = 1, previous_state_SW2 = 1, previous_state_SW3 = 1; // active low buttons; initialized to deasserted state

 cy_rslt_t buttons_init_gpio(void) {
    // to intialize the buttons is to initialize the GPIO pins as inputs
    cy_rslt_t rslt;
    if (rslt = cyhal_gpio_init(PIN_BUTTON_SW1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1) != CY_RSLT_SUCCESS) { // initially deasserted (1)
        printf("Error initializing SW1\n\r");
        CY_ASSERT(0);
    }

    if (rslt = cyhal_gpio_init(PIN_BUTTON_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1) != CY_RSLT_SUCCESS) {
        printf("Error initializing SW2\n\r");
        CY_ASSERT(0);
    }

    if (rslt = cyhal_gpio_init(PIN_BUTTON_SW3, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1) != CY_RSLT_SUCCESS) {
        printf("Error initializing SW3\n\r");
        CY_ASSERT(0);
    }

    return CY_RSLT_SUCCESS;
}

button_state_t buttons_get_state(ece353_button_t button) {
    int button_val; // current button logic level
    int previous_state_to_be; // the previous state to be updated

    // we don't consider rising or falling edge as a previous state
    // but we should output the state as such when the conditions are met

    // NOTE: test out the board; could be wrong
    if (button == BUTTON_SW1) {
        previous_state_to_be = (button_val = cyhal_gpio_read(PIN_BUTTON_SW1));
        if (previous_state_SW1 == 0) {
            if (button_val == 0) {
                // no need to update previous state
                return BUTTON_STATE_LOW; // 0 -> 0, still low
            }
            else {
                previous_state_SW1 = button_val;
                return BUTTON_STATE_RISING_EDGE; // 0 -> 1
            }
        }
        else if (button_val == 1) {
            // no need to update previous state
            return BUTTON_STATE_HIGH; // 1 -> 1
        } else {
            previous_state_SW1 = button_val;
            return BUTTON_STATE_FALLING_EDGE; // 1 -> 0
        }
    }

    if (button == BUTTON_SW2) {
        previous_state_to_be = (button_val = cyhal_gpio_read(PIN_BUTTON_SW2));
        if (previous_state_SW2 == 0) {
            if (button_val == 0) {
                return BUTTON_STATE_LOW; // 0 -> 0
            }
            else {
                previous_state_SW2 = button_val;
                return BUTTON_STATE_RISING_EDGE; // 0 -> 1
            }
        } 
        else if (button_val == 1) {
            return BUTTON_STATE_HIGH; // 1 -> 1
        }
        else {
            previous_state_SW2 = button_val;
            return BUTTON_STATE_FALLING_EDGE; // 1 -> 0
        }
    }

    if (button == BUTTON_SW3) {
        previous_state_to_be = (button_val = cyhal_gpio_read(PIN_BUTTON_SW3));
        if (previous_state_SW3 == 0) {
            if (button_val == 0) {
                return BUTTON_STATE_LOW; // 0 -> 0
            }
            else {
                previous_state_SW3 = button_val;
                return BUTTON_STATE_RISING_EDGE; // 0 -> 1
            }
        } 
        else if (button_val == 1) {
            return BUTTON_STATE_HIGH; // 1 -> 1
        }
        else {
            previous_state_SW3 = button_val;
            return BUTTON_STATE_FALLING_EDGE; // 1 -> 0
        }
    }
}