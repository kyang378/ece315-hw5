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

cy_rslt_t cap_touch_write_u8(cyhal_i2c_t *obj, uint8_t reg, uint8_t value)
{
    return i2c_write_u8(obj, FT6X06_I2C_ADDR, reg, value);
}

cy_rslt_t cap_touch_read_u8(cyhal_i2c_t *obj, uint8_t reg, uint8_t *value)
{
    return i2c_read_u8(obj, FT6X06_I2C_ADDR, reg, value);
}

uint8_t cap_touch_get_num_points(cyhal_i2c_t *obj)
{
    uint8_t status = 0;

    if (cap_touch_read_u8(obj, FT6X06_REG_TD_STATUS, &status) != CY_RSLT_SUCCESS)
    {
        return 0;
    }

    return (status & FT6X06_TD_STATUS_MASK);
}

bool cap_touch_get_position(cyhal_i2c_t *obj, uint16_t *x, uint16_t *y)
{
    uint8_t xh = 0;
    uint8_t xl = 0;
    uint8_t yh = 0;
    uint8_t yl = 0;

    if (x == NULL || y == NULL)
    {
        return false;
    }

    if (cap_touch_read_u8(obj, FT6X06_REG_P1_XH, &xh) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    if (cap_touch_read_u8(obj, FT6X06_REG_P1_XL, &xl) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    if (cap_touch_read_u8(obj, FT6X06_REG_P1_YH, &yh) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    if (cap_touch_read_u8(obj, FT6X06_REG_P1_YL, &yl) != CY_RSLT_SUCCESS)
    {
        return false;
    }

    *x = (((uint16_t)(xh & FT6X06_TOUCH_POS_MSB_MASK)) << 8) | xl;
    *y = (((uint16_t)(yh & FT6X06_TOUCH_POS_MSB_MASK)) << 8) | yl;

    return true;
}
