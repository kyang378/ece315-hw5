/**
 * @file rtos_events.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef __RTOS_EVENTS_H__
 #define __RTOS_EVENTS_H__
 #include "main.h"
 
#ifdef ECE353_FREERTOS

/*******************************************************************************
* Event Group for system events.
 ******************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;

/*******************************************************************************
* Macros used to define the system events
******************************************************************************/
//Switches
#define ECE353_EVENT_SW1_PRESSED (1 << 0)  // Event bit for SW1 pressed
#define ECE353_EVENT_SW2_PRESSED (1 << 1)  // Event bit for SW2 pressed
#define ECE353_EVENT_SW3_PRESSED (1 << 2)  // Event bit for SW3 pressed

//Joystick
#define ECE353_EVENT_JOYSTICK_UP        (1 << 4)
#define ECE353_EVENT_JOYSTICK_DOWN      (1 << 5)
#define ECE353_EVENT_JOYSTICK_LEFT      (1 << 6)
#define ECE353_EVENT_JOYSTICK_RIGHT     (1 << 7)

//IPC - verify this is correct if not working
#define ECE353_EVENT_IPC_ACK_RECEIVED           (1<<9)
#define ECE353_EVENT_IPC_DISCOVERY_RECEIVED     (1 << 10)
#define ECE353_EVENT_IPC_ACTIVE_RECEIVED      (1 << 11)
#define ECE353_EVENT_IPC_INACTIVE_RECEIVED    (1 << 12)
#define ECE353_EVENT_IPC_STATUS_RECEIVED      (1 << 13)
#define ECE353_EVENT_IPC_GUESS_RECEIVED       (1 << 14)




#endif // ECE353_FREERTOS

#endif // __RTOS_EVENTS_H__