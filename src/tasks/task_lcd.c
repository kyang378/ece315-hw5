/**
 * @file task_lcd.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_lcd.h"

#if defined(ECE353_FREERTOS)

/* FreeRTOS Queue for LCD messages */
static QueueHandle_t Queue_Requests = NULL;

/* LCD Task */
void task_lcd(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    lcd_msg_request_t lcd_request; // buffer where the received item will be copied into
    lcd_msg_response_t response;
    bool status = false;

    while(1)
    {
        xQueueReceive(Queue_Requests, &lcd_request, portMAX_DELAY);
        printf("LCD recv cmd=%d msg=%s\n\r",
            lcd_request.msg.command,
            lcd_request.msg.payload.message);

        switch (lcd_request.msg.command) {
            case LCD_CMD_CLEAR_SCREEN:
            {
                /* Clear the screen */
                // we have already wrote the function for parsing the message and doing the command
                master_mind_handle_msg(&lcd_request.msg);
                break;
            }
            case LCD_CMD_PRINT_SW1_COUNT:
            {
                /* Print out the number of times SW1 has been pressed onto the LCD starting at (10,50). Handled by the master_mind_handle_msg function */
                master_mind_handle_msg(&lcd_request.msg);
                break;
            }
            case LCD_CMD_PRINT_SW2_COUNT:
            {
                /* Print out the number of times SW2 has been pressed onto the LCD starting at (10,100). Handled by the master_mind_handle_msg function */
                master_mind_handle_msg(&lcd_request.msg);
                break;
            }
            default:
            {
                /* Invalid command */
                status = false;
            }
            break;
        }
    }
}

/* LCD Task Initialization */
bool task_lcd_resources_init(QueueHandle_t queue_request){

    BaseType_t result;

    if (queue_request == NULL)
    {
        return false;
    }
    Queue_Requests = queue_request;

    /* Create the LCD Task */
    result= xTaskCreate(
        task_lcd,                       // Task function
        "LCD Task",                     // Task name
        TASK_LCD_STACK_SIZE,            // Stack size
        NULL,                           // Task parameters
        TASK_LCD_PRIORITY,              // Task priority
        NULL                            // Task handle
    );

    if(result != pdPASS)
    {
        return false;
    }   

    return true;
}
#endif