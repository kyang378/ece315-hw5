/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX08)
#include "drivers.h"
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_lcd.h"
#include "task_joystick.h"

char APP_DESCRIPTION[] = "ECE353: Example 08 - FreeRTOS LCD Gatekeeper";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t xQueue_Request_LCD = NULL;
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/


void task_sw1(void *pvParameters) {
    uint32_t button_count = 0;
    uint32_t debounce_count = 0;
    lcd_msg_request_t lcd_request;

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(25));

        //check button
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_SW1) == 0) {
            if (debounce_count <= 40) {
                debounce_count++;
            }
        } else {
            debounce_count = 0;
        }

        if (debounce_count == 40) {
            button_count++;

            lcd_request.cmd = LCD_PRINT_SW1_COUNT;
            lcd_request.return_queue = NULL;
            //printf("SW1 raw string: '%s'\n", lcd_request.string);
            snprintf(lcd_request.string, sizeof(lcd_request.string), "sw1 count %lu", button_count);

            //Send message to LCD task
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY); 
        }
    }
}

void task_sw2(void *pvParameters) {
    uint32_t button_count = 0;
    uint32_t debounce_count = 0;
    lcd_msg_request_t lcd_request;

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(25));

        //check button
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_SW2) == 0) {
            if (debounce_count <= 40) {
                debounce_count++;
            }
        } else {
            debounce_count = 0;
        }

        if (debounce_count == 40) {
            button_count++;

            lcd_request.cmd = LCD_PRINT_SW2_COUNT;
            lcd_request.return_queue = NULL;
            snprintf(lcd_request.string, sizeof(lcd_request.string), "sw2 count %lu", button_count);

            //Send message to LCD task
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
    }
}

void task_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    int8_t row = 0;
    int8_t col = 0;
    int8_t button_presses = 0;
    EventBits_t events;
    
    lcd_msg_t lcd_msg;
    
    // Clear the screen

    // Request 20 bytes to store a string

    // Print the number of button presses to the LCD

    while(1)
    {
        // Wait for SW1 events
        
        // Update the button press count
    
        // Print the number of button presses to the LCD
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Buttons initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
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

    /* Register the tasks with FreeRTOS*/
    ECE353_RTOS_Events = xEventGroupCreate();

    //create message queue for LCD requests
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));

    /* Initialize the Button Task resources */
    if (!task_button_init())
    {
        printf("Failed to initialize button task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0); // If the task initialization fails, assert
    }

    /* Initialize LCD resources */ 
    if (!task_lcd_resources_init(xQueue_Request_LCD))
    {
        printf("Failed to initialize lcd task\n\r");
        for(int i = 0; i < 100000; i++) {}
       CY_ASSERT(0); // If the task initialization fails, assert
    }

    //Initialize switch tasks
    if (xTaskCreate(
        task_sw1,
        "SW1 Task", 
        2*configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1U, 
        NULL
        ) != pdPASS) {
            printf("SW1 task creation failed\n\r");
            for (int i = 0; i < 100000; i++) {}
            CY_ASSERT(0);
        }

    if (xTaskCreate(
        task_sw2,
        "2W1 Task", 
        2*configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1U, 
        NULL
        ) != pdPASS) {
            printf("SW2 task creation failed\n\r");
            for (int i = 0; i < 100000; i++) {}
            CY_ASSERT(0);
        }    

    /*
    xTaskCreate(
        task_system_control, 
        "Task System Control", 
        configMINIMAL_STACK_SIZE*10, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );
    */

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif