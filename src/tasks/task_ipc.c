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
#include <string.h>
#include "main.h"
#include "task_console.h"

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
/* ADD CODE */
/* Look at task_ipc.h to find the list of helper functions */

/**
 * @brief 
 * 
 * @param sequence_num the sequence number of the packet to send
 * @return true if packet sucessfully transmits
 * @return false if packet is not received
 */
bool ipc_send_discovery(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_DISCOVERY,
        .sequence_num = sequence_num,
        //ADD INITIALIZATION DATA RELATED TO PAYLOAD BELOW
        //For this function, nothing else is needed
    };

    packet.checksum = calculate_checksum(&packet);

    if(xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    return true;
}

bool ipc_send_guess(uint16_t seq, uint8_t guess[4])
{
    ipc_packet_t pkt;

    pkt.start_byte   = IPC_PACKET_START;
    pkt.cmd          = IPC_CMD_GUESS;
    pkt.sequence_num = seq;

    // Copy the 4-digit guess into the payload
    memcpy(pkt.payload.guess, guess, 4);

    // Compute checksum (same method used elsewhere)
    pkt.checksum = calculate_checksum(&pkt);

    // Queue the packet for transmission
    if (xQueueSend(Queue_IPC_Tx, &pkt, 0) != pdTRUE)
    {
        return false;
    }

    return true;
}



/**
 * @brief Waits for an acknowledgement from an external board
 * 
 * @param timeout_ms how long to wait before returning a failure
 * @return true if an ack is received
 * @return false if an ack is not received
 */
bool ipc_wait_for_ack(uint32_t timeout_ms) {
    EventBits_t events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                             ECE353_EVENT_IPC_ACK_RECEIVED,
                                             pdTRUE,
                                             pdFALSE,
                                             pdMS_TO_TICKS(timeout_ms));
    return (events & ECE353_EVENT_IPC_ACK_RECEIVED) != 0;
}

/**
 * @brief Acknowledges reception of a packet
 * 
 * @param sequence_num the sequence number of the received packet
 * @return true if the ack is sucessfully sent
 * @return false if the ack fails to send
 */
bool ipc_send_ack(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_ACK,
        .sequence_num = sequence_num,
        //ADD INITIALIZATION DATA RELATED TO PALOAD BELOW
        //for this function, nothing else is needed
    };

    packet.checksum = calculate_checksum(&packet);

    if(xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    return true;
}

bool ipc_wait_for_guess(uint32_t timeout_ms, uint8_t guess_out[4]) {
    EventBits_t events;

    events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                 ECE353_EVENT_IPC_GUESS_RECEIVED,
                                 pdTRUE,
                                 pdFALSE,
                                 pdMS_TO_TICKS(timeout_ms));

    if ((events & ECE353_EVENT_IPC_GUESS_RECEIVED) == 0)
    {
        return false;
    }

    if (guess_out != NULL)
    {
        memcpy(guess_out, IPC_Last_Received_Guess, sizeof(IPC_Last_Received_Guess));
    }

    return true;
}

bool ipc_wait_for_feedback(uint32_t timeout_ms, uint8_t *exact_out, uint8_t *misplaced_out) {
    EventBits_t events;

    events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                 ECE353_EVENT_IPC_FEEDBACK_RECEIVED,
                                 pdTRUE,
                                 pdFALSE,
                                 pdMS_TO_TICKS(timeout_ms));

    if ((events & ECE353_EVENT_IPC_FEEDBACK_RECEIVED) == 0)
    {
        return false;
    }

    if (exact_out != NULL)
    {
        *exact_out = IPC_Last_Received_Feedback_Exact;
    }

    if (misplaced_out != NULL)
    {
        *misplaced_out = IPC_Last_Received_Feedback_Misplaced;
    }

    return true;
}

bool ipc_send_active_player(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_ACTIVE_PLAYER,
        .sequence_num = sequence_num,
        //ADD INITIALIZATION DATA RELATED TO PALOAD BELOW
        //for this function, nothing else is needed
    };

    packet.checksum = calculate_checksum(&packet);

    if(xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    return true;
}

bool ipc_send_inactive_player(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_INACTIVE_PLAYER,
        .sequence_num = sequence_num,
        //ADD INITIALIZATION DATA RELATED TO PALOAD BELOW
        //for this function, nothing else is needed
    };

    packet.checksum = calculate_checksum(&packet);

    if(xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    return true;
}

bool ipc_send_status(uint16_t sequence_num, ipc_status_t status) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_STATUS,
        .sequence_num = sequence_num,
        //ADD INITIALIZATION DATA RELATED TO PALOAD BELOW
        //for this function, nothing else is needed
        .payload.status = status,
    };

    packet.checksum = calculate_checksum(&packet);

    if(xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    return true;
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
        // Read the received character
        if (cyhal_uart_getc(&IPC_Uart_Obj, &c, 0) == CY_RSLT_SUCCESS) {
            if (raw_data_index == 0) { //if we are expecting the start byte
                if(c == IPC_PACKET_START) {
                    //store start byte
                    ((uint8_t*)IPC_Rx_Produce_Buffer)[0] = c;
                    raw_data_index = 1;
                } else { //not ready to start - ignore
                    //do nothing
                }
            } else { //we are in the middle of a packet
                //Store next byte
                ((uint8_t*)IPC_Rx_Produce_Buffer)[raw_data_index++] = c;
                if (raw_data_index >= sizeof(ipc_packet_t)){ //if we have finished the packet
                    //swap buffers
                    volatile ipc_packet_t* temp = IPC_Rx_Produce_Buffer;
                    IPC_Rx_Produce_Buffer = IPC_Rx_Consume_Buffer;
                    IPC_Rx_Consume_Buffer = temp;

                    //reset index
                    raw_data_index = 0;

                    //notify RX task
                    vTaskNotifyGiveFromISR(TaskHandle_IPC_Rx, &xHigherPriorityTaskWoken);
                }
            }
        }



        /* ADD CODE */
            //IN THIS FILE:
                //generate an interrupt every time data is received
                //use cyhal_uart_getc to receive data
                //make sure you are using UART for inter-processor communication and not any other UART
                //In interrupt handler: use double buffering
                    //Send task notification when entire packet has arrived (most likely to task_ipc_rx)
                    //Swap produce and consume buffer

        /* You will need to determine when a new packet is starting and then store the packet
         * byte by byte into the produce buffer.  Once all the bytes have been received, 
         * send a TaskNotification to the bottom half task to parse and process the packet.  
         * 
         * You will also need to toggle the produce and consume buffers
         *
         * The raw_data_index variable can be used to keep track of how many bytes have been received for the current packet.
         * When raw_data_index is 0, the next byte received should be the start byte
         */
        
    }
    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
    }
    else
    {
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
