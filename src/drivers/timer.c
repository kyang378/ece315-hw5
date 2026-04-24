/**
 * @file timer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-08-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "timer.h"
#include <complex.h>

cy_rslt_t timer_init(cyhal_timer_t *timer_obj, cyhal_timer_cfg_t *timer_cfg, uint32_t ticks, void *Handler)
{
    cy_rslt_t rslt;

    // Initialize the timer object
    timer_cfg->period = ticks; // Set the timer period to the specified ticks
    timer_cfg->direction = CYHAL_TIMER_DIR_UP; // Set the timer to count up
    timer_cfg->is_compare = false; // Set the timer to capture mode
    //timer_cfg->compare_value = 0; // No compare value
    timer_cfg->is_continuous = true; // Set the timer to continuous mode
    timer_cfg->value = 0; // Initialize the timer value to 0

    rslt = cyhal_timer_init(timer_obj, NC, NULL); // Initialize the timer hardware
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt; // Return if initialization failed
    }

    rslt = cyhal_timer_configure(timer_obj, timer_cfg); // Configure the timer with the specified settings
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt; // Return if configuration failed
    }

    rslt = cyhal_timer_set_frequency(timer_obj, 100000000); // Set the timer frequency to 100 MHz
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt; // Return if setting frequency failed
    }

    cyhal_timer_register_callback(timer_obj, Handler, NULL); // Register the interrupt handler

    cyhal_timer_enable_event(timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true); // Enable terminal count interrupt

    rslt = cyhal_timer_start(timer_obj); // Start the timer
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt; // Return if starting the timer failed
    }

    return rslt; // Return the result of the initialization
}