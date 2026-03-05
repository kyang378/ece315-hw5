/**
 * @file task_console.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-15
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
 * This file contains the implementation of the console transmit (Tx) task.
 * The task is responsible for sending characters to the UART.
 * 
 * Tasks can print messages by sending the string to task_console_tx() using
 * a FreeRTOS queue.
 *
 * task_console_tx() will add the characters to a circular buffer that is
 * accessed by the UART interrupt service routine (ISR).
 *
 */

/* ADD CODE*/
/* Global Variables */
TaskHandle_t TaskHandle_Console_Tx;
//Allocate space for transmit queue
QueueHandle_t console_tx_queue = NULL;

//Allocate space for circular buffer
circular_buffer_t *circular_buffer_tx = NULL;

/**
 * @brief 
 * This task is used to transmit characters to the UART
 * @param param 
 */
void task_console_tx(void *param)
{
    (void)param; // Unused parameter
    console_buffer_t tx_msg;

    while (1)
    {
        /* ADD CODE */

        //wait for console_buffer_t messages from queue
        xQueueReceive(console_tx_queue, &tx_msg, portMAX_DELAY);
        //for loop: examine the message and copy each byte into the circular buffer
        for (uint32_t i = 0; tx_msg.data[i] != '\0'; i++)
        {
            //if buffer is full, vTaskDelay(5);
            while (circular_buffer_full(circular_buffer_tx))
                {
                    vTaskDelay(pdMS_TO_TICKS(5));
                }

            //add next byte to the buffer
            circular_buffer_add(circular_buffer_tx, tx_msg.data[i]);
        }
            
        //printf("Enabling TX interrupt");
        //enable the transmit empty interrupts
        cyhal_uart_enable_event(&cy_retarget_io_uart_obj,
                            CYHAL_UART_IRQ_TX_EMPTY,
                            CYHAL_ISR_PRIORITY_DEFAULT,
                            true);


        //free the data that was sent from the console_buffer_t
        vPortFree(tx_msg.data);
    }
}

/**
 * @brief 
 * This function initializes the resources for the console Tx task. 
 * @return true  if initialization is successful
 * @return false if initialization fails
 * @return false 
 */
bool task_console_resources_init_tx(void)
{
    BaseType_t rslt = pdPASS;

    /* ADD CODE */

    // initialize the Tx FreeRTOS queue
    console_tx_queue = xQueueCreate(CONSOLE_QUEUE_LENGTH, sizeof(console_buffer_t));

    // initialize the circular buffer
    circular_buffer_tx = circular_buffer_init(CONSOLE_MAX_MESSAGE_LENGTH);

    // ensure TX empty interrupts start disabled
    cyhal_uart_enable_event(&cy_retarget_io_uart_obj,
                            CYHAL_UART_IRQ_TX_EMPTY,
                            CYHAL_ISR_PRIORITY_DEFAULT,
                            false);

   // printf("Creating task_console_tx\n");
    //create the task
    rslt = xTaskCreate(
        task_console_tx,
        "Console Tx",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        &TaskHandle_Console_Tx
    );


    if (rslt != pdPASS)
    {
        return false; // Initialization failed
    }

    return true; // Resources initialized successfully
}

/**
 * @brief
 * This function sends formatted messages to task_console_tx. It acts as a wrapper around the FreeRTOS queue
 * to send messages so other tasks can use it easily.
 *
 * Example usage: 
 * task_console_printf("Send Message");
 * task_console_printf("Formatted number: %d", 42);
 *
 * @param str_ptr Pointer to the format string.
 * @param ...     Additional arguments for formatting.
 */
void task_console_printf(char *str_ptr, ...)
{
    console_buffer_t console_buffer;
    char *message_buffer;
    char *task_name;
    uint32_t length = 0;
    va_list args;

    /* ADD CODE */
    /* Allocate the message buffer */
    message_buffer = pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    if (message_buffer)
    {
        va_start(args, str_ptr);
        task_name = pcTaskGetName(xTaskGetCurrentTaskHandle());
        length = snprintf(message_buffer, CONSOLE_MAX_MESSAGE_LENGTH, "%-16s : ",
                              task_name);

        vsnprintf((message_buffer + length), (CONSOLE_MAX_MESSAGE_LENGTH - length),
                  str_ptr, args);

        va_end(args);

        /* ADD CODE */
        /* Initialize the console buffer */
        console_buffer.data = message_buffer;
        console_buffer.index = 0;
        /* ADD CODE */
        /* The receiver task is responsible to free the memory from here on */
        xQueueSend(console_tx_queue, &console_buffer, portMAX_DELAY);

        //need to free : vPortFree
        //vPortFree(message_buffer);
    }
    else
    {
        /* pvPortMalloc failed. Handle error */
        CY_ASSERT(0); // Halt the processor
    }
}
#endif