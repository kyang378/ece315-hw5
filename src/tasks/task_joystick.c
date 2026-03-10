/**
 * @file task_joystick.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "main.h"

#ifdef ECE353_FREERTOS  
#include "drivers.h"
#include "task_joystick.h"

#if defined(HW02)
#include "devices.h"
#endif

QueueHandle_t Queue_Joystick = NULL;
joystick_position_t current_position = JOYSTICK_POS_CENTER; // initialized to center

#if defined(HW02)
static QueueHandle_t Queue_Request_Joystick_HW02 = NULL;
static joystick_position_t current_position_hw02 = JOYSTICK_POS_CENTER;

/*
    Helper function used by the HW02 joystick gatekeeper task.

    The homework behavior only cares about CENTER/LEFT/RIGHT/UP/DOWN.
    When the physical joystick is pushed hard into a corner, the driver can
    report one of the diagonal positions. For HW02 we treat those diagonal
    values as noise and just keep the previous valid cardinal direction.
*/
static joystick_position_t hw02_filter_joystick_position(
    joystick_position_t previous_position,
    joystick_position_t raw_position)
{
    switch (raw_position)
    {
        case JOYSTICK_POS_CENTER:
        case JOYSTICK_POS_LEFT:
        case JOYSTICK_POS_RIGHT:
        case JOYSTICK_POS_UP:
        case JOYSTICK_POS_DOWN:
            return raw_position;

        default:
            return previous_position;
    }
}
#endif

/* Message lookup table for joystick positions */
const char * const joystick_pos_names[] = {
    "Center",
    "Left",
    "Right",
    "Up",
    "Down",
    "Upper Left",
    "Upper Right",
    "Lower Left",
    "Lower Right"
};

 /**
  * @brief 
  *  Task used to monitor the joystick twice a second and print out to the user
  * @param arg 
  */
 void task_joystick(void *arg)
{
    (void)arg; // Unused parameter

    // size = 1, holds joystick_position_t data
    Queue_Joystick = xQueueCreate(1, sizeof(joystick_position_t));

    char msg[50]; // buffer to hold the position

    uint16_t x, y; // stores the raw 16-bit values

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); // sample the joystick every 500 ms

        x = joystick_read_x();
        y = joystick_read_y();

        joystick_position_t position = joystick_get_pos();
        if (position != current_position) {
            current_position = position; // update current position
            xQueueSend(Queue_Joystick, &position, 0); // send position to queue if changed
        }

    }
}

bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/

    /* Create the joystick task */
    
    return true;
}

#if defined(HW02)
void task_hw02_system_joystick(void *arg)
{
    device_request_msg_t joystick_request;
    device_response_msg_t joystick_response;
    joystick_position_t sampled_position;
    joystick_position_t raw_position;

    (void)arg; // Unused parameter

    if (Queue_Request_Joystick_HW02 == NULL)
    {
        CY_ASSERT(0);
    }

    while (1)
    {
        /* First sample the hardware, then filter the raw position so that
           diagonals do not create extra HW02 movement events which would 
           waste resources to resolve, even if there's such guardrails
           about illegal joystick movements in the system_control task
           */
        raw_position = joystick_get_pos();
        sampled_position = hw02_filter_joystick_position(current_position_hw02, raw_position);

        if (sampled_position != current_position_hw02)
        {
            // Any real change in joystick position wakes up the main HW02
            // system-control task through the event group
            current_position_hw02 = sampled_position; // update the current position and notify system control about the change
            xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_JOYSTICK_CHANGED);
        }

        /* The joystick task is also acting as a device gatekeeper.
           So after sampling the joystick, it checks whether system control
           asked for the current position through xQueue_Request_Joystick. */
        while (xQueueReceive(Queue_Request_Joystick_HW02, &joystick_request, 0) == pdPASS)
        {
            joystick_response.device = DEVICE_JOYSTICK;

            if ((joystick_request.device == DEVICE_JOYSTICK) &&
                (joystick_request.operation == DEVICE_OP_READ))
            {
                joystick_response.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
                joystick_response.payload.joystick = current_position_hw02;
            }
            else 
            {
                // we don't support any other operations for the joystick besides read
                joystick_response.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
            }

            if (joystick_request.response_queue != NULL)
            {
                /* Send the sampled-and-filtered joystick position back to the
                   task that asked for it, which for HW02 is system control. */
                xQueueSend(joystick_request.response_queue, &joystick_response, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_HW02_SYSTEM_JOYSTICK_POLL_MS));
    }
}

bool task_hw02_system_joystick_resources_init(QueueHandle_t queue_joystick)
{
    BaseType_t result;

    /* This binds the request queue created in hw02.c to the HW02 joystick
       gatekeeper task, so the task does not secretly create its own queue. */
    if (queue_joystick == NULL)
    {
        return false;
    }

    Queue_Request_Joystick_HW02 = queue_joystick;

    result = xTaskCreate(
        task_hw02_system_joystick,
        "task_hw02_system_joystick",
        TASK_HW02_SYSTEM_JOYSTICK_STACK_SIZE,
        NULL,
        TASK_HW02_SYSTEM_JOYSTICK_PRIORITY,
        NULL
    );

    return result == pdPASS;
}
#endif
#endif
