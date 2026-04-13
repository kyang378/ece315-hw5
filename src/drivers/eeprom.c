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


/** Determine if the EEPROM is busy writing the last
 *  transaction to non-volatile storage
 *
 * @param
 *
 */
void eeprom_wait_for_write(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t status[2];
	uint8_t cmd_msg[2] = {EEPROM_CMD_RDSR, 0xFF}; // 0xFF for don't-care for us to get the status (1 byte) back from the EEPROM
	do {
		// Pull CS low to select the EEPROM
		cyhal_gpio_write(cs_pin, 0);

		// Send the RDSR command and read the status register
		cyhal_spi_transfer(spi_obj, cmd_msg, 2, status, 2, 0xFF);

		// Pull CS high to deselect the EEPROM
		cyhal_gpio_write(cs_pin, 1);
	} while (status[1] & 0x01); // Stay here while the WIP bit is set; first byte is don't-care, second byte contains the status
}

/** Enables Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_enable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the WREN command
	uint8_t cmd = EEPROM_CMD_WREN;
	cyhal_spi_transfer(spi_obj, &cmd, 1, NULL, 0, 0xFF); // data returned is not useful

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);
}

/** Disable Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_disable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the WRDI command
	uint8_t cmd = EEPROM_CMD_WRDI;
	cyhal_spi_transfer(spi_obj, &cmd, 1, NULL, 0, 0xFF); // data returned is not useful

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);
}

/** Writes a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value to write into memory
 *
 */
void eeprom_write_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address, uint8_t data)
{
	// first make sure that the EEPROM is not busy with a previous write operation
	eeprom_wait_for_write(spi_obj, cs_pin);

	// enable writes to the EEPROM
	eeprom_write_enable(spi_obj, cs_pin);

	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the WRITE command
	uint8_t cmd = EEPROM_CMD_WRITE;
	cyhal_spi_transfer(spi_obj, &cmd, 1, NULL, 0, 0xFF); // data returned is not useful (actually all just high impedence)

	// Send the address
	uint8_t addr_bytes[2] = { (uint8_t)(address >> 8), (uint8_t)(address & 0xFF) };
	cyhal_spi_transfer(spi_obj, addr_bytes, 2, NULL, 0, 0xFF);

	// Send the data
	cyhal_spi_transfer(spi_obj, &data, 1, NULL, 0, 0xFF);

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);
}

/** Reads a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 *
 */
uint8_t eeprom_read_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address)
{
	// first make sure that the EEPROM is not busy with a previous write operation
	eeprom_wait_for_write(spi_obj, cs_pin);

	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the READ command
	uint8_t cmd = EEPROM_CMD_READ;
	cyhal_spi_transfer(spi_obj, &cmd, 1, NULL, 0, 0xFF); // data returned is not useful

	// Send the address
	uint8_t addr_bytes[2] = { (uint8_t)(address >> 8), (uint8_t)(address & 0xFF) };
	cyhal_spi_transfer(spi_obj, addr_bytes, 2, NULL, 0, 0xFF); // data returned is not useful

	// Read the data by sending a dummy byte (0xFF)
	uint8_t dont_care = 0xFF;
	uint8_t data;
	cyhal_spi_transfer(spi_obj, &dont_care, 1, &data, 1, 0xFF);

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);

	return data;
}
