/*
 * eeprom.h
 *
 * Created: 11/24/2019 5:10:23 PM
 *  Author: mihai
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

uint8_t EEPROM_read(uint16_t addr);

void EEPROM_write(uint16_t addr, uint8_t data);

void EEPROM_write_array(uint16_t addr, uint8_t *data_ptr, uint16_t size);

void EEPROM_read_array(uint16_t addr, uint8_t *data_ptr, uint16_t size);

#endif /* EEPROM_H_ */