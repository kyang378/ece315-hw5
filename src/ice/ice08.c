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
#include "master_mind_lib.h"
char APP_DESCRIPTION[] = "ECE353: ICE 08 - FreeRTOS LCD Gatekeeper";
/*****************************************************************************/
/* Macros */
/*****************************************************************************/
/*****************************************************************************/
/* Global Variables */
/*****************************************************************************/
/* ADD CODE */
/* FreeRTOS Queue for LCD messages */
QueueHandle_t Queue_LCD_Request = NULL;
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/*****************************************************************************/
/* Function Declarations */
/*****************************************************************************/
/*****************************************************************************/
/* Function Definitions */
/*****************************************************************************/
void task_sw1(void *pvParameters)
{
    (void)pvParameters; // Unused parameter
    printf("Starting Task SW1\n\r");

    int button_count = 0;
    int debounce_count = 0;
    lcd_msg_request_t lcd_request;

    while(1)
    {
    // Sleep for 25 ms
    vTaskDelay(pdMS_TO_TICKS(25));
    /* ADD CODE */
    //check button
    if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_SW1) == 0) {
        if(debounce_count <= 40) {
            debounce_count++;
        }
    } else {
        debounce_count = 0;
    }

    if (debounce_count == 40) {
        button_count++;
        lcd_request.msg.command = LCD_CMD_PRINT_SW1_COUNT;
        lcd_request.return_queue = NULL;
        printf("SW1 detected");
        snprintf(lcd_request.msg.payload.message, 32, "SW1 %d", button_count);
        xQueueSend(Queue_LCD_Request, &lcd_request, portMAX_DELAY);
    }
    }
}


void task_sw2(void *pvParameters)
{
    (void)pvParameters; // Unused parameter
    printf("Starting Task SW2\n\r");

    int button_count = 0;
    int debounce_count = 0;
    lcd_msg_request_t lcd_request;

    while(1)
    {
    // Sleep for 25 ms
    vTaskDelay(pdMS_TO_TICKS(25));
    /* ADD CODE */
    //check button
    if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_SW2) == 0) {
        if(debounce_count <= 40) {
            debounce_count++;
        }
    } else {
        debounce_count = 0;
    }

    if (debounce_count == 40) {
        button_count++;
        lcd_request.msg.command = LCD_CMD_PRINT_SW2_COUNT;
        lcd_request.return_queue = NULL;
        printf("SW2 detected");
        snprintf(lcd_request.msg.payload.message, 32, "SW2 %d", button_count);
        xQueueSend(Queue_LCD_Request, &lcd_request, portMAX_DELAY);
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
/* Application Code */
/*****************************************************************************/
/**
* @brief
* This function implements the behavioral requirements for the ICE
*/
void app_main(void)
{
    /* Register the tasks with FreeRTOS*/
    ECE353_RTOS_Events = xEventGroupCreate();
    /* Create the LCD Request Queue*/
    Queue_LCD_Request = xQueueCreate(10, sizeof(lcd_msg_request_t));

    /* ADD CODE */
    /* Initialize the Button Task resources */
    if (!task_button_init())
    {
        printf("Failed to initialize button task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0); // If the task initialization fails, assert
    }

    /* Initialize LCD resources */ 
    if (!task_lcd_resources_init(Queue_LCD_Request))
    {
        printf("Failed to initialize lcd task\n\r");
        for(int i = 0; i < 100000; i++) {}
       CY_ASSERT(0); // If the task initialization fails, assert
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
    /* Start the scheduler*/ 
    vTaskStartScheduler();
    /* Will never reach this loop once the scheduler starts */

    while (1)
    {
    }
}
#endif