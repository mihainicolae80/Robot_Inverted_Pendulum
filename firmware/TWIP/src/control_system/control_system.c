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
	float angle_old;
	float angle_sum;
	float bp, bi, bd;
	float angle_off;
	uint32_t system_time;
	uint32_t cal_time;
} _pid;


float _dev_bno_x;
float _dev_bno_y;
float _dev_bno_z;
uint8_t _dev_bno_cal_gyro;
uint8_t _dev_bno_cal_acc;



/* Set timer source or no source */
static inline void _timer_enable(bool enable)
{
	// Fast PWM Mode, TOP @OCR0A
	// Prescaled 1/1024
	TCCR2B = (uint8_t)enable * ((1u << WGM02) | (CONF_CTRL_CLK << CS20));
}


// Control system routine
ISR(TIMER2_COMPA_vect)
{
	BNO_angle_t angle_BNO;
	float angle;
	float cmd;
	
	// add 10ms
	_pid.system_time += 10;
	IND_iterate();	
	if (!_pid.running) {
		return;
	}
	
	// current angle
	angle_BNO = BNO_angle();
	angle = angle_BNO.z + _pid.angle_off;
	// start new readings
	BNO_start_reading();
	
	////// DEV //////////
	_dev_bno_x = angle_BNO.x;
	_dev_bno_y = angle_BNO.y;
	_dev_bno_z = angle_BNO.z;
	_dev_bno_cal_gyro = angle_BNO.cal_gyro;
	_dev_bno_cal_acc = angle_BNO.cal_acc;
	///////////////////
	
	// TODO: re-add this after testing calib saving to EEPROM
	/*
	// refresh calibration if calibration lvl decreases
	if ((3 != angle_BNO.cal_acc) || (3 != angle_BNO.cal_gyro)) {
		// disable control system timer
		// (prevent this interrupt)
		_timer_enable(false);
		// enable global interrupts for I2C to work
		sei();
		BNO_write_cal();
		cli();
		// re-enable control system timer
		_timer_enable(true);
	}
	*/
	
#if defined (SAFETY_ANGLE)
	// !!! safety stop !!!
	if (fabs(angle) > CONF_CTRL_MAX_ANGLE) {
		// stop routine and indicate error
		CTRL_stop();
		IND_set_mode(IND_ERROR);
		return;
	}
#endif

	// pass through PID
	cmd = (_pid.bp * angle)
		+  (_pid.bd * (_pid.angle_old - angle))
		+  (_pid.bi * _pid.angle_sum);
	// update PID
	_pid.angle_old = angle;
	// integral sum
	_pid.angle_sum += angle;
	// cap integral sum
	if (_pid.angle_sum > CONF_CTRL_PID_MAX_SUM) {
		_pid.angle_sum = CONF_CTRL_PID_MAX_SUM;
	} else if (_pid.angle_sum < -CONF_CTRL_PID_MAX_SUM) {
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
		} else if (cmd < -255.0f) {
			cmd = -255.0f;
		}
	}
		
	// send motor commands
	MOTOR_A_speed((int16_t)cmd);
	MOTOR_B_speed((int16_t)cmd);
}

void CTRL_init(void)
{
	struct calibration_t calib;
	
	_pid.running = false;
	_pid.system_time = 0;
	_pid.cal_time = 0;
	_pid.angle_off = 0;
	
	// Fast PWM Mode, TOP @OCR0A
	// Disabled output
	TCCR2A = 3u << WGM00;
	
	// TC TOP value
	OCR2A = CONF_CTRL_TIMER_CNT;
	
	// reset timer counter
	TCNT2 = 0;
		
	// Fast PWM Mode, TOP @OCR0A
	// Prescaled 1/1024
	//TCCR2B = (1u << WGM02) | (CONF_CTRL_CLK << CS20);
	_timer_enable(true);
	
	// enable OCRA compare interrupt
	TIMSK2 |= (1 << OCIE2A);
	
	// restore calibration from EEPROM
	EEPROM_read_array(0, (uint8_t *)&calib, sizeof(calib));
	if (CALIB_VALIDATION == calib.validation) {
		_pid.bp = calib.bp;
		_pid.bi = calib.bi;
		_pid.bd = calib.bd;
		_pid.angle_off = calib.angle_off;
		
	}
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

void CTRL_PID_start(void)
{	
	// do not start twice
	if (_pid.running) {
		return;
	}
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		_pid.running = true;
		
		// request vector data from BNO055 sensor
		BNO_start_reading();
		
		// Start motors
		MOTOR_A_speed(0);
		MOTOR_B_speed(0);
		MOTORS_on();
	}
}


void CTRL_stop(void)
{
	uint8_t flags = SREG;
	cli();
	
	// turn off the motors
	MOTORS_off();
	_pid.running = false;
	
	SREG = flags;
}

uint32_t CTRL_get_time(void)
{
	return _pid.system_time;
}


uint32_t CTRL_get_time_elapsed(uint32_t past)
{
	uint32_t present;
	
	present = _pid.system_time;
	
	return (past < present) 
		 ? (present - past) 
		 : (past - present);
}


int32_t CTRL_get_last_angle(void)
{
	return _pid.angle_old;
}

struct calibration_t CTRL_get_calib(void)
{
	struct calibration_t calib;
	
	
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

void CTRL_load_calib(void)
{
	struct calibration_t calib;
	
	// load from EEPROM
	EEPROM_read_array(0, (uint8_t *)&calib, sizeof(calib));
	
	// check if data corrupter
	if (CALIB_VALIDATION == calib.validation) {
		// pid parameters
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
