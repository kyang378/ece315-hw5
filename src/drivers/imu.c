/**
 * @file imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "imu.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @param value 
 */
void imu_write_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t value
)
{
    // Write 2 bytes: the register address and the value
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2]; // dummy buffer for received data
    tx_buffer[0] = reg & 0x7F; // clear MSB for write
    tx_buffer[1] = value;

    // Pull CS low to select the IMU
    cyhal_gpio_write(cs_pin, 0);

    // Perform the SPI transfer
    cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

    // Pull CS high to deselect the IMU
    cyhal_gpio_write(cs_pin, 1);
}

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @return uint8_t 
 */
uint8_t imu_read_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg
)
{
    // Write the register address with MSB set to indicate a read
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2]; // buffer for received data

    tx_buffer[0] = reg | 0x80; // set MSB for read
    tx_buffer[1] = 0x00; // dummy byte (to transmit this byte of data in order to keep the SPI clock running so that we can receive the register value from the IMU)

    // Pull CS low to select the IMU
    cyhal_gpio_write(cs_pin, 0);

    // Perform the SPI transfer
    cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

    // Pull CS high to deselect the IMU
    cyhal_gpio_write(cs_pin, 1);

    return rx_buffer[1]; // The second byte contains the register value; first byte is don't-care
}

/**
 * @brief Read multiple (4) registers from the IMU
 * 
 * @param reg 
 * @param buffer 
 * @param length 
 */
void imu_read_registers(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t *buffer, 
    uint8_t length
)
{
    uint8_t read_cmd = reg | 0x80;

    if(buffer == NULL || length == 0)
    {
        return;
    }

    // Pull CS low to select the IMU
    cyhal_gpio_write(cs_pin, 0);

    // Send the starting register address, then clock out the requested bytes.
    cyhal_spi_transfer(spi_obj, &read_cmd, 1, NULL, 0, 0xFF);
    cyhal_spi_transfer(spi_obj, NULL, 0, buffer, length, 0xFF);

    // Pull CS high to deselect the IMU
    cyhal_gpio_write(cs_pin, 1);
}

/**
 * @brief 
 * This function verifys the WHO_AM_I register and configures the IMU
 * to 104 Hz, ±2g, ±250 dps.
 * @param spi_obj 
 * @param cs_pin 
 * @return true 
 * @return false 
 */
bool imu_init(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
    // Wait for 15mS after power-up
    cyhal_system_delay_ms(15);

    // Verify the WHO_AM_I register
    uint8_t who_am_i = imu_read_reg(spi_obj, cs_pin, IMU_REG_WHO_AM_I);
    if (who_am_i != 0x6A)
    {
        return false;
    }

    // Reset the IMU
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C, 0x01); // CTRL3_C register; first bit is the reset bit

    // Wait for the reset to complete
    do
    {
        cyhal_system_delay_ms(1);
    } while (imu_read_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C) & 0x01);

    // Configure the IMU:  104 Hz (slowest rate supported), ±2g, ±250 dps
    // the slower the rate, the less power the IMU consumes
    // can detect the acceleration up to ±2g, and angular rate up to ±250 dps (degrees per second)
    // the smaller the range, the more sensitive the measurements are (i.e. smaller changes in acceleration/angular rate will result in larger changes in the raw register values, which allows for more precise measurements)
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL1_XL, ODR_104HZ | FS_XL_2G);

    return true;
}
