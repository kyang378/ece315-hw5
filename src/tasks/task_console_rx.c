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

//buffers
console_buffer_t console_buffer1;
console_buffer_t console_buffer2;

//buffer pointers
console_buffer_t *produce_console_buffer;
console_buffer_t *consume_console_buffer;

//task handle
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
        /* ADD CODE */
        //wait indefinitely for a task notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        //printf("Received string: '%s'\n", consume_console_buffer->data);

        //process data pointed to by console buffer pointer (will most likely require string compare)
        //if "RED_ON", turn on red LED
        if (strcmp(consume_console_buffer->data, "RED_ON") == 0)
        {
            //printf("RED_ON received \n");
            //turn on red LED
            leds_set_state(LED_RED, LED_STATE_ON);
        }
        else if (strcmp(consume_console_buffer->data, "RED_OFF") == 0)
        {
            //if "RED_OFF", turn off red LED
            //printf("RED_OFF received \n");
            //turn off red LED
            leds_set_state(LED_RED, LED_STATE_OFF);
        }
        

        //ignore everything else (clear buffer required? not specifically mentioned)

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

    /* ADD CODE */
    //allocate an array of data from the heap for the console buffers
    console_buffer1.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    console_buffer2.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);

    //initialize pointers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    //set initial indeces to 0
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;

    //create the console rx task
    rslt = xTaskCreate(
        task_console_rx,
        "Console Rx",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        &TaskHandle_Console_Rx
    );

    return (rslt == pdPASS); // Resources initialized successfully
}
#endif