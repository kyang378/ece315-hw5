/**
 * @file task_imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_imu.h"

 #if defined(ECE353_FREERTOS)
#include "imu.h"
#include "task_console.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t Queue_IMU_Request;

/*****************************************************************************/
/* Static Variables                                                          */
/*****************************************************************************/
static SemaphoreHandle_t *SPI_Semaphore = NULL;
static cyhal_spi_t *imu_spi_obj = NULL;
static cyhal_gpio_t imu_cs_pin = NC;

/**
 * @brief 
 * This function acts as the published interface to get data from the IMU.
 * X, Y, and Z data is returned via the imu_data parameter.
 * @param return_queue 
 * @param imu_data 
 * @return true 
 * @return false 
 */
bool system_sensors_get_imu(QueueHandle_t return_queue, int16_t imu_data[3])
{
    device_request_msg_t request;
    device_response_msg_t response;

    if(return_queue == NULL || imu_data == NULL)
    {
        return false;
    }

    request.device = DEVICE_IMU;
    request.operation = DEVICE_OP_READ;
    request.address = IMU_REG_OUTX_L_XL;
    request.value = 0;
    request.response_queue = return_queue;

    if(xQueueSend(Queue_IMU_Request, &request, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    if(xQueueReceive(return_queue, &response, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    if(response.device != DEVICE_IMU || response.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return false;
    }

    imu_data[0] = (int16_t)response.payload.imu[0];
    imu_data[1] = (int16_t)response.payload.imu[1];
    imu_data[2] = (int16_t)response.payload.imu[2];

    return true;
}

 void task_imu(void *arg)
 {
    (void) arg;
    device_request_msg_t request;
    device_response_msg_t response;

    int16_t accel_data[3];

    task_console_printf("Starting IMU Task\r\n");

    // Grab the SPI semaphore before accessing the IMU
    // because imu_init() will perform SPI transactions to configure the IMU
    if(xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY) != pdPASS)
    {
        CY_ASSERT(0);
    }

    /* Initialize the IMU */
    if(!imu_init(imu_spi_obj, imu_cs_pin))
    {
        task_console_printf("IMU init failed!\r\n");
        CY_ASSERT(0);
    }

    task_console_printf("IMU init successful\r\n");

    // Release the SPI semaphore after initializing the IMU
    xSemaphoreGive(*SPI_Semaphore);

    while(1) // QUESTION: do we still need a semaphore here if we have a queue?
    {
        /* Wait for a request to be available */
        xQueueReceive(Queue_IMU_Request, &request, portMAX_DELAY);

        response.device = DEVICE_IMU;
        response.status = DEVICE_OPERATION_STATUS_READ_FAILURE;

        if(request.operation == DEVICE_OP_READ)
        {
            vTaskDelay(pdMS_TO_TICKS(250));

            if(xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY) == pdPASS)
            {
                /* Get the IMU data */
                imu_read_registers(
                    imu_spi_obj,
                    imu_cs_pin,
                    (uint8_t)request.address,
                    (uint8_t *)accel_data,
                    sizeof(accel_data)
                );

                xSemaphoreGive(*SPI_Semaphore);

                response.payload.imu[0] = (uint16_t)accel_data[0];
                response.payload.imu[1] = (uint16_t)accel_data[1];
                response.payload.imu[2] = (uint16_t)accel_data[2];
                response.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;

                task_console_printf(
                    "Accel x: %d, Accel y: %d, Accel z: %d\r\n",
                    accel_data[0],
                    accel_data[1],
                    accel_data[2]
                );
            }
        }

        if(request.response_queue != NULL)
        {
            xQueueSend(request.response_queue, &response, portMAX_DELAY);
        }
    }
}

/**
  * @brief 
  * This function will create the IMU task for reading data from the IMU sensor.
  * It assumes that you have already created a semaphore for SPI access and initialized
  * the SPI peripheral.  This function does NOT initialize the SPI peripheral OR CS Pin 
  * because the SPI peripheral is shared between multiple tasks (e.g. IMU, EEPROM, etc.). 
  * @param spi_semaphore 
  * @return true 
  * @return false 
  */
 bool task_imu_resources_init(SemaphoreHandle_t *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
 {
    if(spi_semaphore == NULL || spi_obj == NULL || cs_pin == NC)
    {
        return false;
    }

    /* Save the SPI resources */
    SPI_Semaphore = spi_semaphore;
    imu_spi_obj = spi_obj;
    imu_cs_pin = cs_pin;

    /* Create the IMU Request Queue */
    Queue_IMU_Request = xQueueCreate(1, sizeof(device_request_msg_t));
    if(Queue_IMU_Request == NULL)
    {
        return false;
    }

    /* Create the IMU Task */
   if (xTaskCreate(
       task_imu, 
       "IMU", 
       10*configMINIMAL_STACK_SIZE, 
       spi_semaphore, 
       tskIDLE_PRIORITY + 1, 
       NULL) != pdPASS)
   {
       return false;
   }

   return true;
 }

#endif /* ECE353_FREERTOS */
