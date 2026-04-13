/**
 * @file task_temp_sensor.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "cy_result.h"
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "task_temp_sensor.h"
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
static SemaphoreHandle_t *I2C_Semaphore;

/* Queue used to send commands used to temp sensor */
QueueHandle_t Queue_Temp_Sensor_Requests;

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/** Read the value of the input port
 *
 * @param reg The reg address to read
 *
 */
static float LM75_get_temp(void)
{
	float temp = 0;
	// since the value of temperature will be returned as a 2s complement value, might as well read it as a signed integer so when we reformat there's no need to worry about the sign bit.
	int16_t raw_value =  0;
	cy_rslt_t rslt;

	// Read 2-bytes from the temperature register
	rslt = i2c_read_u16(I2C_Obj, LM75_SUBORDINATE_ADDR, LM75_TEMP_REG, (uint16_t*)&raw_value);

	if (rslt != CY_RSLT_SUCCESS)
	{
		return 0;
	}

	// Need to format the raw value read from the sensor
	// The LM75 returns a 9-bit (located in the upper 9 bits), two's complement value
	// will need to shift the value to get rid of the unused bits
	temp = (float)(raw_value >> 7);

	// Convert the raw value to degrees Celsius
	// Each bit (1) is worth 0.5 degrees C	
	temp = temp * 0.5;

	return temp;
}

static uint8_t LM75_get_product_id(void)
{
	uint8_t prod_id = 0;
	cy_rslt_t rslt;

	/*Read the product ID register*/
	rslt = i2c_read_u8(I2C_Obj, LM75_SUBORDINATE_ADDR, LM75_PRODUCT_ID_REG, &prod_id);

	if (rslt != CY_RSLT_SUCCESS)
	{
		return 0;
	}

	return prod_id;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief 
 * This is a helper function that is called by tasks other than the temp sensor task
 * when they want to read the temperature from the sensor.
 */
bool system_sensors_get_temp(QueueHandle_t return_queue, float *temperature)
{
	device_request_msg_t packet;
	device_response_msg_t response;

	if(return_queue == NULL || temperature == NULL)
	{
		return false;
	}

	/* Format the request packet */
	packet.device = DEVICE_TEMPERATURE;
	packet.operation = DEVICE_OP_READ;
	packet.response_queue = return_queue;
	// don't need to fill out the value field since we're just reading the temperature.
	// and don't need to specify an address field since the temp sensor only has one main register we care about whose address we already know

	/* Send a request to the temp sensor task*/
	xQueueSend(Queue_Temp_Sensor_Requests, &packet, portMAX_DELAY);

	/* Wait for the response from the temp sensor task */
	xQueueReceive(return_queue, &response, portMAX_DELAY);

	/* Return the temperature from the response provided by the temp sensor task */
	*temperature = response.payload.temperature;

	if (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
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
 * Task used to monitor the reception of command packets sent to the temp sensor
 * @param param
 * Unused
 */
void task_temp_sensor(void *param)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;
	
	task_console_printf("Starting Temp Sensor Task\r\n");
	
	/* Verify that the device was found on the I2C Bus */
	// make sure we grab the semaphore before accessing the I2C bus
	xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
	uint8_t prod_id = LM75_get_product_id();

	if (prod_id != LM75_PRODUCT_ID)
	{
		task_console_printf("Temp Sensor not found! Check connections and try again.\r\n");
		vTaskDelay(pdMS_TO_TICKS(1000)); // give UART enough time to print out the message before halting the processor

		CY_ASSERT(0); // since we're only interfacing with the temp sensor in this task, if we can't find it then there's no point in continuing to run the task since it won't be able to do anything useful, so just halt the processor.
	}
	else
	{
		task_console_printf("Temp Sensor found! Starting task...\r\n");
	}

	/*Release the semaphore*/
	xSemaphoreGive(*I2C_Semaphore);


	// once verified the temp sensor is there, now we can do our normal gatekeeper task
	while (1)
	{
		/* Wait for a request message message */
		xQueueReceive(Queue_Temp_Sensor_Requests, &request_packet, portMAX_DELAY);
	
		/* Grab the I2C semaphore */
		xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY); // need to do this anytime we want to access the I2C bus to ensure exclusive access since it's a shared resource

		/* Read the Temperature*/
		float temperature = LM75_get_temp();

		/* Release the I2C semaphore */
		xSemaphoreGive(*I2C_Semaphore);

		/* Return the temperature */
		response_packet.device = DEVICE_TEMPERATURE;
		response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
		response_packet.payload.temperature = temperature;
		xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);


	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the Temp Sensor.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_temp_sensor_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
	/* Save the I2C object and semaphore */
	I2C_Obj = i2c_obj;
	I2C_Semaphore = i2c_semaphore;

	if (I2C_Semaphore == NULL || I2C_Obj == NULL)
	{
		return false;
	}	

	/* Create the Queue used to receive requests  */
	Queue_Temp_Sensor_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_Temp_Sensor_Requests == NULL)
	{
		return false;
	}
	
	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_temp_sensor,
		"Temp Sensor",
		10*configMINIMAL_STACK_SIZE,
		i2c_semaphore,
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