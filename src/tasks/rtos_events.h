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

 // extern means this variable is defined in some other file, but we can access it here by including the header file; this is because we want to use this event group in multiple files, so we want to define it in one file and declare it as extern in the header file so we can access it in other files that include this header file.

 // for example, we can define the event group in ice05.c, and then include this header file in task_buttons.h and task_buzzer.h to access the event group and set/wait for events.
 extern EventGroupHandle_t ECE353_RTOS_Events; 

/*******************************************************************************
* Macros used to define the system events
******************************************************************************/

#define ECE353_EVENT_BUTTON_SW1_PRESSED   (1 << 0)    // Event bit 0
#define ECE353_EVENT_BUTTON_SW2_PRESSED   (1 << 1)    
#define ECE353_EVENT_BUTTON_SW3_PRESSED   (1 << 2)

#endif // ECE353_FREERTOS

#endif // __RTOS_EVENTS_H__