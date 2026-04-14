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
#include "devices.h"
#include "task_eeprom.h"
#include "task_cap_touch.h"
/**
 * @brief
 * This file contains the implementation of the console receive (Rx) task.
 * The task is responsible for processing incoming console commands and
 * controlling the state of the LEDs accordingly.
 * 
 * The task uses a double buffer to process the incoming console commands.
 * The supported commands are "EEPROM R ADDR", "EEPROM W ADDR VAL", and
 * "CAP_TOUCH".
 */


 /* Global Variables */
console_buffer_t console_buffer1;
console_buffer_t console_buffer2;
static QueueHandle_t Queue_Console_Rx_Response = NULL;

// allocate pointers to the buffers
console_buffer_t *produce_console_buffer;
console_buffer_t *consume_console_buffer;

// allocate a task handle for task notifications from the ISR to our bottom-half task
TaskHandle_t TaskHandle_Console_Rx;

// helper function to append a request with the correct return queue,
// send it to a gatekeeper task, and wait for the response
static bool task_console_send_request_and_wait(
    device_request_msg_t *request,
    device_response_msg_t *response
)
{
    QueueHandle_t request_queue = NULL;

    if(request == NULL || response == NULL || Queue_Console_Rx_Response == NULL)
    {
        return false;
    }

    switch(request->device)
    {
        case DEVICE_EEPROM:
            request_queue = Queue_EEPROM_Requests;
            break;

        case DEVICE_CAP_TOUCH:
            request_queue = Queue_Request_Cap_Touch;
            break;
        default:
            return false;
    }

    if(request_queue == NULL)
    {
        return false;
    }

    //set the response queue in the request packet
    request->response_queue = Queue_Console_Rx_Response;
    xQueueReset(Queue_Console_Rx_Response);

    // send request and wait for response
    if(xQueueSend(request_queue, request, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    if(xQueueReceive(Queue_Console_Rx_Response, response, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    return true;
}

/**
 * @brief
 * This function is the bottom half task for receiving console input.
 *
 * It waits for a task notification from the ISR indicating that a new 
 * command has been received. The task then processes the command and 
 * sends the appropriate request to the corresponding gatekeeper task
 * based on the command received.
 *
 * When one of the eeprom commands has been successfully detected, 
 * print out a message to the console indicating which command was 
 * received and any parameters supplied to that command using 
 * task_console_printf().
 *
 * After sending the request message, task_console_rx() should wait for the
 * response from the gatekeeper task before processing the next command. 
 * task_eeprom() shall send a device_request_msg_t using the response_queue 
 * specified in the device_request_msg_t
 *
 * @param param Unused parameter
 */
void task_console_rx(void *param)
{
    (void)param; // Unused parameter

    while (1)
    {
        device_request_msg_t request = {0};
        device_response_msg_t response = {0};

        // Wait indefinitely for a Task Notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // parse the received command in consume_console_buffer and fill in the request structure
        if (parse_cli_data(consume_console_buffer->data, &request))
        {
            if(task_console_send_request_and_wait(&request, &response))
            {
                if(request.device == DEVICE_EEPROM)
                {
                    if(response.status == DEVICE_OPERATION_STATUS_WRITE_SUCCESS)
                    {
                        task_console_printf(
                            "EEPROM WRITE: Addr=0x%04X, Value=0x%02X\r\n\r\n",
                            request.address,
                            request.value
                        );
                    }
                    else if(response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
                    {
                        task_console_printf(
                            "EEPROM READ: Addr=0x%04X, Value=0x%02X\r\n\r\n",
                            request.address,
                            response.payload.eeprom
                        );
                    }
                    else if(response.status == DEVICE_OPERATION_STATUS_WRITE_FAILURE)
                    {
                        task_console_printf(
                            "Failed to write EEPROM at Addr=0x%04X\r\n\r\n",
                            request.address
                        );
                    }
                    else if(response.status == DEVICE_OPERATION_STATUS_READ_FAILURE)
                    {
                        task_console_printf(
                            "Failed to read EEPROM at Addr=0x%04X\r\n\r\n",
                            request.address
                        );
                    }
                }
                else if (request.device == DEVICE_CAP_TOUCH)
                {
                    if(response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
                    {
                        task_console_printf(
                            "Cap Touch: Sensor 0=%d, Sensor 1=%d\r\n\r\n",
                            response.payload.cap_touch[0],
                            response.payload.cap_touch[1]
                        );
                    }
                    else if(response.status == DEVICE_OPERATION_STATUS_READ_FAILURE)
                    {
                        task_console_printf(
                            "Failed to read Capacitive Touch data\r\n\r\n"
                        );
                    }
                }
            }
            else
            {
                task_console_printf("Failed to process command\r\n\r\n");
            }
        } else
        {
            task_console_printf("Received unrecognized command\r\n\r\n");
        }
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

    if(console_buffer1.data == NULL || console_buffer2.data == NULL)
    {
        return false;
    }

    // initialize the produce and consume pointers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    // set the initial indices of the buffers to 0
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;
    produce_console_buffer->data[0] = '\0';
    consume_console_buffer->data[0] = '\0';

    Queue_Console_Rx_Response = xQueueCreate(1, sizeof(device_response_msg_t));
    if(Queue_Console_Rx_Response == NULL)
    {
        return false;
    }

    // Create the console Rx task (the bottom-half task that processes the received commands)
    rslt = xTaskCreate(
        task_console_rx, 
        "Task Console Rx", 
        configMINIMAL_STACK_SIZE*5, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        &TaskHandle_Console_Rx
    );

    return (rslt == pdPASS); // Resources initialized successfully
}
#endif
