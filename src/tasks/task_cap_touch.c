/**
 * @file task_cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #include "task_cap_touch.h"

#if defined(ECE353_FREERTOS)
#include "rtos_events.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t        Queue_Request_Cap_Touch = NULL; //changed off of static for access elsewhere
static SemaphoreHandle_t    I2C_Semaphore = NULL;
cyhal_i2c_t         *I2C_Obj = NULL;                //changed off of static for access elsewhere
static cyhal_gpio_t        Cap_Touch_Int_Pin = NC;


//HELPER FUNCTIONS

bool system_sensors_get_cap_touch(QueueHandle_t return_queue, uint16_t *x, uint16_t *y)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    if (return_queue == NULL || x == NULL || y == NULL)
    {
        return false;
    }

    request_packet.device         = DEVICE_CAP_TOUCH;
    request_packet.operation      = DEVICE_OP_READ;
    request_packet.address        = 0;      // not used
    request_packet.value          = 0;      // not used
    request_packet.response_queue = return_queue;

    if (xQueueSend(Queue_Request_Cap_Touch, &request_packet, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    if (xQueueReceive(return_queue, &response_packet, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    if (response_packet.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return false;
    }

    *x = response_packet.payload.cap_touch[0];
    *y = response_packet.payload.cap_touch[1];

    return true;
}


static void task_cap_touch(void *param)
{
    device_request_msg_t  request_packet;
    device_response_msg_t response_packet;

    uint16_t x = 0;
    uint16_t y = 0;

    task_console_printf("Starting Cap Touch Task\r\n");

    while (1)
    {
        /* Wait for a request */
        xQueueReceive(Queue_Request_Cap_Touch, &request_packet, portMAX_DELAY);

        /* Validate request */
        if (request_packet.device != DEVICE_CAP_TOUCH)
        {
            task_console_printf("CAP_TOUCH: Invalid device type %d\r\n",
                                request_packet.device);
            continue;
        }

        if (request_packet.operation != DEVICE_OP_READ)
        {
            task_console_printf("CAP_TOUCH: Invalid operation %d\r\n",
                                request_packet.operation);
            continue;
        }

        /**********************************************************************
         * Perform the read operation
         **********************************************************************/
        xSemaphoreTake(I2C_Semaphore, portMAX_DELAY);

        bool ok = cap_touch_get_coordinates(&x, &y);

        xSemaphoreGive(I2C_Semaphore);

        /* Prepare response */
        response_packet.device = DEVICE_CAP_TOUCH;

        if (ok)
        {
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
            response_packet.payload.cap_touch[0] = x;
            response_packet.payload.cap_touch[1] = y;
        }
        else
        {
            response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
            response_packet.payload.cap_touch[0] = 0;
            response_packet.payload.cap_touch[1] = 0;
        }

        /* Send response back to requester */
        if (request_packet.response_queue != NULL)
        {
            xQueueSend(request_packet.response_queue,
                       &response_packet,
                       portMAX_DELAY);
        }
        else
        {
            task_console_printf("CAP_TOUCH: NULL response_queue for READ request\r\n");
        }
    }
}



bool task_cap_touch_resources_init(
    QueueHandle_t queue_request, 
    SemaphoreHandle_t i2c_semaphore, 
    cyhal_i2c_t *i2c_obj, 
    cyhal_gpio_t pin_cap_touch_int
)
{
    if(queue_request == NULL || i2c_semaphore == NULL || i2c_obj == NULL || pin_cap_touch_int == NC)
    {
        return false;
    }   

    /* Save the resources */
    Queue_Request_Cap_Touch = queue_request;
    I2C_Semaphore = i2c_semaphore;
    I2C_Obj = i2c_obj;
    Cap_Touch_Int_Pin = pin_cap_touch_int;

    /**************************************************************************
     * 1. Configure the INT pin from the FT6236
     *    - Input
     *    - Falling-edge interrupt (FT6236 pulls INT low when data ready)
     **************************************************************************/
    cy_rslt_t rslt;

    rslt = cyhal_gpio_init(
        Cap_Touch_Int_Pin,
        CYHAL_GPIO_DIR_INPUT,
        CYHAL_GPIO_DRIVE_NONE,
        true       // initial value ignored for input
    );

    if (rslt != CY_RSLT_SUCCESS)
    {
        task_console_printf("Cap Touch: Failed to init INT pin\r\n");
        return false;
    }

    /* Configure interrupt on falling edge */
    cyhal_gpio_enable_event(
        Cap_Touch_Int_Pin,
        CYHAL_GPIO_IRQ_FALL,
        CYHAL_ISR_PRIORITY_DEFAULT,
        true
    );

    /**************************************************************************
     * 2. Create the Cap Touch Task
     **************************************************************************/
    if (xTaskCreate(
            task_cap_touch,
            "Cap Touch Task",
            TASK_CAP_TOUCH_STACK_SIZE,
            NULL,
            TASK_CAP_TOUCH_PRIORITY,
            NULL) != pdPASS)
    {
        task_console_printf("Cap Touch: Failed to create task\r\n");
        return false;
    }

    //task_console_printf("Cap Touch Task   : Starting Cap Touch Task\r\n");

    return true;
}

#endif /* ECE353_FREERTOS */