/**
 * @file eeprom.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2023-10-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "eeprom.h"
#include "cyhal_hw_types.h"
#include <sys/types.h>
#include "task_console.h"


/** Determine if the EEPROM is busy writing the last
 *  transaction to non-volatile storage
 *
 * @param
 *
 */

 void eeprom_wait_for_write(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    tx_buf[0] = 0x05;   // RDSR instruction
    tx_buf[1] = 0x00;   // Dummy byte to clock out status register

    do
    {
        // Assert chip select (active low)
        cyhal_gpio_write(cs_pin, false);

        cyhal_spi_transfer(
            spi_obj,
            tx_buf, sizeof(tx_buf),   // TX buffer + length
            rx_buf, sizeof(rx_buf),   // RX buffer + length
            0                         // No timeout
        );

        // Deassert chip select
        cyhal_gpio_write(cs_pin, true);

        // Loop until WIP bit (bit 0) becomes 0
    } while (rx_buf[1] & 0x01);

} 



/** Enables Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_enable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buf[1] = {0x06};  // WREN instruction
    uint8_t rx_buf[1];           // Dummy receive buffer

    // Assert chip select (active low)
    cyhal_gpio_write(cs_pin, false);

    cyhal_spi_transfer(
        spi_obj,
        tx_buf, sizeof(tx_buf),   // TX buffer + length
        rx_buf, sizeof(rx_buf),   // RX buffer + length
        0                         // No timeout
    );

    // Deassert chip select
    cyhal_gpio_write(cs_pin, true);

}

/** Disable Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_disable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buf[1] = {0x04};  // WRDI instruction
    uint8_t rx_buf[1];           // Dummy receive buffer

    // Assert chip select (active low)
    cyhal_gpio_write(cs_pin, false);

    cyhal_spi_transfer(
        spi_obj,
        tx_buf, sizeof(tx_buf),   // TX buffer + length
        rx_buf, sizeof(rx_buf),   // RX buffer + length
        0                         // No timeout
    );

    // Deassert chip select
    cyhal_gpio_write(cs_pin, true);

}

/** Writes a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value to write into memory
 *
 */
void eeprom_write_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address, uint8_t data)
{
	//Enable writes
    eeprom_write_enable(spi_obj, cs_pin);

    //Build transmit buffer for WRITE instruction
    uint8_t tx_buf[4];
    uint8_t rx_buf[4];

    tx_buf[0] = 0x02;                 // WRITE instruction
    tx_buf[1] = (address >> 8) & 0xFF; // Address high byte
    tx_buf[2] = address & 0xFF;        // Address low byte
    tx_buf[3] = data;                  // Data byte

    //Assert chip select
    cyhal_gpio_write(cs_pin, false);

    cyhal_spi_transfer(
        spi_obj,
        tx_buf, sizeof(tx_buf),   // TX buffer + length
        rx_buf, sizeof(rx_buf),   // RX buffer + length
        0                         // No timeout
    );

    //Deassert chip select
    cyhal_gpio_write(cs_pin, true);

    //Wait for write cycle to complete
    eeprom_wait_for_write(spi_obj, cs_pin);

}

/** Reads a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 *
 */
uint8_t eeprom_read_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address)
{
	uint8_t tx_buf[4];
    uint8_t rx_buf[4];

    // Build transmit buffer
    tx_buf[0] = 0x03;                 // READ instruction
    tx_buf[1] = (address >> 8) & 0xFF; // Address high byte
    tx_buf[2] = address & 0xFF;        // Address low byte
    tx_buf[3] = 0x00;                  // Dummy byte to clock out data

    // Assert chip select
    cyhal_gpio_write(cs_pin, false);

    cyhal_spi_transfer(
        spi_obj,
        tx_buf, sizeof(tx_buf),   // TX buffer + length
        rx_buf, sizeof(rx_buf),   // RX buffer + length
        0                         // No timeout
    );

    // Deassert chip select
    cyhal_gpio_write(cs_pin, true);

    // The data byte is in rx_buf[3]
    return rx_buf[3];

}