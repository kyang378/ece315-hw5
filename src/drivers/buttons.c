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

/*****************************************************************************/
/* Functions                                                                    */
/*****************************************************************************/

 int previous_state_SW1 = 1, previous_state_SW2 = 1, previous_state_SW3 = 1; // active low buttons; initialized to deasserted state

 cy_rslt_t buttons_init_gpio(void) {
    // to intialize the buttons is to initialize the GPIO pins as inputs
    cy_rslt_t rslt;
    if ((rslt = cyhal_gpio_init(PIN_BUTTON_SW1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0)) != CY_RSLT_SUCCESS) { // initially deasserted (1)
        printf("Error initializing SW1\n\r");
        CY_ASSERT(0);
    }

    if ((rslt = cyhal_gpio_init(PIN_BUTTON_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0)) != CY_RSLT_SUCCESS) {
        printf("Error initializing SW2\n\r");
        CY_ASSERT(0);
    }

    if ((rslt = cyhal_gpio_init(PIN_BUTTON_SW3, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0)) != CY_RSLT_SUCCESS) {
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
    return BUTTON_STATE_HIGH; // default return value
}


// Timer object and configuration for button debouncing; these and the function below are  kept static to limit scope to this file
static cyhal_timer_t button_timer;
static cyhal_timer_cfg_t button_timer_cfg;

// Timer interrupt handler for button debouncing
static void button_timer_handler(void* args, cyhal_timer_event_t event)
{
    // static within a function itself acts like a global variable but limits scope to this function, so we can keep track of counts for each button which is tracked across multiple calls to this function. the initialization of the static variable only happens once.
    static uint8_t button_count[3] = {0, 0, 0}; // counters for SW1, SW2, SW3

    // get the current state of each button
    uint8_t SW1 = PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1;
    uint8_t SW2 = PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2;
    uint8_t SW3 = PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3;

    // check past states up to 4 interrupts ago and update counts
    if (SW1 == 0) 
    {
        button_count[0]++;
        if (button_count[0] == 5) 
        { // 5 consecutive reads of pressed state
            ECE353_Events.sw1 = 1; // set event flag for SW1
        }
    } else 
    {
        button_count[0] = 0; // if button is not in pressed state, reset count (since we need consecutive reads)
        ECE353_Events.sw1 = 0; // clears button pressed event
    }

    if (SW2 == 0) 
    {
        button_count[1]++;
        if (button_count[1] == 5) 
        {
            ECE353_Events.sw2 = 1;
        }
    } else 
    {
        button_count[1] = 0;
        ECE353_Events.sw2 = 0;
    }

    if (SW3 == 0) 
    {
        button_count[2]++;
        if (button_count[2] == 5) 
        {
            ECE353_Events.sw3 = 1;
        }
    } else 
    {
        button_count[2] = 0;
        ECE353_Events.sw3 = 0;
    }

}

// Function to initialize the timer for button debouncing
cy_rslt_t buttons_init_timer()
{
    // we assume buttons will stop bouncing within 25 milliseconds
    // and configure a timer to generate interrupts every 5 ms
    // so when we see five consecutive falling edges, we can confirm the button press

    // to find the ticks value for 5 ms at 100 MHz:
    // each tick is 1/100M = 1e-8 seconds
    // so for 5 ms, we need 5e-3 / 1e-8 = 500000 ticks
    return timer_init(&button_timer, &button_timer_cfg, 500000, button_timer_handler);
}
