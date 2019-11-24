/*
 * eeprom.c
 *
 * Created: 11/24/2019 5:10:34 PM
 *  Author: mihai
 */ 

#include "EEPROM/eeprom.h"

#include <avr/io.h>
#include <util/atomic.h>



uint8_t EEPROM_read(uint16_t addr)
{
	uint8_t data;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// wait for EEPROM Write completion
		while (EECR & (1u << EEPE))
			;
		// set address
		EEAR = addr;
		// do read
		EECR = (1u << EERE);
		data = EEDR;
	}
	
	return data;
}

void EEPROM_write(uint16_t addr, uint8_t data)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// wait for EEPROM Write completion
		while (EECR & (1u << EEPE))
			;
		// set address, data
		EEAR = addr;
		EEDR = data;
		// enable write
		EECR = (1u << EEMPE);
		EECR = (1u << EEPE);
	}
}

void EEPROM_write_array(uint16_t addr, uint8_t *data_ptr, uint16_t size)
{
	for (uint16_t i = 0; i < size; i++) {
		EEPROM_write(addr + i, data_ptr[i]);
	}
}

void EEPROM_read_array(uint16_t addr, uint8_t *data_ptr, uint16_t size)
{
	for (uint16_t i = 0; i < size; i++) {
		data_ptr[i] = EEPROM_read(addr + i);
	}
}
