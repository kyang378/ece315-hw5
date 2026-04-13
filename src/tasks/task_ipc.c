/**
 * @file task_ipc.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "task_ipc.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"
#include "cyhal_uart.h"
#include "main.h"

#if defined(ECE353_FREERTOS)

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_uart_t IPC_Uart_Obj;
cyhal_uart_cfg_t IPC_Uart_Config =
{
    .data_bits = 8,
    .stop_bits = 1,
    .parity = CYHAL_UART_PARITY_NONE,
    .rx_buffer = NULL,
    .rx_buffer_size = 0
};

uint32_t IPC_Actual_Baud;

/**
 * @brief 
 * Simple checksum calculation function.  Take the XOR of all bytes
 * except the start and checksum bytes.
 * @param packet 
 * @return __inline 
 */
static __inline uint8_t calculate_checksum(ipc_packet_t *packet)
{
    uint8_t checksum = 0;
    for(int i = 1; i < sizeof(ipc_packet_t) - 1; i++)
    {
        checksum ^= ((uint8_t*)packet)[i];
    }
    return checksum;
}   

/**
 * @brief 
 * Validates the given IPC packet by checking the start byte and checksum
 * @param packet 
 * @return __inline 
 */
bool validate_packet(ipc_packet_t *packet)
{
    uint8_t checksum = 0;

    // Check that the packet pointer is valid
    if(packet == NULL)
    {
        return false;
    }


    // Check for the start byte
    if(packet->start_byte != IPC_PACKET_START)
    {
        return false;
    }

    // Calculate the checksum
    checksum = calculate_checksum(packet);

    // Validate the checksum
    return (checksum == packet->checksum);
}

/********************************************************************/
/* Helper Functions for sending IPC packets                         */
/********************************************************************/

// the ultimate helper that constructs the packet and sends it to the IPC Tx task via the FreeRTOS Queue. The other helper functions just call this one with the appropriate command and payload.
static bool ipc_send_packet(ipc_cmd_t cmd, uint16_t sequence_num, ipc_status_t status)
{
    ipc_packet_t packet = {0};

    if(Queue_IPC_Tx == NULL)
    {
        return false;
    }

    packet.start_byte = IPC_PACKET_START;
    packet.cmd = cmd;
    packet.sequence_num = sequence_num;
    packet.payload.status = status;
    packet.checksum = calculate_checksum(&packet);

    if((cmd != IPC_CMD_ACK) && (ECE353_RTOS_Events != NULL))
    {
        xEventGroupClearBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED);
    }

    return (xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) == pdPASS);
}

bool ipc_send_discovery(uint16_t sequence_num)
{
    return ipc_send_packet(IPC_CMD_DISCOVERY, sequence_num, IPC_STATUS_OK);
}

bool ipc_send_active_player(uint16_t sequence_num)
{
    return ipc_send_packet(IPC_CMD_ACTIVE_PLAYER, sequence_num, IPC_STATUS_OK);
}

bool ipc_send_inactive_player(uint16_t sequence_num)
{
    return ipc_send_packet(IPC_CMD_INACTIVE_PLAYER, sequence_num, IPC_STATUS_OK);
}

bool ipc_send_status(uint16_t sequence_num, ipc_status_t status)
{
    return ipc_send_packet(IPC_CMD_STATUS, sequence_num, status);
}

bool ipc_send_ack(uint16_t sequence_num)
{
    return ipc_send_packet(IPC_CMD_ACK, sequence_num, IPC_STATUS_OK);
}

bool ipc_wait_for_ack(uint32_t timeout_ms)
{
    TickType_t timeout_ticks;

    if(ECE353_RTOS_Events == NULL)
    {
        return false;
    }

    timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if((timeout_ms > 0U) && (timeout_ticks == 0U))
    {
        timeout_ticks = 1;
    }

    EventBits_t events = xEventGroupWaitBits(
        ECE353_RTOS_Events,
        ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED,
        pdTRUE,
        pdFALSE,
        timeout_ticks
    );

    return (events & ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED) != 0;
}






/**
 * @brief
 * Interrupt handler for the IPC UART. This function handles both RX and TX interrupts.
 *
 * @param handler_arg Pointer to handler arguments (not used).
 * @param event The UART event that triggered the interrupt.
 */
void ipc_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char c;
    static uint8_t raw_data_index = 0;

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        // Read in the received character from the hardware rx FIFO
        cyhal_uart_getc(&IPC_Uart_Obj, (uint8_t*)&c, 0);

        /* You will need to determine when a new packet is starting and then store the packet
         * byte by byte into the produce buffer.  Once all the bytes have been received, 
         * send a TaskNotification to the bottom half task to parse and process the packet.  
         * 
         * You will also need to toggle the produce and consume buffers
         *
         * The raw_data_index variable can be used to keep track of how many bytes have been received for the current packet.
         * When raw_data_index is 0, the next byte received should be the start byte
         */
        if(raw_data_index == 0)
        {
            if(c == IPC_PACKET_START)
            {
                ((uint8_t*)IPC_Rx_Produce_Buffer)[raw_data_index] = c;
                raw_data_index++;
            }
        }
        else
        {
            ((uint8_t*)IPC_Rx_Produce_Buffer)[raw_data_index] = c;
            raw_data_index++;

            if(raw_data_index >= sizeof(ipc_packet_t))
            {
                // we have received a full packet, so we can swap buffers and send a task notification to the bottom half task to process the packet
                volatile ipc_packet_t *temp = IPC_Rx_Produce_Buffer;
                IPC_Rx_Produce_Buffer = IPC_Rx_Consume_Buffer;
                IPC_Rx_Consume_Buffer = temp;

                IPC_Rx_Produce_Buffer->start_byte = 0; // reset start byte for next packet

                xTaskNotifyFromISR(TaskHandle_IPC_Rx, 0, eNoAction, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // yield to bottom half task
                
                // lastly, reset raw_data_index for the next packet
                raw_data_index = 0;
            }
        }
    }
    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
    }
    else
    {
    }
}

bool task_ipc_init(void)
{
    cy_rslt_t rslt;

    // Initialize the IPC UART
   rslt =  cyhal_uart_init(
        &IPC_Uart_Obj, 
        PIN_IPC_TX, 
        PIN_IPC_RX, 
        NC, 
        NC, 
        NULL, 
        &IPC_Uart_Config
    );
    if (rslt != CY_RSLT_SUCCESS)
    {
        return false; // Initialization failed
    }

    rslt = cyhal_uart_set_baud(&IPC_Uart_Obj, 115200, &IPC_Actual_Baud);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return false; // Initialization failed
    }

    cyhal_uart_clear(&IPC_Uart_Obj);

    // Register the UART handler
    cyhal_uart_register_callback(&IPC_Uart_Obj, ipc_event_handler, NULL);

    // Enable Rx Interrupts
    cyhal_uart_enable_event(
        &IPC_Uart_Obj,
        CYHAL_UART_IRQ_RX_NOT_EMPTY,
        3,
        true
    );


    if(task_ipc_resources_init_rx() == false)
    {
        return false; // Initialization failed
    }

    if(task_ipc_resources_init_tx() == false)
    {
        return false; // Initialization failed
    }

    return true; // Initialization successful
}
#endif  
