/**
 * @file cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "cap_touch.h"

// I2C object created in hw04.c
extern cyhal_i2c_t *I2C_Obj;

/******************************************************************************
 * Read 1 byte from FT6236
 *****************************************************************************/
bool cap_touch_read_byte(uint8_t reg, uint8_t *value)
{
    cy_rslt_t rslt = i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, reg, value);
    return (rslt == CY_RSLT_SUCCESS);
}

/******************************************************************************
 * Write 1 byte to FT6236
 *****************************************************************************/
bool cap_touch_write_byte(uint8_t reg, uint8_t value)
{
    cy_rslt_t rslt = i2c_write_u8(I2C_Obj, FT6X06_I2C_ADDR, reg, value);
    return (rslt == CY_RSLT_SUCCESS);
}

/******************************************************************************
 * Read X/Y coordinates of touch point 1
 *
 * Returns:
 *   true  = valid coordinates
 *   false = no touch or I2C failure
 *****************************************************************************/
bool cap_touch_get_coordinates(uint16_t *x, uint16_t *y)
{
    uint8_t touches = 0;

    // Read number of touches (0, 1, or 2)
    if (!cap_touch_read_byte(FT6X36_REG_TD_STATUS, &touches))
        return false;

    touches &= 0x0F;  // lower 4 bits contain number of touches

    if (touches == 0)
        return false;

    uint8_t xh, xl, yh, yl;

    if (!cap_touch_read_byte(FT6X36_REG_P1_XH, &xh)) return false;
    if (!cap_touch_read_byte(FT6X36_REG_P1_XL, &xl)) return false;
    if (!cap_touch_read_byte(FT6X36_REG_P1_YH, &yh)) return false;
    if (!cap_touch_read_byte(FT6X36_REG_P1_YL, &yl)) return false;


    // Datasheet: high byte uses only lower 4 bits for coordinate
    *x = ((xh & 0x0F) << 8) | xl;
    *y = ((yh & 0x0F) << 8) | yl;

    if (*x > 500 || *y > 400) return false; //data not on screen

    // Swap axes (FT6236 reports Y in Sensor0 and X in Sensor1)
    uint16_t temp = *x;
    *x = *y;
    *y = temp;

    //inverting y axis to read top = 0, bottom = high
    *y = 240 - *y;



    return true;
}

