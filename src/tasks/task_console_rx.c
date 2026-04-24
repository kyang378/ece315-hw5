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

//response queue handle
QueueHandle_t Queue_Console_Response;

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
    (void)param;

    //give eeprom task time to start
    vTaskDelay(pdMS_TO_TICKS(20));

    while (1)
    {
        // Wait for ISR to notify that a full command is ready
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char *cmd = consume_console_buffer->data;

        // -----------------------------------------
        // Handle LED commands (simple string compare)
        // -----------------------------------------
        if (strcmp(cmd, "RED_ON") == 0)
        {
            leds_set_state(LED_RED, LED_STATE_ON);
            continue;
        }
        else if (strcmp(cmd, "RED_OFF") == 0)
        {
            leds_set_state(LED_RED, LED_STATE_OFF);
            continue;
        }

        //use parse_cli_data to process other reqests
        device_request_msg_t request;

        
        if (parse_cli_data(cmd, &request)) {

            // Assign response queue for any device
            request.response_queue = Queue_Console_Response;

            device_response_msg_t response;

            if (request.device == DEVICE_EEPROM) {
                // Optional: print write info
                if (request.operation == DEVICE_OP_WRITE) {
                    task_console_printf(
                        "EEPROM WRITE: Addr=0x%04X, Value=0x%02X\r\n",
                        request.address,
                        request.value
                    );
                }

                // Send to EEPROM gatekeeper
                xQueueSend(Queue_EEPROM_Requests, &request, portMAX_DELAY);

                // Wait for response
                xQueueReceive(Queue_Console_Response, &response, portMAX_DELAY);

                // Print read result
                if (request.operation == DEVICE_OP_READ) {
                    task_console_printf(
                        "EEPROM READ: Addr=0x%04X, Value=0x%02X\r\n",
                        request.address,
                        response.payload.eeprom
                    );
                }
            }
            else if (request.device == DEVICE_CAP_TOUCH) {
                // Send to Cap Touch gatekeeper
                xQueueSend(Queue_Request_Cap_Touch, &request, portMAX_DELAY);

                // Wait for response
                xQueueReceive(Queue_Console_Response, &response, portMAX_DELAY);

                if (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS) {
                    task_console_printf(
                        "Cap Touch: Sensor0(X)=%u, Sensor1(Y)=%u\r\n",
                        response.payload.cap_touch[0],
                        response.payload.cap_touch[1]
                    );
                } else {
                    task_console_printf("Failed to read Capacitive Touch data\r\n");
                }
            }

            continue;
        }


        //Unknown command
        task_console_printf("Unknown command received \r\n");
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

    Queue_Console_Response = xQueueCreate(
        1,
        sizeof(device_response_msg_t)
    );


    //create the console rx task
    rslt = xTaskCreate(
        task_console_rx,
        "Console Rx",
        4*configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        &TaskHandle_Console_Rx
    );

    return (rslt == pdPASS); // Resources initialized successfully
}
#endif