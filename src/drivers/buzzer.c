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
#include "ece353-pins.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cyhal_timer.h"


#define BUZZER_TIMER_FREQ 100000000u  // 100 MHz timer clock

// Internal timer objects
static cyhal_timer_t buzzer_timer;
static cyhal_timer_cfg_t buzzer_cfg;

/**
 * @brief Internal interrupt handler for the buzzer timer.
 *        Toggles the buzzer GPIO pin.
 */
static void buzzer_handler(void *handler_arg, cyhal_timer_event_t event)
{
    (void)handler_arg;
    (void)event;

    PORT_BUZZER->OUT ^= MASK_BUZZER;
}

/**
 * @brief Initialize the buzzer hardware and start tone generation.
 *
 * @param frequency  Desired buzzer frequency in Hz.
 *                   If frequency == 0, the buzzer is turned off.
 *
 * @return cy_rslt_t CY_RSLT_SUCCESS on success, error code otherwise.
 */
cy_rslt_t buzzer_init(uint32_t frequency)
{
    cy_rslt_t rslt;

    // -------------------------------------------------------
    // 1. Initialize buzzer GPIO
    // -------------------------------------------------------
   // Cy_GPIO_Clr(PORT_BUZZER, PIN_BUZZER);  // drive low at the port level

    rslt = cyhal_gpio_init(PIN_BUZZER,
                           CYHAL_GPIO_DIR_OUTPUT,
                           CYHAL_GPIO_DRIVE_STRONG,
                           false);
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt;
    }

    // -------------------------------------------------------
    // 2. Initialize timer
    // -------------------------------------------------------
    rslt = cyhal_timer_init(&buzzer_timer, NC, NULL);
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt;
    }

    // -------------------------------------------------------
    // 3. Set timer clock to 100 MHz (CRITICAL FIX)
    // -------------------------------------------------------
    rslt = cyhal_timer_set_frequency(&buzzer_timer, BUZZER_TIMER_FREQ);
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt;
    }

    // -------------------------------------------------------
    // 4. Compute tick count
    // -------------------------------------------------------
    uint32_t ticks = 0;

    if (frequency > 0) {
        // Tick count = (TimerFreq / Frequency) / 2
        ticks = (BUZZER_TIMER_FREQ / frequency) / 2;
    }

    // -------------------------------------------------------
    // 5. Configure timer
    // -------------------------------------------------------
    buzzer_cfg.compare_value = 0;
    buzzer_cfg.period        = ticks;
    buzzer_cfg.direction     = CYHAL_TIMER_DIR_UP;
    buzzer_cfg.is_compare    = false;
    buzzer_cfg.is_continuous = true;

    rslt = cyhal_timer_configure(&buzzer_timer, &buzzer_cfg);
    if (rslt != CY_RSLT_SUCCESS) {
        return rslt;
    }

    // -------------------------------------------------------
    // 6. Register interrupt handler
    // -------------------------------------------------------
    cyhal_timer_register_callback(&buzzer_timer, buzzer_handler, NULL);
    cyhal_timer_enable_event(&buzzer_timer,
                             CYHAL_TIMER_IRQ_TERMINAL_COUNT,
                             CYHAL_ISR_PRIORITY_DEFAULT,
                             true);

    // -------------------------------------------------------
    // 7. Stop timer initially (buzzer is off by default)
    // -------------------------------------------------------
        cyhal_timer_stop(&buzzer_timer);


    return rslt;
}

void buzzer_on(void)
{
    cyhal_timer_start(&buzzer_timer);
}

void buzzer_off(void)
{
    cyhal_timer_stop(&buzzer_timer);
    cyhal_gpio_write(PIN_BUZZER, 0);
}