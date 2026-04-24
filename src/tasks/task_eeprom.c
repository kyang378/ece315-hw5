/**
 * @file task_eeprom.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-17
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
 #include "task_eeprom.h"
 #include "task_console.h"

 /* Global Variables */
QueueHandle_t Queue_EEPROM_Requests;
static SemaphoreHandle_t *SPI_Semaphore = NULL;
static cyhal_spi_t *eeprom_spi_obj = NULL;
static cyhal_gpio_t eeprom_cs_pin = NC;

/**
 * @brief 
 *  This function acts as the published interface to write data to the EEPROM.
 *  It will format the device request message and send it to the EEPROM task.
 *
 *  If the user provides a valid return queue, this function will wait for
 *  the response from the EEPROM task before returning.  If the user provides
 *  a NULL return queue, this function will return immediately after sending
 *  the request.
 * @param return_queue 
 * @param address 
 * @param data 
 * @param length 
 * @return true 
 * @return false 
 */
bool system_sensors_eeprom_write(
    QueueHandle_t return_queue,
    uint16_t address,
    uint8_t data)
{
    device_request_msg_t request;
    device_response_msg_t response;

    // Fill out the request packet
    request.device = DEVICE_EEPROM;
    request.operation = DEVICE_OP_WRITE;
    request.address = address;
    request.value = data;
    request.response_queue = return_queue;

    // Send the request to the EEPROM task
    if (xQueueSend(
            Queue_EEPROM_Requests,
            &request,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    // If caller does NOT want a response, we are done
    if (return_queue == NULL)
    {
        return true;
    }

    // Caller wants confirmation — wait for the response
    if (xQueueReceive(
            return_queue,
            &response,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    // Check if the write succeeded
    return (response.status == DEVICE_OPERATION_STATUS_WRITE_SUCCESS);
}   

/**
 * @brief 
 * This function is the published interface to read data from the EEPROM.
 * It will format the device request message and send it to the EEPROM task.
 *
 * The value read from the EEPROM will be returned via the data pointer
 *
 * @param return_queue 
 * @param address 
 * @param data 
 * @return true 
 * @return false 
 */
bool system_sensors_eeprom_read(
    QueueHandle_t return_queue,
    uint16_t address,
    uint8_t *data)
{
    device_request_msg_t request;
    device_response_msg_t response;

    // Validate inputs
    if (return_queue == NULL || data == NULL)
    {
        return false;
    }

    // Fill out the request packet
    request.device = DEVICE_EEPROM;
    request.operation = DEVICE_OP_READ;
    request.address = address;
    request.value = 0;                 // Not used for reads
    request.response_queue = return_queue;

    // Send the request to the EEPROM task
    if (xQueueSend(
            Queue_EEPROM_Requests,
            &request,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    // Wait for the EEPROM task to respond
    if (xQueueReceive(
            return_queue,
            &response,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    // Check if the read succeeded
    if (response.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return false;
    }

    // Extract the EEPROM byte
    *data = response.payload.eeprom;

    return true;
}

/**
 * @brief 
 *  Task used to monitor the reception of command packets sent the EEPROM
 * @param arg 
 */
void task_eeprom(void *arg)
{
    (void) arg;

    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    task_console_printf("Starting EEPROM Task\r\n");

    while(1)
    {
        

        // Wait for a request from any task
        xQueueReceive(
            Queue_EEPROM_Requests,
            &request_packet,
            portMAX_DELAY
        );

        // Prepare the response header
        response_packet.device = DEVICE_EEPROM;

        // Claim exclusive access to the SPI bus
        xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);

        if(request_packet.operation == DEVICE_OP_WRITE)
        {
            // Perform the write
            eeprom_write_byte(
                eeprom_spi_obj,
                eeprom_cs_pin,
                request_packet.address,
                request_packet.value
            ); 

            response_packet.status =
                DEVICE_OPERATION_STATUS_WRITE_SUCCESS;
        }
        else if(request_packet.operation == DEVICE_OP_READ)
        {
            // Perform the read
            uint8_t val = eeprom_read_byte(
                eeprom_spi_obj,
                eeprom_cs_pin,
                request_packet.address
            );

            response_packet.payload.eeprom = val;
            response_packet.status =
                DEVICE_OPERATION_STATUS_READ_SUCCESS;
        }
        else
        {
            // Should never happen, but safe to handle
            response_packet.status =
                DEVICE_OPERATION_STATUS_READ_FAILURE;
        }

        // Release the SPI bus

        xSemaphoreGive(*SPI_Semaphore);

        // Send response back if caller provided a queue
        if(request_packet.response_queue != NULL)
        {
            xQueueSend(
                request_packet.response_queue,
                &response_packet,
                portMAX_DELAY
            );
        }
    }
}


/**
 * @brief 
 * Function used to initialize resources for the EEPROM task
 * @param spi_semaphore 
 * @param spi_obj 
 * @param cs_pin 
 * @return true 
 * @return false 
 */
bool task_eeprom_resources_init(SemaphoreHandle_t *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
 {
    if(spi_semaphore == NULL || spi_obj == NULL || cs_pin == NC)
    {
        return false;
    }

    // Save handles for access to the SPI peripheral and semaphore
    SPI_Semaphore = spi_semaphore;
    eeprom_spi_obj = spi_obj;
    eeprom_cs_pin = cs_pin;

    /*Create the EEPROM Requests Queue */
    Queue_EEPROM_Requests = xQueueCreate(10, sizeof(device_request_msg_t ));

    /* Create the FreeRTOS task for the EEPROM */
    if (xTaskCreate(
        task_eeprom, 
        "EEPROM", 
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