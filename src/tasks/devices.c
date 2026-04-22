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


/**
 * @brief Helper method to skip spaces when parsing
 * 
 * @param p the current parsing index
 * @return char* the adress of the next non-space character
 */
static char* skip_spaces(char *p)
{
    while (*p == ' ') {
        p++;
    }
    return p;
}


/**
 * @brief Parses a string for a hexidecimal number (adress) beginning with 0x 
 * 
 * @param p the current parsing index
 * @param out the hex number found
 * @return true if a hex number was found
 * @return false if a hex number was not found
 */
static bool parse_hex(const char **p, uint32_t *out)
{
    const char *s = *p;
    uint32_t value = 0;

    // Must start with "0x" or "0X"
    if (!(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {
        return false;
    }
    s += 2;

    bool found_digit = false;

    while (1)
    {
        char c = *s;
        uint8_t digit;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        }
        else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        }
        else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        }
        else {
            break;
        }

        found_digit = true;
        value = (value << 4) | digit;
        s++;
    }

    if (!found_digit) {
        return false;
    }

    *out = value;
    *p = s;
    return true;
}

/**
 * @brief parses a command and fills out the appropriate device request packet
 * 
 * @param data the command received
 * @param request the request packet to be filled
 * @return true if successful
 * @return false if unsuccessful or if command is invalid
 */
bool parse_cli_data(char *data, device_request_msg_t *request) {
    char *p = data;


    // EEPROM COMMANDS
    
    if (strncmp(p, "EEPROM", 6) == 0) {
        p += 6;
        p = skip_spaces(p);

        // EEPROM read
        if (*p == 'R') {
            p++;
            p = skip_spaces(p);

            //get the adress
            uint32_t addr32;
            if (!parse_hex((const char **)&p, &addr32)) {
                return false;
            }

            //fill out packet
            request->device    = DEVICE_EEPROM;
            request->operation = DEVICE_OP_READ;
            request->address   = (uint16_t)addr32;
            request->value     = 0;
            request->response_queue = NULL; 

            return true;
        }

        //EEPROM Write
        if (*p == 'W')
        {
            p++;
            p = skip_spaces(p);

            //get adress to write to
            uint32_t addr32;
            if (!parse_hex((const char **)&p, &addr32)) {
                return false;
            }

            //get value to write
            p = skip_spaces(p);
            
            uint32_t val32;
            if (!parse_hex((const char **)&p, &val32)) {
                return false;
            }

            //fill out packet
            request->device    = DEVICE_EEPROM;
            request->operation = DEVICE_OP_WRITE;
            request->address   = (uint16_t)addr32;
            request->value     = (uint8_t)val32;
            request->response_queue = NULL;

            return true;
        }

        return false;
    }

    // CAP_TOUCH COMMAND
    if (strcmp(p, "CAP_TOUCH") == 0) {
        request->device         = DEVICE_CAP_TOUCH;
        request->operation      = DEVICE_OP_READ;
        request->address        = 0;
        request->value          = 0;
        request->response_queue = NULL;   // will be set by console

        return true;
    }

    // OTHER COMMANDS HERE ...

    return false;
}






#endif /* ECE353_FREERTOS */
