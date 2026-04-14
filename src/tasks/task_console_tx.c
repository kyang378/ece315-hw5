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

// We need a transmit queue for all the FreeRTOS tasks that want to use the UART to send messages to the console. This queue will be used to send messages from the tasks to the gatekeeping console Tx task, which will then send the messages to the UART hardware FIFO through a circular buffer.

// Therefore, we need to declare a transmit queue as well as a circular buffer.
// NOTE: to link this bottom-half task with the IRS, so we're using the same queue and the
// same circular buffer, we have to make sure that they share the same name.

// the queue and the circular buffer are defined in task_console.h as extern, so we can define them here in the .c file with the same name!
QueueHandle_t xQueue_Console_Tx;
circular_buffer_t *circular_buffer_tx; // circular buffer for UART Tx data


/**
 * @brief 
 * This gatekeeper task manages the reception of console packets from other tasks and
 * transmits characters to the UART
 * @param param 
 */
void task_console_tx(void *param)
{
    (void)param; // Unused parameter
    console_buffer_t tx_msg;

    while (1)
    {
        // Wait for packets (console_buffer_t messages) to arrive from the queue used by
        // other tasks to send messages they want to print to the console by asking for the 
        // gatekeeper task to do it for them. The packets will arrive into the tx_msg buffer
        // instead of doing block-by-block copy
        if (xQueueReceive(xQueue_Console_Tx, &tx_msg, portMAX_DELAY) != pdPASS)
        {
            /* Failed to receive message from the queue. Handle error */
            CY_ASSERT(0); // Halt the processor
        }


        // Once we have the packet, we use a for loop to examine the message and adds each byte into the circular buffer
        for (uint32_t i = 0; i < strlen(tx_msg.data); i++)
        {
            // If the circular buffer is full, we block ourselves (this task) for 5ms. Meanwhile while we are blocked, the FIFO empty interrupt is still occuring, which will allow the ISR to remove data from the circular buffer and add it to the hardware FIFO, thus freeing up space in the circular buffer.
            while (circular_buffer_full(circular_buffer_tx))
            {
                vTaskDelay(pdMS_TO_TICKS(5)); // block for 5 ms
            }

            // Add the next byte to the circular buffer when some space has been freed up by the ISR. Make sure this happens atomically.
            taskENTER_CRITICAL();
            circular_buffer_add(circular_buffer_tx, tx_msg.data[i]);
            taskEXIT_CRITICAL();
        }

        // Now we have data in the circular buffer, we can enable the transmit-empty interrupt to prompt the ISR (the consumer) to pull data from the buffer and put it in the hardware FIFO to be sent out to the console, thus starting the transmission of the message to the console.

        // priority = 7 because it's hardware interrupt
        // QUESTION: are the interrupt priorities and the task priorities measured by the same scale? - no, but for interrupts, it's 0-7
        cyhal_uart_enable_event(&cy_retarget_io_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, 7, true);

        // When we're done adding the message to the circular buffer, we need to free the 
        // data that was sent from the console_buffer_t, which was malloc'd by the sender 
        // task (the "necessary info" needed by us the gatekeeper task to do what they want 
        // to do for them). The task that consumes the message (us) from the queue needs to 
        // free that data back.
        vPortFree(tx_msg.data); // recall the .data is a pointer to the characters

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

    // Initialize the TX FreeRTOS queue for messages to be sent to the console Tx task
    xQueue_Console_Tx = xQueueCreate(10, sizeof(console_buffer_t));
    if (xQueue_Console_Tx == NULL)
    {
        return false; // Initialization failed
    }

    // Initialize the circular buffer for UART TX data
    circular_buffer_tx = circular_buffer_init(CONSOLE_MAX_MESSAGE_LENGTH);

    // Note: we do not turn on the transmit-empty interrupts until we have data to send in
    // the circular buffer, so we will enable the transmit-empty interrupt in task_console_tx when we have data to send.

    // DON'T FORGET TO REGISTER THE TX TASK!
    rslt = xTaskCreate(
        task_console_tx, 
        "Console Tx Task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

     if (rslt != pdPASS)
    {
        return false; // Initialization failed
    }

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
    BaseType_t scheduler_running;

    scheduler_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);

    if ((xQueue_Console_Tx == NULL) || (!scheduler_running))
    {
        va_start(args, str_ptr);
        vprintf(str_ptr, args);
        va_end(args);
        return;
    }

    /* ADD CODE */
    /* Allocate the message buffer */
    message_buffer = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH); // would point to the formatted message

    if (message_buffer)
    {
        va_start(args, str_ptr);

        // tells us what task is trying to send the message to the console
        task_name = pcTaskGetName(xTaskGetCurrentTaskHandle());
        length = snprintf(message_buffer, CONSOLE_MAX_MESSAGE_LENGTH, "%-17s: ",
                              task_name);

        // tells us the format of the message
        vsnprintf((message_buffer + length), (CONSOLE_MAX_MESSAGE_LENGTH - length),
                  str_ptr, args);

        va_end(args);

        /* Initialize the console buffer */
        console_buffer.data = message_buffer;
        // index tells us how many characters we got
        // each time we send one character from the console buffer to the circular buffer
        // we decrement this amount by 1. It's a for loop that sends the characters one by one to the circular buffer
        console_buffer.index = length;
        

        /* Sends the data (encapsulated by the console_buffer) to the queue. Then task_console_tx is going to receive the console buffer from the queue
        and get that data one by one into the circular buffer using a for loop as discussed, whose index is the index we set correctly as the length of the message we have.
        NOTE: The receiver task is responsible to free the memory from here on
        
        we correctly did all these in task_console_tx() function */
        if (xQueueSend(xQueue_Console_Tx, &console_buffer, portMAX_DELAY) != pdPASS)
        {
            /* Failed to send message to the console Tx task. Handle error */
            CY_ASSERT(0); // Halt the processor
        }

    }
    else
    {
        /* pvPortMalloc failed. Handle error */
        CY_ASSERT(0); // Halt the processor
    }
}
#endif
