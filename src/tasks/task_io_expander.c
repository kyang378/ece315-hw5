/**
 * @file io_expander.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#if defined(ECE353_FREERTOS)
#include "cyhal_gpio.h"
#include "task_io_expander.h"
#include "task_console.h"
#include "rtos_events.h"
#include "devices.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_io_expander(void *param);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
static cyhal_i2c_t *I2C_Obj;
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to io expander */
QueueHandle_t Queue_IO_Expander_Requests;

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

bool system_sensors_io_expander_write(QueueHandle_t return_queue, uint8_t address, uint8_t value)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	/* Format the request packet */
	request_packet.device = DEVICE_IO_EXP;
	request_packet.operation = DEVICE_OP_WRITE;
	request_packet.address = address;
	request_packet.value = value;
	request_packet.response_queue = return_queue;

	/* Send the request packet to the io expander task */
	if (xQueueSend(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY) != pdPASS)
	{
		return false;
	}

	/* Optionally wait for a response if the caller wants completion status. */
	if (return_queue != NULL)
	{
		if (xQueueReceive(return_queue, &response_packet, portMAX_DELAY) != pdPASS)
		{
			return false;
		}

		if (response_packet.device != DEVICE_IO_EXP || response_packet.status != DEVICE_OPERATION_STATUS_WRITE_SUCCESS)
		{
			return false;
		}
	}

	return true;

}

bool system_sensors_io_expander_read(QueueHandle_t return_queue, uint8_t address, uint8_t *value)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	if(return_queue == NULL || value == NULL)
	{
		return false;
	}

	/* Format the request packet */
	request_packet.device = DEVICE_IO_EXP;
	request_packet.operation = DEVICE_OP_READ;
	request_packet.address = address;
	request_packet.response_queue = return_queue;
	// don't need to fill out the value field since we're just reading from the io expander and the address field already specifies which register we're reading from

	/* Send the request packet to the io expander task */
	if (xQueueSend(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY) != pdPASS)
	{
		return false;
	}

	/* Wait for the response from the io expander task */
	if (xQueueReceive(return_queue, &response_packet, portMAX_DELAY) != pdPASS)
	{
		return false;
	}

	/* Return the value read from the io expander from the response provided by the io expander task */
	
	if (response_packet.device == DEVICE_IO_EXP && response_packet.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
	{
		*value = response_packet.payload.io_expander;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief
 * Task used to monitor the reception of command packets sent the io expander
 * @param param
 * Unused
 */
void task_io_expander(void *param)
{
	(void)param;
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	task_console_printf("Starting IO Expander Task\r\n");

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);

		//response_packet.device = DEVICE_IO_EXP;

		if (request_packet.device == DEVICE_IO_EXP && request_packet.operation == DEVICE_OP_WRITE)
		{
			response_packet.status = DEVICE_OPERATION_STATUS_WRITE_FAILURE;

			/* Write the value to the specified register address of the IO expander via I2C */
			if (xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY) == pdPASS)
			{
				if (i2c_write_u8(I2C_Obj, TCA9534_SUBORDINATE_ADDR, request_packet.address, request_packet.value) == CY_RSLT_SUCCESS)
				{					
					if (request_packet.response_queue != NULL)
						{
							response_packet.status = DEVICE_OPERATION_STATUS_WRITE_SUCCESS;
							response_packet.device = DEVICE_IO_EXP;
							xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
						}
				}
				xSemaphoreGive(*I2C_Semaphore);
			}
		}
		else if (request_packet.operation == DEVICE_OP_READ)
		{
			uint8_t read_value = 0;
			response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;

			/* Read the value from the specified register address of the IO expander via I2C and save it in read_value */
			if (xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY) == pdPASS)
			{
				if (i2c_read_u8(I2C_Obj, TCA9534_SUBORDINATE_ADDR, request_packet.address, &read_value) == CY_RSLT_SUCCESS)
				{
					if (request_packet.response_queue != NULL)
						{
							response_packet.payload.io_expander = read_value;
							response_packet.device = DEVICE_IO_EXP;
							response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
							xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
						}
				}
				xSemaphoreGive(*I2C_Semaphore);
				}
			}
		else
		{
			response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
			task_console_printf("Invalid operation type in request packet!\n\r");
		}
	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the IO Expander.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_io_expander_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
	/* Save the I2C object and semaphore */
	I2C_Obj = i2c_obj;
	I2C_Semaphore = i2c_semaphore;
	if (I2C_Semaphore == NULL || I2C_Obj == NULL)
	{
		return false;
	}

	/* Create the Queue used to control blinking of the status LED*/
	Queue_IO_Expander_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_IO_Expander_Requests == NULL)
	{
		return false;
	}

	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_io_expander,
		"Task IO Exp",
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
