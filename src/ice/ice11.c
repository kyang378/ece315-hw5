/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE11)
#include "drivers.h"
#include "task_buttons.h"
#include "task_ipc.h"
#include "rtos_events.h"

char APP_DESCRIPTION[] = "ECE353: ICE 11 - FreeRTOS IPC Rx/Tx";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/* This function will be used to discover other board. This function should not
 * return until the discovery is complete.  The discovery is complete when we receive
 * a discovery message from the other board OR we send a discovery message that is 
 * Acked by the other board */
void discover_board(uint16_t *sequence_num)
{
    bool discovery_complete = false;
    uint16_t current_sequence;
    EventBits_t events;

    while(discovery_complete == false)
    {
        current_sequence = *sequence_num;

        if(ECE353_RTOS_Events != NULL)
        {
            xEventGroupClearBits(
                ECE353_RTOS_Events,
                ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RECEIVED
            );
        }

        // Send a discovery message
        ipc_send_discovery(current_sequence);

        events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RECEIVED,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(IPC_ACK_TIMEOUT_MS)
        );

        // the discovery is complete when either we send a discovery message that is ACKed
        // by the other board...
        if((events & ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED) != 0)
        {
            printf("Received ACK for sequence number: %u!\n\r\n\r", (unsigned int)current_sequence);
            discovery_complete = true;
        }
        // ...or we receive a discovery message from the other board
        else if((events & ECE353_RTOS_EVENTS_IPC_DISCOVERY_RECEIVED) != 0)
        {
            printf("Discovery message received from other board!\n\r\n\r");
            discovery_complete = true;
        }
        else
        {
            printf("Discovery timeout for sequence number: %u. Retrying...\n\r", (unsigned int)current_sequence);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

    }
}

/**
 * @brief
 * This task will be used to verify the functionality of the IPC UART specification
 * by ....
 *
 * @param arg 
 * Unused parameter
 */
void task_system_control(void *arg)
{
    (void)arg; // Unused parameter
    EventBits_t events;
    bool ack_received;
    uint16_t sequence_num = 0;
    uint16_t current_sequence = 0;
    ipc_status_t status_code = IPC_STATUS_CRC_FAIL;

    /* Begin the discovery process. */
    discover_board(&sequence_num);

    while(1)
    {
        // Wait for SW1, SW2,or SW3 to be pressed.  If you have not gotten task_buttons.c working yet, 
        // you will need to do so before you can proceed with this task. 
        events = xEventGroupWaitBits
        (
            ECE353_RTOS_Events,
            ECE353_EVENT_BUTTON_SW1_PRESSED | ECE353_EVENT_BUTTON_SW2_PRESSED | ECE353_EVENT_BUTTON_SW3_PRESSED,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

        if(events & ECE353_EVENT_BUTTON_SW1_PRESSED)
        {
            /* Send the active player message  */
            current_sequence = sequence_num;
            if(ipc_send_active_player(current_sequence) == false)
            {
                printf("Failed to send active player packet.\n\r\n\r");
                continue;
            }

            sequence_num++;

            /* Wait for the ack */
            ack_received = ipc_wait_for_ack(IPC_ACK_TIMEOUT_MS);

            /* Print out a message indicating if the ACK was received */
            if(ack_received)
            {
                printf("ACK received for active player sequence %u.\n\r\n\r", (unsigned int)current_sequence);
            }
            else
            {
                printf("Timed out waiting for ACK on active player sequence %u.\n\r\n\r", (unsigned int)current_sequence);
            }
        }
        else if(events & ECE353_EVENT_BUTTON_SW2_PRESSED)
        {
            /* Send the inactive player message  */
            current_sequence = sequence_num;
            if(ipc_send_inactive_player(current_sequence) == false)
            {
                printf("Failed to send inactive player packet.\n\r\n\r");
                continue;
            }

            sequence_num++;

            /* Wait for the ack */
            ack_received = ipc_wait_for_ack(IPC_ACK_TIMEOUT_MS);

            /* Print out a message indicating if the ACK was received */
            if(ack_received)
            {
                printf("ACK received for inactive player sequence %u.\n\r\n\r", (unsigned int)current_sequence);
            }
            else
            {
                printf("Timed out waiting for ACK on inactive player sequence %u.\n\r\n\r", (unsigned int)current_sequence);
            }
        }
        else if(events & ECE353_EVENT_BUTTON_SW3_PRESSED)
        {
            /* Send the status message with an error code  */
            current_sequence = sequence_num;
            if(ipc_send_status(current_sequence, status_code) == false)
            {
                printf("Failed to send status packet.\n\r\n\r");
                continue;
            }

            sequence_num++;

            /* Wait for the ack */
            ack_received = ipc_wait_for_ack(IPC_ACK_TIMEOUT_MS);

            /* Print out a message indicating if the ACK was received */
            if(ack_received)
            {
                printf("ACK received for status sequence %u.\n\r\n\r", (unsigned int)current_sequence);
            }
            else
            {
                printf("Timed out waiting for ACK on status sequence %u.\n\r\n\r", (unsigned int)current_sequence);
            }

            if(status_code == IPC_STATUS_CRC_FAIL)
            {
                status_code = IPC_STATUS_INVALID_MSG_TYPE;
            }
            else
            {
                status_code = IPC_STATUS_CRC_FAIL;
            }
        }
        else
        {
             printf("Unknown Event!\n\r\n\r");
        }
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    // Initialize the LEDs
    rslt = leds_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("LED initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();
    if(ECE353_RTOS_Events == NULL)
    {
        printf("Event group initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_button_init())
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create the System Control Task
    xTaskCreate(
        task_system_control, 
        "System Control", 
        configMINIMAL_STACK_SIZE*5,      
        NULL, 
        2, 
        NULL
    );

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif
