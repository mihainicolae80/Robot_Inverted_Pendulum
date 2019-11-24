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



int main(void)
{	
	// init system peripherals
	I2C_init();
	UART_init();
	
	// init sub-systems
	MOTOR_init();
	CONS_init();
	CTRL_init();
	
	// init default ctrl system parameters
	CTRL_set_PID_terms(55.0f, 3, 9.0f);
	CTRL_set_angle_off(-3.6f);
	
	// enable global interrupts
	sei();
	
	// load calibration from EEPROM
	CTRL_load_calib();
	
	BNO_init(true);
	CTRL_PID_start();
	// enable button
	DDRB &= ~(1 << PORTB7);
	
	// announce STARTED
	IND_set_mode(IND_RUNNING);
	
	// main loop
	uint32_t _time = CTRL_get_time();
	while (1) {
		CONS_handle();
		// on button press, re-enable system
		if (0 == (PINB & (1 << PORTB7))) {
			IND_set_mode(IND_RUNNING);
			CTRL_PID_start();
		}
		
		if (CTRL_get_time_elapsed(_time) > 500) {
			_time = CTRL_get_time();
			print("============\n\r");
			print("x=%f\n\r", _dev_bno_x);
			print("y=%f\n\r", _dev_bno_y);
			print("z=%f\n\r", _dev_bno_z);
			print("c_g=%d\n\r", _dev_bno_cal_gyro);
			print("c_a=%d\n\r", _dev_bno_cal_acc);
		}
	}

	return 0;
}