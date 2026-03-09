/**
 * @file task_console_rx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#ifdef ECE353_FREERTOS
#include "drivers.h"
#include "task_console.h"
#include "cyhal_uart.h"
/**
 * @brief
 * This file contains the implementation of the console receive (Rx) task.
 * The task is responsible for processing incoming console commands and
 * controlling the state of the LEDs accordingly.
 * 
 * The task uses a double buffer to process the incoming console commands.
 * The supported commands will be "RED_ON" and "RED_OFF" to control the red LED.
 */

/* ADD CODE */
/* Global Variables */
console_buffer_t console_buffer1;
console_buffer_t console_buffer2;

// allocate pointers to the buffers
console_buffer_t *produce_console_buffer;
console_buffer_t *consume_console_buffer;

// allocate a task handle for task notifications from the ISR to our bottom-half task
TaskHandle_t TaskHandle_Console_Rx;

/**
 * @brief
 * This function is the bottom half task for receiving console input.
 *
 * It waits for a task notification from the ISR indicating that a new 
 * command has been received. The task then processes the command and 
 * controls the state of the LEDs accordingly.
 *
 * @param param Unused parameter
 */
void task_console_rx(void *param)
{
    (void)param; // Unused parameter
    while (1)
    {
        // Wait indefinitely for a Task Notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Process data pointed to by console buffer ptr when notification arrives:

        // if "RED_ON" received, turn on the red led
        if (strcmp(consume_console_buffer->data, "RED_ON") == 0)
        {
            // Turn on the red led
            leds_set_state(LED_RED, LED_STATE_ON);
        }

        // if "RED_OFF" received, turn off the red led
        else if (strcmp(consume_console_buffer->data, "RED_OFF") == 0)
        {
            // Turn off the red led
            leds_set_state(LED_RED, LED_STATE_OFF);
        }

        // All other commands are ignored
    }
}

/**
 * @brief
 * This function initializes the resources for the console Rx task.
 * @return true if resources were initialized successfully
 * @return false if resource initialization failed
 */
bool task_console_resources_init_rx(void)
{
    BaseType_t rslt;

    // Allocate an array of data from the heap for the console buffers
    // pvPortMalloc is a FreeRTOS function that allocates memory from the heap; it is similar to malloc in standard C, but it is thread-safe and can be used in FreeRTOS applications. We need to allocate memory for the console buffers because they will be used to store the incoming console commands, and we want to ensure that we have enough space to hold the maximum message length defined by CONSOLE_MAX_MESSAGE_LENGTH.
    console_buffer1.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    console_buffer2.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);

    // initialize the produce and consume pointers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    // set the initial indices of the buffers to 0
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;

    // Create the console Rx task (the bottom-half task that processes the received commands)
    rslt = xTaskCreate(
        task_console_rx, 
        "Console Rx Task", 
        configMINIMAL_STACK_SIZE*2, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        &TaskHandle_Console_Rx
    );

    return (rslt == pdPASS); // Resources initialized successfully
}
#endif