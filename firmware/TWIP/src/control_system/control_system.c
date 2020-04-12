/*
 * control_system.c
 *
 * Created: 4/6/2019 3:12:00 PM
 *  Author: mihai
 */ 

#include "control_system/control_system.h"

#include <stdbool.h>
#include <math.h>

#include "config/conf_board.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/common.h>
#include <util/delay.h>

#include "timers/timers.h"
#include "config/conf_control_system.h"
#include "motors/motors.h"
#include "BNO055/BNO055.h"
#include "indicator/indicator.h"
#include "EEPROM/eeprom.h"
#include "config/conf_BNO055.h"


#define _ABS(x) ((x) < 0 ? (-(x)) : (x))

static volatile struct {
	bool running;
	bool motor_control;
	float angle_old;
	float angle_sum;
	float bp, bi, bd;
	float angle_off;
	uint32_t system_time;
	uint32_t cal_time;
	// status for I2C operation
	I2C_stat_t read_handle_angle;
	I2C_stat_t read_handle_calib;
} _pid;




/* Set timer source or no source */
static inline void _timer_set_enable(bool enable)
{
	// Fast PWM Mode, TOP @OCR0A
	// Prescaler 1/1024
	TCCR2B = (uint8_t)enable * ((1u << WGM02) | (CONF_CTRL_CLK << CS20));
}


// Control system routine
ISR(TIMER2_COMPA_vect)
{	
	// add 10ms to system time
	_pid.system_time += 1;
	// update indicator LED
	IND_iterate();
}

void CTRL_init(void)
{	
	_pid.system_time = 0;
	_pid.cal_time = 0;
	_pid.angle_off = 0;
	_pid.motor_control = false;
	// init I2C read handler
	_pid.read_handle_angle = I2C_ADDRESS_NACK;
	_pid.read_handle_calib = I2C_ADDRESS_NACK;
	
	// Fast PWM Mode, TOP @OCR0A
	// Disabled output
	TCCR2A = 3u << WGM00;
	
	// TC TOP value
	OCR2A = CONF_CTRL_TIMER_CNT;
	
	// reset timer counter
	TCNT2 = 0;
		
	// Fast PWM Mode, TOP @OCR0A
	// Prescaler 1/1024
	//TCCR2B = (1u << WGM02) | (CONF_CTRL_CLK << CS20);
	_timer_set_enable(true);
	
	// enable OCRA compare interrupt
	TIMSK2 |= (1 << OCIE2A);
}

void CTRL_set_PID_terms(float bp, float bi, float bd)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		_pid.bp = bp;
		_pid.bi = bi;
		_pid.bd = bd;
	}
}

void CTRL_set_PID_bp(float bp)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		_pid.bp = bp;
	}
}

void CTRL_set_PID_bi(float bi)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		_pid.bi = bi;	
	}
}

void CTRL_set_PID_bd(float bd)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		_pid.bd = bd;
	}
}

void CTRL_set_angle_off(float off)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		_pid.angle_off = off;
	}
}

void CTRL_start(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (!_pid.motor_control) {
			
			// wait for previous data requests (angle, calib)
			while (I2C_IN_PROGRESS == _pid.read_handle_angle)
				; // wait
			while (I2C_IN_PROGRESS == _pid.read_handle_calib)
				; // wait
			
			// request new data (angle, calibration)
			BNO_request_angle(&(_pid.read_handle_angle));
			BNO_request_calib(&(_pid.read_handle_calib));
			
			// Start motors
			MOTOR_A_speed(0);
			MOTOR_B_speed(0);
			MOTORS_on();	
			
			// enable control system output to motors
			_pid.motor_control = true;
		}
	}
}


void CTRL_stop(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// turn off the motors
		MOTORS_off();
		// disable motor control
		_pid.motor_control = false;
		// Indicator LED to ERROR
		IND_set_mode(IND_ERROR);
	}
}


static inline void _PID_handle(float angle)
{
	float cmd;
	
	
#if defined (SAFETY_ANGLE)
	// !!! safety stop !!!
	if (fabs(angle) > CONF_CTRL_MAX_ANGLE) {
		// stop routine and indicate error
		CTRL_stop();
	}
#endif

	// motor control is disabled
	if (!_pid.motor_control) {
		MOTOR_A_speed(0);
		MOTOR_B_speed(0);
		MOTORS_off();
		return;
	}

	// pass through PID
	cmd =  (_pid.bp * angle)
		+  (_pid.bd * (_pid.angle_old - angle))
		+  (_pid.bi * _pid.angle_sum);
	// update PID
	_pid.angle_old = angle;
	// integral sum
	_pid.angle_sum += angle;
	// cap integral sum
	if (_pid.angle_sum > CONF_CTRL_PID_MAX_SUM) {
		_pid.angle_sum = CONF_CTRL_PID_MAX_SUM;
	}
	else if (_pid.angle_sum < -CONF_CTRL_PID_MAX_SUM) {
		_pid.angle_sum = -CONF_CTRL_PID_MAX_SUM;
	}
	
	// zero angle
	if (fabs(cmd) < CONF_CTRL_PID_MIN_CMD) {
		cmd = 0;
		// turn off PWM
		DDRB &= ~((1 << PORTB0) | (1 << PORTB1));
		} else {
		// enable PWM
		DDRB |= (1 << PORTB0) | (1 << PORTB1);
		// cap between -255 and 255
		if (cmd > 255.0f) {
			cmd = 255.0f;
		}
		else if (cmd < -255.0f) {
			cmd = -255.0f;
		}
	}
	
	// send motor commands
	MOTOR_A_speed((int16_t)cmd);
	MOTOR_B_speed((int16_t)cmd);
}

void CTRL_handle(void)
{
	static uint32_t timer_PID = 0;
	static uint32_t timer_calib = 0;
	BNO_angle_t BNO_angle_data;
	BNO_calib_t BNO_calib_data;
	float angle;
	
	
	// handle once every 10ms
	if (CTRL_get_elapsed_ms(timer_PID) > CONF_CTRL_PID_LOOP_INTERVAL) {
		timer_PID = CTRL_get_time_ms();
		
		///////// {   PID   } //////////
		// if angle data not ready
		while((I2C_IN_PROGRESS == _pid.read_handle_angle))
			; // wait
		
		// current angle + calibration offset
		BNO_angle_data = BNO_angle();
		angle = BNO_angle_data.z + _pid.angle_off;
		// request new reading
		BNO_request_angle(&(_pid.read_handle_angle));
		// run PID and command motors
		_PID_handle(angle);
	}
	
	
	// TOOD: add calib streaming condition
	////////// {   Calibration Levels Streaming   } /////
	if (false && (CTRL_get_elapsed_ms(timer_calib) > CONF_CTRL_CALIB_LOOP_INTERVAL)) {
		// reset timer
		timer_calib = CTRL_get_time_ms();
		// wait for data to be ready
		while (I2C_IN_PROGRESS == _pid.read_handle_calib)
			; // wait
		
		// get latest calibration data
		BNO_calib_data = BNO_calib();
		// request new data for next iteration
		BNO_request_calib(&(_pid.read_handle_calib));
		
		// re-apply calibration from EEPROM
		// if sensor calibration levels decreased
		if ((3 != BNO_calib_data.cal_acc) || (3 != BNO_calib_data.cal_gyro)) {
			// attempt to load calibration from EEPROM
			CTRL_load_calib_from_EEPROM();
		}
		
		// TODO: send calibration data on UART
	}
}

uint32_t CTRL_get_time_ms(void)
{
	return _pid.system_time;
}


uint32_t CTRL_get_elapsed_ms(uint32_t past)
{	
	uint32_t present = _pid.system_time;
	
	return (past < present) 
		 ? (present - past) 
		 : (past - present);
}


int32_t CTRL_get_last_angle(void)
{
	return _pid.angle_old;
}

struct CTRL_calib_t CTRL_get_calib(void)
{
	struct CTRL_calib_t calib;
	
	// set calibration as valid
	calib.validation = CALIB_VALIDATION;
	// PID coeficients
	calib.bp = _pid.bp;
	calib.bi = _pid.bi;
	calib.bd = _pid.bd;
	// IMU sensor offset
	calib.angle_off = _pid.angle_off;
	// IMU sensor calibration
	BNO_read_reg(
		ACCEL_OFFSET_X_LSB_ADDR, 
		(uint8_t *)&(calib.bno), 
		MAG_RADIUS_MSB_ADDR - ACCEL_OFFSET_X_LSB_ADDR + 1
	);
	
	return calib;
}

void CTRL_load_calib_from_EEPROM(void)
{
	struct CTRL_calib_t calib;
	
	// load from EEPROM
	EEPROM_read_array(0, (uint8_t *)&calib, sizeof(calib));
	
	// Check data VALID
	if (CALIB_VALIDATION == calib.validation) {
		// set PID parameters
		_pid.bp = calib.bp;
		_pid.bi = calib.bi;
		_pid.bd = calib.bd;
		_pid.angle_off = calib.angle_off;
		
		// BNO055 enter config mode
		BNO_write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
		_delay_ms(30);
		
		// select register page 0
		BNO_write_reg(BNO055_PAGE_ID_ADDR, 0);
		
		// write BNO055 calibration
		for (uint16_t i = ACCEL_OFFSET_X_LSB_ADDR;
		i <= MAG_RADIUS_MSB_ADDR;
		i ++) {
			BNO_write_reg(i, calib.bno[i - ACCEL_OFFSET_X_LSB_ADDR]);	
		}
		
		// restore previous mode
		BNO_write_reg(BNO055_OPR_MODE_ADDR, CONF_BNO055_MODE);
		_delay_ms(30);
	}
}
