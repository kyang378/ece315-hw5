/**
 * @file ice08.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE08)
#include "drivers.h"        
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_lcd.h"
#include "task_joystick.h"

char APP_DESCRIPTION[] = "ECE353: ICE 08 - FreeRTOS LCD Gatekeeper";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
EventGroupHandle_t ECE353_RTOS_Events = NULL;
QueueHandle_t xQueue_Request_LCD = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_sw1(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    uint32_t button_count = 0;
    uint32_t debounce_count = 0;
    lcd_msg_request_t lcd_request;

    printf("Starting Task SW1\n\r");
    while(1)
    {
        // Sleep for 25 ms
        vTaskDelay(pdMS_TO_TICKS(25));
        // check the button
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) == 0) // active low button
        {
            if (debounce_count <= 40)
            {
                debounce_count++;
                printf("TEST: button_count incremented.\n\r");
            }
        }
        else {
            debounce_count = 0;
        }

        if (debounce_count == 40)
        {
            // increment button_count and create the lcd message
            button_count++;
            lcd_request.msg.command = LCD_CMD_PRINT_SW1_COUNT;
            lcd_request.return_queue = NULL; // no message expected in return from gatekeeper
            
            // print the string into the payload of the message of our lcd request using snprintf
            snprintf(lcd_request.msg.payload.message, sizeof(lcd_request.msg.payload.message), "SW1 Count: %lu", (unsigned long)button_count);

            // send the message to task_LCD
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
            printf("SW1 send: %s\n\r", lcd_request.msg.payload.message);
        }
    }
}

void task_sw2(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    uint32_t button_count = 0;
    uint32_t debounce_count = 0;
    lcd_msg_request_t lcd_request; // request to be formed and sent out

    printf("Starting Task SW2\n\r");
    while(1)
    {
        // Sleep for 25 ms
        vTaskDelay(pdMS_TO_TICKS(25));

        // check the button
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) == 0) // active low button
        {
            if (debounce_count <= 40)
            {
                debounce_count++;
            }
        }
        else {
            debounce_count = 0;
        }

        if (debounce_count == 40)
        {
            // increment button_count and create the lcd message
            button_count++;
            lcd_request.msg.command = LCD_CMD_PRINT_SW2_COUNT;
            lcd_request.return_queue = NULL; // no message expected in return from gatekeeper
            
            snprintf(lcd_request.msg.payload.message, sizeof(lcd_request.msg.payload.message), "SW2 Count: %lu", (unsigned long)button_count);
            printf("SW2 pressed, count=%lu\n\r", (unsigned long)button_count);

            // send the message to task_LCD
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
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

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Buttons initialization failed!\n\r");
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
    // Create the RTOS event group, which will be used for the button events. 
    ECE353_RTOS_Events = xEventGroupCreate();
    
    /* Create the LCD Request Queue*/
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));


    /* Register the tasks with FreeRTOS*/

    // this function binds the static global queue from task_lcd.c to the queue in the argument
    if (!task_lcd_resources_init(xQueue_Request_LCD))
    {
        printf("LCD Task Resource Initialization Failed!\n\r");
        for (int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    xTaskCreate(
        task_sw1, 
        "Task SW1", 
        configMINIMAL_STACK_SIZE*2, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    xTaskCreate(
        task_sw2, 
        "Task SW2", 
        configMINIMAL_STACK_SIZE*2, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    // clear the screen (so we start with a black screen and we print strings in white)
    lcd_clear_screen(LCD_COLOR_BLACK);
    printf("TEST2: cleared the screen\n\r");

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif
