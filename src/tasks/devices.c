/**
 * @file devices.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "devices.h"

#if defined(ECE353_FREERTOS)
#include "task_imu.h"
#include "task_temp_sensor.h"
#include "task_light_sensor.h"
#include "task_io_expander.h"
#include "task_eeprom.h"
#include "task_console.h"

// Parse the CLI command string into a device request. The response queue is
// owned by the caller, so this function only fills in the command fields.
bool parse_cli_data(char* data, device_request_msg_t* request)
{
    int address = 0;
    int value = 0;
    char extra = '\0';
    char command[32] = {0};

    if(data == NULL || request == NULL)
    {
        return false;
    }

    request->device = DEVICE_UNKNOWN;
    request->operation = DEVICE_OP_READ;
    request->address = 0;
    request->value = 0;
    request->response_queue = NULL;

    if(sscanf(data, "EEPROM R %i %c", &address, &extra) == 1)
    {
        if(address < 0 || address > 0x7FFF)
        {
            return false;
        }

        request->device = DEVICE_EEPROM;
        request->operation = DEVICE_OP_READ;
        request->address = (uint16_t)address;
        return true;
    }

    if(sscanf(data, "EEPROM W %i %i %c", &address, &value, &extra) == 2)
    {
        if(address < 0 || address > 0x7FFF || value < 0 || value > 0xFF)
        {
            return false;
        }

        request->device = DEVICE_EEPROM;
        request->operation = DEVICE_OP_WRITE;
        request->address = (uint16_t)address;
        request->value = (uint8_t)value;
        return true;
    }

    // CAP_TOUCH has no parameters, so accept it only when it is the sole token.
    if ((sscanf(data, "%31s %c", command, &extra) == 1) &&
        (strcmp(command, "CAP_TOUCH") == 0))
    {
        request->device = DEVICE_CAP_TOUCH;
        request->operation = DEVICE_OP_READ;
        return true;
    }

    return false;
}

#endif /* ECE353_FREERTOS */
