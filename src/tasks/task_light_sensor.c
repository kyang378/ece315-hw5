/**
 * @file task_light_sensor.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "task_light_sensor.h"
#include "task_console.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
/* I2C Object Handle */
static cyhal_i2c_t *I2C_Obj;

/* I2C Semaphore */
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to light sensor */
QueueHandle_t Queue_Light_Sensor_Requests;


/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/
/**
 * @brief
 * Set ALS MODE to Active and initiate a software reset
 */
static void ltr_light_sensor_start(void)
{
    uint8_t value;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

    /* Enable ALS mode + software reset */
    value = LTR_REG_CONTR_SW_RESET ;
    i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, value);

    value = LTR_REG_CONTR_ALS_MODE;
    i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, value);

    xSemaphoreGive(*I2C_Semaphore);
}




static uint8_t ltr_light_get_contr(void)
{
    uint8_t value = 0;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, &value);
    xSemaphoreGive(*I2C_Semaphore);

    return value;
}


static uint8_t ltr_light_sensor_status(void)
{
    uint8_t value = 0;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_STATUS, &value);
    xSemaphoreGive(*I2C_Semaphore);

    return value;
}


/**
 * @brief
 * Returns the part ID of the LTR_329ALS-01
 * @return uint8_t
 */
static uint8_t ltr_light_sensor_part_id(void)
{
    uint8_t value = 0;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_PART_ID, &value);
    xSemaphoreGive(*I2C_Semaphore);

    return value;
}


static uint8_t ltr_light_sensor_manufac_id(void)
{
    uint8_t value = 0;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_MANUFAC_ID, &value);
    xSemaphoreGive(*I2C_Semaphore);

    return value;
}


static uint16_t ltr_light_sensor_get_ch0(void)
{
    uint8_t msbyte = 0;
    uint8_t lsbyte = 0;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

    /* Read MSB first */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_1, &msbyte);

    /* Then read LSB */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_0, &lsbyte);

    xSemaphoreGive(*I2C_Semaphore);

    return (uint16_t)(msbyte << 8) | lsbyte;
}

static uint16_t ltr_light_sensor_get_ch1(void)
{
    uint8_t msbyte = 0;
    uint8_t lsbyte = 0;

    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

    /* Read MSB first */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_1, &msbyte);

    /* Then read LSB */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_0, &lsbyte);

    xSemaphoreGive(*I2C_Semaphore);

    return (uint16_t)(msbyte << 8) | lsbyte;
}


static void ltr_light_sensor_get_readings(uint16_t *ch1, uint16_t *ch0)
{
    uint8_t status = 0;

    status = ltr_light_sensor_status();
    // while((status & LTR_REG_STATUS_NEW_DATA) != LTR_REG_STATUS_NEW_DATA)
    // {
    //     // Wait
    //     status = ltr_light_sensor_status();
    // }

    *ch1 = ltr_light_sensor_get_ch1();
    *ch0 = ltr_light_sensor_get_ch0();
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/
/*
* @brief 
 * This is a helper function that is called by tasks other than the light sensor task
 * when they want to read the ambient light from the sensor.
*/
bool system_sensors_get_light(QueueHandle_t return_queue, uint16_t *ambient_light)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    if (return_queue == NULL || ambient_light == NULL)
    {
        return false;
    }

    /* Fill out the request packet */
    request_packet.device = DEVICE_LIGHT;
    request_packet.operation = DEVICE_OP_READ;
    request_packet.address = 0;          // Not used for light sensor
    request_packet.value = 0;            // Not used for light sensor
    request_packet.response_queue = return_queue;

    /* Send request to the Light Sensor Task */
    xQueueSend(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY);

    /* Wait for the response */
    xQueueReceive(return_queue, &response_packet, portMAX_DELAY);

    /* Check status */
    if (response_packet.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return false;
    }

    /* Extract the light sensor reading */
    *ambient_light = response_packet.payload.light_sensor;

    return true;
}


/**
 * @brief
 * Task used to monitor the reception of command packets sent the light sensor
 * @param param
 * Unused
 */
void task_light_sensor(void *param)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    task_console_printf("Starting Light Sensor Task\r\n");

    // Verify ID
    uint8_t manuf_id = ltr_light_sensor_manufac_id();

    if (manuf_id != 0x05)   // LTR-329ALS-01 manufacturer ID is 0x05
    {
        task_console_printf("ERROR: Light Sensor Manufacturer ID mismatch! (0x%02X)\r\n", manuf_id);
        vTaskSuspend(NULL);   // Stop this task permanently
    }

    task_console_printf("Manufacturer ID = 0x%02x\r\n", manuf_id);

    //Initialize sensor
    ltr_light_sensor_start();
    vTaskDelay(pdMS_TO_TICKS(20));


    while (1)
    {
        /* Wait for a request */
        xQueueReceive(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY);

        /* Prepare the response packet */
        response_packet.device = DEVICE_LIGHT;

        //Read request received
        if (request_packet.operation == DEVICE_OP_READ)
        {
            uint16_t ch0 = 0;
            uint16_t ch1 = 0;

            /* Wait for NEW_DATA and read both channels */
            ltr_light_sensor_get_readings(&ch1, &ch0);

            /* Return CH0 as the ambient light value */
            response_packet.payload.light_sensor = ch0;
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
        }
        else
        {
            //Invalid or unsupported request
            task_console_printf("ERROR: Light Sensor received invalid request type (%d)\r\n",
                                request_packet.operation);

            response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
        }

        /* Send response back to requester */
        xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
    }
}



/**
 * @brief
 * Initializes software resources related to the operation of
 * the Temp Sensor.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_light_sensor_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
    /* Save the I2C Object */
    I2C_Obj = i2c_obj;

    /* Save the I2C Semaphore */
    I2C_Semaphore = i2c_semaphore;
    if (I2C_Semaphore == NULL)
    {
        return false;
    }

	/* Create the Queue used to receive requests from other tasks */
	Queue_Light_Sensor_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_Light_Sensor_Requests == NULL)
	{
		return false;
	}
	
	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_light_sensor,
		"Light Sensor",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY + 1,
		NULL) != pdPASS)
	{
		return false;
	}
	else 
	{
		return true;
	}	
}	
#endif