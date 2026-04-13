/**
 * @file task_temp_sensor.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
 #ifndef __TASK_TEMP_SENSOR_H__
#define __TASK_TEMP_SENSOR_H__

#include "main.h"
#include "cy_result.h"

#if defined (ECE353_FREERTOS)
#include "drivers.h"
#include "devices.h"
#include "task_temp_sensor.h"
#include "task_console.h"

// from data sheet (7-bit address: 1001.a2a1a0, where the physical address pins are 1s)
#define LM75_SUBORDINATE_ADDR                 0x4F
#define LM75_TEMP_REG						  0x00
#define LM75_PRODUCT_ID_REG                   0x07
#define LM75_PRODUCT_ID                       0xA1

static cyhal_i2c_t* I2C_Obj = NULL;

extern QueueHandle_t Queue_Temp_Sensor_Requests;

// we'll have individual gatekeeper tasks for each device on the I2C bus, so we'll need a semaphore to control access to the I2C bus since it's a shared resource. The gatekeeper tasks will take the semaphore before accessing the I2C bus and give it back when they're done. This ensures that only one task is using the I2C bus at a time and prevents conflicts.

// QUESTION: why not just have one gatekeeper task so we don't need a semaphore? We need to make sure there's only one task using the I2C resources at a time anyways.
static SemaphoreHandle_t* I2C_Semaphore = NULL;

/* Functions used to interact with the Temp Sensor */
bool system_sensors_get_temp(QueueHandle_t return_queue, float *temperature);

/* Function used to initialize resources for the Temp Sensor task */
bool task_temp_sensor_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj );

#endif

#endif
