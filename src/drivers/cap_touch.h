#ifndef __CAP_TOUCH_H__
#define __CAP_TOUCH_H__

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "i2c.h"

//*****************************************************************************
// FT6236 I2C Address
//*****************************************************************************
#define FT6X06_I2C_ADDR                 0x38

//*****************************************************************************
// FT6236 Register Map (from datasheet page 26)
//*****************************************************************************
#define FT6X36_REG_TD_STATUS            0x02   // Number of touch points

// Touch 1 coordinate registers
#define FT6X36_REG_P1_XH                0x03
#define FT6X36_REG_P1_XL                0x04
#define FT6X36_REG_P1_YH                0x05
#define FT6X36_REG_P1_YL                0x06

//*****************************************************************************
// Public API
//*****************************************************************************
bool cap_touch_read_byte(uint8_t reg, uint8_t *value);
bool cap_touch_write_byte(uint8_t reg, uint8_t value);
bool cap_touch_get_coordinates(uint16_t *x, uint16_t *y);

#endif
