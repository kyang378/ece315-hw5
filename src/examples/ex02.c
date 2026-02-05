/**
 * @file ex02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX02)

#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 02 - IO for LCD";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

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
    cy_rslt_t rslt;

    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");
    if (rslt =lcd_initialize() != CY_RSLT_SUCCESS)
    {
        printf("LCD Initialization failed with error: %d\n", rslt);
        CY_ASSERT(0); // halts the processor (also stops debugger)
    }
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
    // fill a white rectangle centered on the screen that's 50 pixels wide and 30 pixels tall
    lcd_draw_rectangle(160, 120, 50, 30, LCD_COLOR_WHITE, true); 

    // main app should have an infinite loop and never return, which will cause a system crash (fault) to occur.
    while (1) 
    {
    }
}
#endif