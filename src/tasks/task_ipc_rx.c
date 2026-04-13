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

static void task_ipc_print_packet(volatile ipc_packet_t *packet)
{
    printf("Received IPC Packet:\n\r");
    printf("Start Byte: 0x%02X\n\r", (unsigned int)packet->start_byte);
    printf("Command: 0x%02X\n\r", (unsigned int)packet->cmd);
    printf("Sequence Number: %u\n\r", (unsigned int)packet->sequence_num);

    if(packet->cmd == IPC_CMD_STATUS)
    {
        printf("Payload: 0x%02X\n\r", (unsigned int)packet->payload.status);
    }
    else
    {
        printf("Payload: unused\n\r");
    }

    printf("Checksum: 0x%02X\n\r", (unsigned int)packet->checksum);
}

/* Globals */
TaskHandle_t TaskHandle_IPC_Rx = NULL;

/* Use a double buffering strategy for IPC packets */
static volatile ipc_packet_t IPC_Rx_Buffer0;
static volatile ipc_packet_t IPC_Rx_Buffer1;

volatile ipc_packet_t *volatile IPC_Rx_Produce_Buffer = &IPC_Rx_Buffer0;
volatile ipc_packet_t *volatile IPC_Rx_Consume_Buffer = &IPC_Rx_Buffer1;

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
    (void)param;
    bool send_ack;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if(validate_packet((ipc_packet_t *)IPC_Rx_Consume_Buffer) == true)
        {
            send_ack = true;

            task_ipc_print_packet(IPC_Rx_Consume_Buffer);

            switch(IPC_Rx_Consume_Buffer->cmd)
            {
                case IPC_CMD_DISCOVERY:
                {
                    printf("Operation: discovery command received.\n\r");

                    if(ECE353_RTOS_Events != NULL)
                    {
                        xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_DISCOVERY_RECEIVED);
                    }

                    break;
                }

                case IPC_CMD_ACTIVE_PLAYER:
                {
                    printf("Operation: active-player command received.\n\r");
                    break;
                }

                case IPC_CMD_INACTIVE_PLAYER:
                {
                    printf("Operation: inactive-player command received.\n\r");
                    break;
                }

                case IPC_CMD_STATUS:
                {
                    printf("Operation: status command received.\n\r");
                    break;
                }

                case IPC_CMD_ACK:
                {
                    printf("Operation: acknowledgement received.\n\r");

                    if(ECE353_RTOS_Events != NULL)
                    {
                        xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED);
                    }

                    send_ack = false;
                    break;
                }

                default:
                {
                    printf("Operation: unsupported command.\n\r\n\r");
                    send_ack = false;
                    break;
                }
            }

            if(send_ack)
            {
                if(ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num))
                {
                    printf("ACK sent for sequence %u.\n\r\n\r", (unsigned int)IPC_Rx_Consume_Buffer->sequence_num);
                }
                else
                {
                    printf("Failed to send ACK for sequence %u.\n\r\n\r", (unsigned int)IPC_Rx_Consume_Buffer->sequence_num);
                }
            }
            else
            {
                printf("\n\r");
            }
        }
        else
        {
            printf("Invalid IPC packet received!\n\r");
        }
    }
}

bool task_ipc_resources_init_rx(void)
{
    BaseType_t task_ipc_rx_status = xTaskCreate(
        task_ipc_rx,
        "IPC Rx Task",
        IPC_STACK_SIZE,
        NULL,
        IPC_PRIORITY,
        &TaskHandle_IPC_Rx
    );

    if(task_ipc_rx_status != pdPASS)
    {
        return false;
    }

    return true;
}

#endif
