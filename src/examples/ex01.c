/**
 * @file ex01.c
 * @author Shaw McCoy (sjmccoy@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX01)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 01 - Intro to C";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
#define REG_OUT_LED_GREEN (*(volatile uint32_t *)0x40310480)
/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{

    cy_rslt_t result;

    console_init();
    
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    printf("Initializing User LED...\n\r");
    //Initialize P9.2 as output
    result = cyhal_gpio_init(PIN_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if (result != CY_RSLT_SUCCESS) {
        printf("Failed to initialize user LED\n\r");
        CY_ASSERT(0);
    }

    printf("Starting main application...\n\r");

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    /* Enter Infinite Loop*/
    while (1) {
        //Turn on Green LED
        //cyhal_gpio_write(PIN_LED_GREEN, 1);
        REG_OUT_LED_GREEN |= MASK_LED_GREEN;
        cyhal_system_delay_ms(500);
        //Turn off Green LED
        //cyhal_gpio_write(PIN_LED_GREEN, 0);
        REG_OUT_LED_GREEN &= ~MASK_LED_GREEN;
        cyhal_system_delay_ms(500);
        
    }
}
#endif