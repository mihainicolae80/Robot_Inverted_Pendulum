/*
 * indicator.c
 *
 * Created: 4/6/2019 2:57:50 PM
 *  Author: mihai
 */ 


#include "indicator/indicator.h"
#include <avr/io.h>
#include "config/conf_board.h"
#include "control_system/control_system.h"
#include <stdbool.h>

static struct {
	uint32_t delay_on_ms;
	uint32_t delay_off_ms;
	uint32_t last_change;
	bool led_on;
} _local;

void IND_set_mode(indicator_mode_t mode)
{
	// enable led output
	DDRB |= (1 << PORTB5);
	_local.led_on = true;
	
	switch (mode) {
		case IND_BOOTING:
			PORTB |= (1 << PORTB5);
			break;
		case IND_RUNNING:
			_local.delay_on_ms = 500;
			_local.delay_off_ms = 500;
			break;
		case IND_ERROR:
			_local.delay_on_ms = 50;
			_local.delay_off_ms = 500;
			break;
		case IND_SIGNAL:
			_local.delay_on_ms = 50;
			_local.delay_off_ms = 50;
			break;
	}
	
	// force reset
	if (_local.led_on) {
		_local.last_change = CTRL_get_time_ms() - _local.delay_on_ms;
	} else {
		_local.last_change = CTRL_get_time_ms() - _local.delay_off_ms;
	}
}

void IND_iterate(void)
{
	if (_local.led_on) {
		if (CTRL_get_elapsed_ms(_local.last_change) > _local.delay_on_ms) {
			_local.led_on = false;
			DDRB &= ~(1 << PORTB5);
			_local.last_change = CTRL_get_time_ms();
		}
	} else {
		if (CTRL_get_elapsed_ms(_local.last_change) > _local.delay_off_ms) {
			_local.led_on = true;
			DDRB |= (1 << PORTB5);
			_local.last_change = CTRL_get_time_ms();
		}
	}
}