#include <avr/io.h>
#include "uart/uart.h"
#include <avr/interrupt.h>
#include "console/console.h"
#include "i2c/i2c.h"
#include "config/conf_board.h"
#include "util/delay.h"
#include "indicator/indicator.h"
#include "control_system/control_system.h"
#include "motors/motors.h"
#include "BNO055/BNO055.h"
#include "EEPROM/eeprom.h"


int main(void)
{	
	// init system peripherals
	I2C_init();
	UART_init();
	
	// init sub-systems
	MOTOR_init();
	CONS_init();
	CTRL_init();
	
	// init Control System parameters
	CTRL_set_PID_terms(55.0f, 3, 9.0f);
	CTRL_set_angle_off(-3.6f);
	
	// enable global interrupts
	sei();
	
	// init sensor BNO055
	BNO_init(true);
	// try to copy calibration data
	// from EEPROM to BNO055 sensor
	CTRL_load_calib_from_EEPROM();
	
	// enable button
	DDRB &= ~(1 << PORTB7);
	
	// LED indicate system Running
	IND_set_mode(IND_RUNNING);
	
	// start Control System
	CTRL_start();
	
	// main loop
	while (1) {
		// handle Console Commands
		CONS_handle();
		// handle Control System
		CTRL_handle();
		// on button press, re-enable system
		if (0 == (PINB & (1 << PORTB7))) {
			IND_set_mode(IND_RUNNING);
			CTRL_start();
		}
	}

	return 0;
}