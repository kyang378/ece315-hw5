#ifndef __CAP_TOUCH_H__
#define __CAP_TOUCH_H__

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "i2c.h"
#include "stdio.h"

#define FT6X06_I2C_ADDR             0x38

#define FT6X06_REG_DEV_MODE         0x00
#define FT6X06_REG_GEST_ID          0x01
#define FT6X06_REG_TD_STATUS        0x02
#define FT6X06_REG_P1_XH            0x03
#define FT6X06_REG_P1_XL            0x04
#define FT6X06_REG_P1_YH            0x05
#define FT6X06_REG_P1_YL            0x06
#define FT6X06_REG_G_MODE           0xA4
#define FT6X06_REG_FIRMID           0xA6
#define FT6X06_REG_FOCALTECH_ID     0xA8
#define FT6X06_REG_RELEASE_CODE_ID  0xAF
#define FT6X06_REG_STATE            0xBC

#define FT6X06_DEVICE_MODE_WORKING  0x00
#define FT6X06_G_MODE_POLLING       0x00
#define FT6X06_G_MODE_TRIGGER       0x01
#define FT6X06_FOCALTECH_ID         0x11

#define FT6X06_TD_STATUS_MASK       0x0F
#define FT6X06_TOUCH_POS_MSB_MASK   0x0F

cy_rslt_t cap_touch_write_u8(cyhal_i2c_t *obj, uint8_t reg, uint8_t value);
cy_rslt_t cap_touch_read_u8(cyhal_i2c_t *obj, uint8_t reg, uint8_t *value);
uint8_t cap_touch_get_num_points(cyhal_i2c_t *obj);
bool cap_touch_get_position(cyhal_i2c_t *obj, uint16_t *x, uint16_t *y);

#endif
