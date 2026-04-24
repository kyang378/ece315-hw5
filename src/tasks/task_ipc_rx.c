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
#include <stdbool.h>

#if defined(ECE353_FREERTOS)
#include "task_ipc.h"

/* Globals */
TaskHandle_t TaskHandle_IPC_Rx = NULL;

/* Use a double buffering strategy for IPC packets */
static volatile ipc_packet_t IPC_Rx_Buffer0;
static volatile ipc_packet_t IPC_Rx_Buffer1;

volatile ipc_packet_t* volatile IPC_Rx_Produce_Buffer = &IPC_Rx_Buffer0;
volatile ipc_packet_t* volatile IPC_Rx_Consume_Buffer = &IPC_Rx_Buffer1;


uint8_t IPC_Last_Received_Guess[4] = {0};
uint16_t IPC_Last_Received_Sequence = 0;


/**
 * @brief
 *
 * This task is used to process received IPC packets.  The task will block
 * on a FreeRTOS Task Notification.  When a notification is received,
 * the task will process the IPC packet stored in the consume buffer.
 *
 * For validation purposes, the task will print out the contents of the
 * received IPC packet to the console.
 * 
 * @param arg
 * Unused parameter
 */
void task_ipc_rx(void *param)
{
    while(1)
    {
        // Wait for a FreeRTOS Task Notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if(validate_packet((ipc_packet_t *)IPC_Rx_Consume_Buffer) == true) 
        {
            /* ADD CODE */
            // Process the received IPC packet
            switch (IPC_Rx_Consume_Buffer->cmd) {
                case IPC_CMD_DISCOVERY:
                {
                    printf("Received discovery message. \n\r");
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_IPC_DISCOVERY_RECEIVED);
                    break;
                }
                case IPC_CMD_ACK:
                {
                    //set the event
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_IPC_ACK_RECEIVED);
                    break;
                }
                case IPC_CMD_ACTIVE_PLAYER:
                {
                    printf("Received active player message \n\r");
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);

                    break;
                }
                case IPC_CMD_INACTIVE_PLAYER:
                {
                    printf("Received inactive player message \n\r");
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    break;
                }
                case IPC_CMD_STATUS:
                {
                    ipc_status_t status = IPC_Rx_Consume_Buffer->payload.status;

                    switch(status)
                    {
                        case IPC_STATUS_OK:
                            printf("Received status: OK\n\r");
                            xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_IPC_STATUS_RECEIVED);
                            break;

                        case IPC_STATUS_CRC_FAIL:
                            printf("Received status: CRC FAIL\n\r");
                            break;

                        case IPC_STATUS_INVALID_MSG_TYPE:
                            printf("Received status: INVALID MESSAGE TYPE\n\r");
                            break;

                        default:
                            printf("Received status: UNKNOWN (0x%02X)\n\r", status);
                            break;

                        
                    }
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    break;
                }
                case IPC_CMD_GUESS:
                {
                    // Copy the guess out of the consume buffer
                    memcpy(IPC_Last_Received_Guess,
                        (const void *)IPC_Rx_Consume_Buffer->payload.guess,
                        4);

                    // Save sequence number for debugging or ACK tracking
                    IPC_Last_Received_Sequence = IPC_Rx_Consume_Buffer->sequence_num;

                    // Signal the system control task
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_IPC_GUESS_RECEIVED);

                    // Always ACK
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);

                    break;
                }


                default:
                {
                    printf("Received unknown/unsupported message type. \n\r");
                    break;
                }
            }
        }
        else {
            printf("Invalid IPC packet received!\n\r");
        }
    }
}

bool task_ipc_resources_init_rx(void)
{
    // Create the IPC Rx Task
    BaseType_t task_ipc_rx_status = xTaskCreate(
        task_ipc_rx,                 // Function that implements the task.
        "IPC Rx Task",               // Text name for the task.
        IPC_STACK_SIZE,             // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        IPC_PRIORITY,               // Priority at which the task is created.
        &TaskHandle_IPC_Rx          // Used to pass out the created task's handle.
    );

    if(task_ipc_rx_status != pdPASS)
    {
        return false;
    }

    return true;    
}

#endif
