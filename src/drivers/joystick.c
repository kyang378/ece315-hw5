#include "joystick.h"
#include "cy_result.h"
#include "cyhal_adc.h"

/* Static Global Variables */
static cyhal_adc_t joystick_adc_obj; // the A2D converter
static cyhal_adc_channel_t joystick_adc_chan_x_obj; // analog channel
static cyhal_adc_channel_t joystick_adc_chan_y_obj;
const cyhal_adc_channel_config_t channel_config = { .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };

const cyhal_adc_config_t joystick_config = {
    .continuous_scanning = true, 
    .resolution = 12, // but per the API the function spits out 16-bit #, which is just 12 extrapolated to 16. So from a software perspective, we can just treat it as a 16-bit ADC and not worry about the resolution setting.
    .average_count = 1, 
    .average_mode_flags = 0,
    .ext_vref_mv = 0,
    .vneg = CYHAL_ADC_VNEG_VREF,
    .vref = CYHAL_ADC_REF_VDDA_DIV_2,
    .ext_vref = NC,
    .is_bypassed = false,
    .bypass_pin = NC
};

/* Public API */

/** Initialize the ADC channels connected to the Joystick
 *
 * @param - None
 */
cy_rslt_t joystick_init(void)
{
    cy_rslt_t rslt;

    /* Initialize ADC.*/
    rslt = cyhal_adc_init(&joystick_adc_obj, PIN_ANALOG_JOY_X, NULL);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    /* Set the Reference Voltage to be VDDA */
    rslt = cyhal_adc_configure(&joystick_adc_obj, &joystick_config);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }


    /* Initialize X direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_x_obj, 
        &joystick_adc_obj, 
        PIN_ANALOG_JOY_X, 
        CYHAL_ADC_VNEG, 
        &channel_config
    );
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    /* Initialize Y direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_y_obj, 
        &joystick_adc_obj, 
        PIN_ANALOG_JOY_Y, 
        CYHAL_ADC_VNEG, 
        &channel_config
    );

    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

}

/** Read X direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_x(void)
{
    /* ADD CODE */
    return cyhal_adc_read_u16(&joystick_adc_chan_x_obj);

}

/** Read Y direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_y(void)
{
    /* ADD CODE */
    return cyhal_adc_read_u16(&joystick_adc_chan_y_obj);

}


/**
 * @brief 
 * Returns the current position of the joystick 
 * @return joystick_position_t 
 */
joystick_position_t joystick_get_pos(void)
{
    uint16_t x_val;
    uint16_t y_val;
    joystick_position_t position = JOYSTICK_POS_CENTER;
    
    // an adc value < 0.825 or > 2.475 => joystick has been moved
    x_val = joystick_read_x(); y_val = joystick_read_y();
    if (x_val > JOYSTICK_THRESH_X_LEFT_2P475V)
    {
        if (y_val > JOYSTICK_THRESH_Y_UP_2P475V) position = JOYSTICK_POS_UPPER_LEFT;
        else if (y_val < JOYSTICK_THRESH_Y_DOWN_0P825V) position = JOYSTICK_POS_LOWER_LEFT;
        else position = JOYSTICK_POS_LEFT; // otherwise y direction unchanged (is in middle)
    } 
    
    else if (x_val < JOYSTICK_THRESH_X_RIGHT_0P825V)
    {
        if (y_val > JOYSTICK_THRESH_Y_UP_2P475V) position = JOYSTICK_POS_UPPER_RIGHT;
        else if (y_val < JOYSTICK_THRESH_Y_DOWN_0P825V) position = JOYSTICK_POS_LOWER_RIGHT;
        else position = JOYSTICK_POS_RIGHT;
    } 
    
    else 
    {   // if x stays in middle
        if (y_val > JOYSTICK_THRESH_Y_UP_2P475V) position = JOYSTICK_POS_UP;
        else if (y_val < JOYSTICK_THRESH_Y_DOWN_0P825V) position = JOYSTICK_POS_DOWN;
    }

    return position;
}
