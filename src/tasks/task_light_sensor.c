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

#define LTR_STARTUP_DELAY_MS        110u
#define LTR_DATA_POLL_DELAY_MS      10u
#define LTR_DATA_READY_TIMEOUT_MS   1000u


/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/
/**
 * @brief
 * Set ALS MODE to Active and initiate a software reset
 */
static void ltr_light_sensor_start(void)
{

    /* Write to ALS_CONTR register to activate active mode and initiate a software reset */
    // need to do two separate writes; doesn't make sense to do it all at once (doesn't make sense to both reset and set the device to active mode.)
    i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_SW_RESET);  
    i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_ALS_MODE);    

}

static uint8_t ltr_light_get_contr(void)
{
    uint8_t value = 0;

    // read from the control register
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, &value);

    return value;
}

static uint8_t ltr_light_sensor_status(void)
{
    uint8_t value = 0;
    
    // read from the status register
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_STATUS, &value);

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

    // note part ID is bits 7:4, so needs shifting
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_PART_ID, &value);
    value = value >> 4;

    return value;
}

static uint8_t ltr_light_sensor_manufac_id(void)
{
    uint8_t value = 0;

    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_MANUFAC_ID, &value);

    return value;
}

static uint16_t ltr_light_sensor_get_ch0(void)
{
    uint8_t msbyte;
    uint8_t lsbyte;

    // read the two bytes corresponding to channel 0 data
    // datasheet specifies that the lower byte should be read first
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_0, &lsbyte);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_1, &msbyte);

    return (uint16_t)(msbyte << 8) | lsbyte;
}

static uint16_t ltr_light_sensor_get_ch1(void)
{
    uint8_t msbyte;
    uint8_t lsbyte;

    // read the two bytes corresponding to channel 1 data
    // datasheet specifies that the lower byte should be read first
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_0, &lsbyte);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_1, &msbyte);
    

    return (uint16_t)(msbyte << 8) | lsbyte;
}

static bool ltr_light_sensor_get_readings(uint16_t *ch1, uint16_t *ch0, uint8_t *status_out)
{
    uint8_t status = 0;
    TickType_t start_time = xTaskGetTickCount();

    while((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(LTR_DATA_READY_TIMEOUT_MS))
    {
        status = ltr_light_sensor_status();

        /*
         * Bit 2 indicates unread data is available.
         * Bit 7 is 0 when the sampled data is valid.
         */
        if (((status & LTR_REG_STATUS_NEW_DATA) == LTR_REG_STATUS_NEW_DATA) &&
            ((status & LTR_REG_STATUS_VALID_DATA) == 0))
        {
            // channel 1 needs to be read first before channel 0 according to the datasheet
            *ch1 = ltr_light_sensor_get_ch1();
            *ch0 = ltr_light_sensor_get_ch0();

            if (status_out != NULL)
            {
                *status_out = status;
            }

            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(LTR_DATA_POLL_DELAY_MS));
    }

    if (status_out != NULL)
    {
        *status_out = status;
    }

    return false;
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

    if(return_queue == NULL || ambient_light == NULL)
    {
        return false;
    }

    /* Format the request packet */
    request_packet.device = DEVICE_LIGHT;
    request_packet.operation = DEVICE_OP_READ;
    request_packet.response_queue = return_queue;

    /* Send the request packet to the light sensor task */
    if (xQueueSend(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    /* Wait for the response from the light sensor task */
    if (xQueueReceive(return_queue, &response_packet, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    /* Return the ambient light value from the response provided by the light sensor task */
    *ambient_light = response_packet.payload.light_sensor;

    if (response_packet.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }

}

/**
 * @brief
 * Task used to monitor the reception of command packets sent to the light sensor
 * @param param
 * Unused
 */
void task_light_sensor(void *param)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

	task_console_printf("Starting Light Sensor Task\r\n");

    /* Before we monitor the request queue, first verify that the device was found on the I2C Bus by checking the part ID*/
    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY); // about to access the I2C bus
    uint8_t part_id = ltr_light_sensor_part_id();
    if (part_id != LTR_PART_ID)
    {
        task_console_printf("Light Sensor not found on I2C bus! Part ID read: 0x%02X\n\r", part_id);
        vTaskDelay(pdMS_TO_TICKS(1000)); // give time for UART to print out the message before suspending the task itself
        vTaskSuspend(NULL); // suspend self since we can't do anything without the sensor
    }
    else
    {
        uint8_t manufac_id = ltr_light_sensor_manufac_id();
        if (manufac_id != LTR_MANUFAC_ID)
        {
            task_console_printf("Unexpected Manufacturer ID read from Light Sensor: 0x%02X\n\r", manufac_id);
            vTaskDelay(pdMS_TO_TICKS(1000)); // give time for UART to print out the message before suspending the task itself
            vTaskSuspend(NULL); // suspend self since we can't do anything without the sensor
        }
    }

    /* Start the Light Sensor */
    ltr_light_sensor_start();

    /* Release the semaphore and allow the first conversion to complete. */
    xSemaphoreGive(*I2C_Semaphore);
    vTaskDelay(pdMS_TO_TICKS(LTR_STARTUP_DELAY_MS));

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY);

        /* Check that the message contains valid and recognized request type; otherwise print a descriptive error message to the console*/
        if (request_packet.device != DEVICE_LIGHT)
        {
            task_console_printf("Invalid device type in request packet!\n\r");
        }
        if (request_packet.operation != DEVICE_OP_READ)
        {
            task_console_printf("Invalid operation type in request packet!\n\r");
        }

        /* Grab the I2C semaphore */
		xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY); // about to access the I2C bus

        /* Read the light sensor values */
        uint16_t ch0 = 0;
        uint16_t ch1 = 0;
        uint8_t status = 0;
        bool read_success = ltr_light_sensor_get_readings(&ch1, &ch0, &status);

        /* Release the I2C semaphore */
        xSemaphoreGive(*I2C_Semaphore);

        /* Return the light sensor value in the response packet and send it back to the requester via the response queue provided in the request packet */
        response_packet.device = DEVICE_LIGHT;
        if (read_success)
        {
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
            response_packet.payload.light_sensor = ch0; // payload is ch0
        }
        else
        {
            response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
            response_packet.payload.light_sensor = 0;
            task_console_printf("Light sensor read timed out. ALS_STATUS=0x%02X\r\n", status);
        }

        xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);

	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the Light Sensor.  This function expects that the I2C bus had already
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
		100*configMINIMAL_STACK_SIZE,
		I2C_Semaphore, // pass the I2C semaphore as the task parameter since the light sensor task needs to use it to control access to the I2C bus
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
