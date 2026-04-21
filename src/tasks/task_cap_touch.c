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

/* Static Function Declarations                                              */
/*****************************************************************************/
static void task_cap_touch(void *arg);
static bool task_cap_touch_verify_device(void);
static bool task_cap_touch_configure_device(void);
static bool task_cap_touch_startup(bool *device_detected);
static void task_cap_touch_map_coordinates(uint16_t raw_x, uint16_t raw_y, uint16_t *sensor0, uint16_t *sensor1);

#define CAP_TOUCH_STARTUP_ATTEMPTS  10u
#define CAP_TOUCH_STARTUP_DELAY_MS  50u

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t Queue_Request_Cap_Touch = NULL;
static SemaphoreHandle_t I2C_Semaphore = NULL;
static cyhal_i2c_t *I2C_Obj = NULL;
static cyhal_gpio_t Cap_Touch_Int_Pin = NC;

/*****************************************************************************/
/* Static Function Definitions                                               */
/*****************************************************************************/
static bool task_cap_touch_verify_device(void)
{
    uint8_t focaltech_id = 0;

    if (cap_touch_read_u8(I2C_Obj, FT6X06_REG_FOCALTECH_ID, &focaltech_id) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    return (focaltech_id == FT6X06_FOCALTECH_ID);
}

static bool task_cap_touch_configure_device(void)
{
    // Keep the controller in its normal operating mode so touch data is
    // available in the working-mode register map.
    if (cap_touch_write_u8(I2C_Obj, FT6X06_REG_DEV_MODE, FT6X06_DEVICE_MODE_WORKING) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    // HW04 polls for touch data only when the CLI command arrives, so the
    // controller is configured for polling instead of interrupt-trigger mode.
    if (cap_touch_write_u8(I2C_Obj, FT6X06_REG_G_MODE, FT6X06_G_MODE_POLLING) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    return true;
}

static bool task_cap_touch_startup(bool *device_detected)
{
    uint32_t attempt;

    if (device_detected != NULL)
    {
        *device_detected = false;
    }

    for (attempt = 0; attempt < CAP_TOUCH_STARTUP_ATTEMPTS; attempt++)
    {
        if (xSemaphoreTake(I2C_Semaphore, portMAX_DELAY) != pdPASS)
        {
            CY_ASSERT(0);
        }

        // Warm resets can leave the FT6236 unavailable briefly, so startup
        // retries both detection and configuration before declaring failure.
        if (task_cap_touch_verify_device())
        {
            if (device_detected != NULL)
            {
                *device_detected = true;
            }

            if (task_cap_touch_configure_device())
            {
                xSemaphoreGive(I2C_Semaphore);
                return true;
            }
        }

        xSemaphoreGive(I2C_Semaphore);
        vTaskDelay(pdMS_TO_TICKS(CAP_TOUCH_STARTUP_DELAY_MS));
    }

    return false;
}

static void task_cap_touch_map_coordinates(uint16_t raw_x, uint16_t raw_y, uint16_t *sensor0, uint16_t *sensor1)
{
    // The FT6236 coordinate frame is rotated relative to the LCD orientation
    // used by the homework screenshots, so remap the raw values before
    // returning them to the console task.
    uint16_t mapped_sensor0 = raw_y;
    uint16_t mapped_sensor1 = 0;

    if (raw_x < LCD_COLS)
    {
        mapped_sensor1 = (LCD_COLS - 1u) - raw_x;
    }

    if (mapped_sensor0 >= LCD_ROWS)
    {
        mapped_sensor0 = LCD_ROWS - 1u;
    }

    if (sensor0 != NULL)
    {
        *sensor0 = mapped_sensor0;
    }

    if (sensor1 != NULL)
    {
        *sensor1 = mapped_sensor1;
    }
}

static void task_cap_touch(void *arg)
{
    (void)arg;
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    task_console_printf("Starting Cap Touch Task\r\n");

    while (!task_cap_touch_startup(NULL))
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (1)
    {
        uint8_t num_points = 0;
        uint16_t x = 0;
        uint16_t y = 0;

        xQueueReceive(Queue_Request_Cap_Touch, &request_packet, portMAX_DELAY);

        response_packet.device = DEVICE_CAP_TOUCH;
        response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
        response_packet.payload.cap_touch[0] = 0;
        response_packet.payload.cap_touch[1] = 0;

        if ((request_packet.device == DEVICE_CAP_TOUCH) &&
            (request_packet.operation == DEVICE_OP_READ))
        {
            if (xSemaphoreTake(I2C_Semaphore, portMAX_DELAY) == pdPASS)
            {
                // The gatekeeper owns all FT6236 traffic so the shared I2C bus
                // is accessed atomically with respect to other sensor tasks.
                num_points = cap_touch_get_num_points(I2C_Obj);

                if ((num_points > 0) && (num_points <= 2) &&
                    cap_touch_get_position(I2C_Obj, &x, &y))
                {
                    response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
                    task_cap_touch_map_coordinates(
                        x,
                        y,
                        &response_packet.payload.cap_touch[0],
                        &response_packet.payload.cap_touch[1]);
                }

                xSemaphoreGive(I2C_Semaphore);
            }
        }

        if (request_packet.response_queue != NULL)
        {
            xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
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
    if(queue_request == NULL || i2c_semaphore == NULL || i2c_obj == NULL)
    {
        return false;
    }   

    /* Save the resources */
    Queue_Request_Cap_Touch = queue_request;
    I2C_Semaphore = i2c_semaphore;
    I2C_Obj = i2c_obj;
    Cap_Touch_Int_Pin = pin_cap_touch_int;

    if (Cap_Touch_Int_Pin != NC)
    {
        cyhal_gpio_init(Cap_Touch_Int_Pin, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    }

    if (xTaskCreate(
        task_cap_touch,
        "Cap Touch Task",
        TASK_CAP_TOUCH_STACK_SIZE,
        NULL,
        TASK_CAP_TOUCH_PRIORITY,
        NULL) != pdPASS)
    {
        return false;
    }

    return true;
}
#endif /* ECE353_FREERTOS */
