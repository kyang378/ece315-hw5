/**
 * @file task_joystick.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __TASK_JOYSTICK_H__
#define __TASK_JOYSTICK_H__
#include "main.h"
#include "drivers.h"
#include "rtos_events.h"
#include <complex.h>

#ifdef ECE353_FREERTOS
#if defined(HW02)
#define TASK_HW02_SYSTEM_JOYSTICK_STACK_SIZE    (configMINIMAL_STACK_SIZE*5)
#define TASK_HW02_SYSTEM_JOYSTICK_PRIORITY      (tskIDLE_PRIORITY + 1)
#define TASK_HW02_SYSTEM_JOYSTICK_POLL_MS       (50)
#endif

extern QueueHandle_t Queue_Joystick;

void task_joystick(void *arg);

bool task_joystick_init(void);

#if defined(HW02)
void task_hw02_system_joystick(void *arg);
bool task_hw02_system_joystick_resources_init(QueueHandle_t queue_joystick);
#endif

#endif
#endif /* __TASK_JOYSTICK_H__ */
