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


// function that initializes the timer; the handlers are in some other file
cy_rslt_t timer_init(cyhal_timer_t *timer_obj, cyhal_timer_cfg_t *timer_cfg, uint32_t ticks, void *Handler)
{
    cy_rslt_t rslt;

    timer_cfg->period = ticks; // set the timer period
    timer_cfg->direction = CYHAL_TIMER_DIR_UP; // set the timer to count up
    timer_cfg->is_compare = false; // disable compare mode
    timer_cfg->is_continuous = true; // set the timer to run continuously, i.e. run it
    timer_cfg->value = 0; // initialize the timer value to 0

    if ((rslt = cyhal_timer_init(timer_obj, NC, NULL)) != CY_RSLT_SUCCESS)
    {
        return rslt;
    }
    
    if ((rslt = cyhal_timer_configure(timer_obj, timer_cfg)) != CY_RSLT_SUCCESS) 
    {
        return rslt;
    }

    if ((rslt = cyhal_timer_set_frequency(timer_obj, 100000000)) != CY_RSLT_SUCCESS) 
    {
        return rslt;
    }

    // register callback (what to do when timer interrupt occurs)
    cyhal_timer_register_callback(timer_obj, Handler, NULL);

    // set the event to which timer interrupt occurs and enable it. 
    
    // in this case the event is the terminal count (i.e. interrupt occurs when terminal(period/tick) count is reached)
    cyhal_timer_enable_event(timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true);

    // start the timer with the configured settings
    if ((rslt = cyhal_timer_start(timer_obj) != CY_RSLT_SUCCESS)) 
    {
        return rslt;
    }


    return rslt; // Return the result of the initialization
}