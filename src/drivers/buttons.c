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

 // Static storage for previous button state (initialized to HIGH)
 static uint8_t prev_logic_level[3] = {1,1,1};

 // Static storage for timer and GPIO objects
 static cyhal_gpio_t button_pins[3];
 static cyhal_timer_t button_timer_obj;

 /**
  * @brief Initialize GPIO pins for buttons
  * @return cy_rslt_t Status of initialization
  */
 cy_rslt_t buttons_init_gpio(void) {
    cy_rslt_t rslt = CY_RSLT_SUCCESS;
    
    uint32_t gpio_pins[3] = {PIN_BUTTON_SW1, PIN_BUTTON_SW2, PIN_BUTTON_SW3};
    
    for (int i = 0; i < 3; i++) {
        rslt = cyhal_gpio_init(gpio_pins[i], CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
        if (rslt != CY_RSLT_SUCCESS) {
            return rslt;
        }
    }
    
    return rslt;
 }


 cy_rslt_t buttons_init_timer(void) {
    cy_rslt_t rslt = CY_RSLT_SUCCESS;
    
    return rslt;
 }

 void buttons_init(void) {
    buttons_init_gpio();
    buttons_init_timer();
 }

button_state_t buttons_get_state(ece353_button_t button) {
    button_state_t state;

    //read current logic level
    uint32_t gpio_pins[3] = {PIN_BUTTON_SW1, PIN_BUTTON_SW2, PIN_BUTTON_SW3};
    uint8_t current_logic_level = cyhal_gpio_read(gpio_pins[button]);


    
    // Determine state based on current vs previous logic levels
    // 00 (0) = 0->0 (LOW)
    // 01 (1) = 0->1 (RISING_EDGE)
    // 10 (2) = 1->0 (FALLING_EDGE)
    // 11 (3) = 1->1 (HIGH)
    if (prev_logic_level[button] == 1 && current_logic_level == 1) {
        state = BUTTON_STATE_HIGH;
    } else if (prev_logic_level[button] == 1 && current_logic_level == 0) {
        state = BUTTON_STATE_FALLING_EDGE;
    } else if (prev_logic_level[button] == 0 && current_logic_level == 1) {
        state = BUTTON_STATE_RISING_EDGE;
    } else {
        state = BUTTON_STATE_LOW;
    }

    // Update previous logic level
    prev_logic_level[button] = current_logic_level;
    
    return state;
 }