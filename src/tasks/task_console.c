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
#include "task_console.h"

#ifdef ECE353_FREERTOS  
static bool Console_Ignore_Line_Feed = false;

/**
 * @brief 
 * This function is the event handler for the console UART. This is the ISR for both
 * the transmit and receive interrupts.
 *
 * The ISR will receive characters from the UART and store them in a console buffer
 * until the user presses the ENTER key.  At that point, the ISR will send a task
 * notification to the console task to process the received string.
 *
 * The ISR will also echo the received character back to the console.
 *
 * @param handler_arg 
 * @param event 
 */
void console_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t c; // the character received from the UART

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        // Read in the character
        cyhal_uart_getc(&cy_retarget_io_uart_obj, &c, 0);

        // Ignore the LF in a CRLF pair so one ENTER only produces one command.
        if (Console_Ignore_Line_Feed && c == '\n')
        {
            Console_Ignore_Line_Feed = false;
        }
        // if character is equal to backspace or the delete key,
        // remove the last character from the array
        else if (c == '\b' || c == 127) // 127 is the ASCII code for delete
        {
            Console_Ignore_Line_Feed = false;

            if (produce_console_buffer->index > 0)
            {
                produce_console_buffer->index--;
                produce_console_buffer->data[produce_console_buffer->index] = '\0';

                cyhal_uart_putc(&cy_retarget_io_uart_obj, '\b');
                cyhal_uart_putc(&cy_retarget_io_uart_obj, ' ');
                cyhal_uart_putc(&cy_retarget_io_uart_obj, '\b');
            }
        }
        // else if the current charcter is the \n or \r 
        else if (c == '\n' || c == '\r')
        {
            // Normalize line endings so the terminal cursor stays aligned.
            cyhal_uart_putc(&cy_retarget_io_uart_obj, '\r');
            cyhal_uart_putc(&cy_retarget_io_uart_obj, '\n');

            Console_Ignore_Line_Feed = (c == '\r');

            // null terminate the string
            produce_console_buffer->data[produce_console_buffer->index] = '\0';
            
            // swap the role of the produce and consume buffers beofre sending a task notification for the bottom half task to process the received string
            console_buffer_t *temp = produce_console_buffer;
            produce_console_buffer = consume_console_buffer;
            consume_console_buffer = temp;

            // the second argument (the flag) indicates if we need to yield to a different 
            // task than the one that is interrupted. If our bottom half task does have a higher priority than the currently running task, then this xHigherPriorityTaskWoken boolean flag will be set to 1.

            // then, portYIELD_FROM_ISR() will check the value of xHigherPriorityTaskWoken, and if it is 1, it will yield to the higher priority task that was just woken up by the task notification. If it is 0, then it will continue executing the current task after the ISR finishes (and when the current task finishes its time slice, then we would run the bottom half task)


            // change the index back to 0 for the next round of receiving characters first
            produce_console_buffer->index = 0; // reset the index for the next message
            produce_console_buffer->data[0] = '\0';

            // so the difference we make by doing the following two lines, is so that when
            // our bottom half test does have a higher priority than the current one, instead of waiting for the current task to finish its time slice, we can immediately switch to the bottom half task right after the ISR finishes, which allows us to process the received console command with lower latency.
            vTaskNotifyGiveFromISR(TaskHandle_Console_Rx, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // yield to the bottom half task

        }else {
            Console_Ignore_Line_Feed = false;

            if (produce_console_buffer->index < CONSOLE_MAX_MESSAGE_LENGTH - 1) 
            {
                cyhal_uart_putc(&cy_retarget_io_uart_obj, c);
                produce_console_buffer->data[produce_console_buffer->index] = c;
                produce_console_buffer->index++;
            }
        }
    }

    // if the UART's hardware TX FIFO is empty, we can accept another block of data into our FIFO to send to the console, by giving a task notification to the bottom-half task (console Tx task) to send the next block of data in the circular buffer to the hardware FIFO
    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
        // first, we want to make sure there is actually data in the circular buffer
        // if it's empty, we would disable the transmit-empty interrupt, so we don't keep getting interrupts when there is no data to send
        if (circular_buffer_empty(circular_buffer_tx))
        {
            cyhal_uart_enable_event(&cy_retarget_io_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, 7, false);
        }
        
        // if the circular buffer is not empty, get the next character from the circular buffer and send it to the hardware FIFO
        if (!circular_buffer_empty(circular_buffer_tx))
        {
            circular_buffer_remove(circular_buffer_tx, (char*) &c);
            cyhal_uart_putc(&cy_retarget_io_uart_obj, c);
        }
    }
    else
    {
    }
}

/**
 * @brief
 * This function initializes the console tasks and resources.
 * @return true
 * @return false
 */
bool task_console_init(void)
{
    /* Register a function for the UART ISR*/
    cyhal_uart_register_callback(
        &cy_retarget_io_uart_obj,           // UART object
        console_event_handler,         // Event handler
        NULL                       // Handler argument
    );

    /* Initialize UART Rx Resources */
    if (!task_console_resources_init_rx())
    {
        return false; // Initialization failed
    }

    /* Initialize UART Tx Resources */
    if (!task_console_resources_init_tx())
    {
        return false; // Initialization failed
    }
    else
    {
        // Enable UART Rx Interrupts
        cyhal_uart_enable_event(
            &cy_retarget_io_uart_obj, 
            CYHAL_UART_IRQ_RX_NOT_EMPTY, 
            7, 
            true);
    }
    
    return true; // Initialization successful
}
#endif  
