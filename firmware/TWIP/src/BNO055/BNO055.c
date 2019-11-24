/*
 * BNO055.c
 *
 * Created: 3/30/2019 6:10:36 PM
 *  Author: mihai
 */ 
#include "BNO055/BNO055.h"
#include "config/conf_board.h"
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/conf_BNO055.h"
#include "console/console.h"
#include "indicator/indicator.h"
#include <avr/common.h>
#include "uart/uart.h"
#include "control_system/control_system.h"


static uint8_t _addr_angle = VECTOR_EULER;
static uint8_t _addr_calib = BNO055_CALIB_STAT_ADDR;
static struct {
	uint8_t mode;
	uint8_t angle_buff[6];
	uint8_t calib_state;
	I2C_stat_t _stat_r, _stat_w;
} _local;


bool BNO_start_reading(void)
{
	uint8_t flags;
	
	flags = SREG;
	cli();
	if (I2C_available_tasks() < 2) {
		// fail
		SREG = flags;
		return false;	
	}
	
	// request heading, pitch, roll
	I2C_write(CONF_BNO055_ADDRESS, &_addr_angle, 1, &_local._stat_w);
	I2C_read(CONF_BNO055_ADDRESS, _local.angle_buff, 6, &_local._stat_r);
	// request calibration state
	I2C_write(CONF_BNO055_ADDRESS, &_addr_calib, 1, &_local._stat_w);
	I2C_read(CONF_BNO055_ADDRESS, &(_local.calib_state), 1, &_local._stat_r);	
	
	SREG = flags;
	return true;
}


void BNO_read_calib_levels(uint8_t *lvl_sys, uint8_t *lvl_gyro, uint8_t *lvl_acc, uint8_t *lvl_mag)
{
	uint8_t data;
	
	// read calibration register
	BNO_read_reg(BNO055_CALIB_STAT_ADDR, &data, 1);
	
	*lvl_sys = (data >> 6u) & 3u;
	*lvl_gyro = (data >> 4u) & 3u;
	*lvl_acc = (data >> 2u) & 3u;
	*lvl_mag = (data) & 3u;
}

void BNO_read_print_cal(void)
{
	uint8_t buff[22];
	int i;
	bno055_reg_t reg;
	
	for (reg = ACCEL_OFFSET_X_LSB_ADDR, i = 0;
	reg <= MAG_RADIUS_MSB_ADDR;
	reg ++, i ++) {
		// read reg from BNO055
		BNO_read_reg(reg, buff + i, 1);
		// write raw data to UART 
		UART_write(buff[i]);
	}
}

BNO_angle_t BNO_angle(void)
{
	uint8_t flags;
	BNO_angle_t angle;
	float fx, fy, fz;
	
	flags = SREG;
	cli();
	
	
	// convert to angle
	fx = ((int16_t)_local.angle_buff[0]) | ((int16_t)_local.angle_buff[1] << 8);
	fy = ((int16_t)_local.angle_buff[2]) | ((int16_t)_local.angle_buff[3] << 8);
	fz = ((int16_t)_local.angle_buff[4]) | ((int16_t)_local.angle_buff[5] << 8);	
	// to obtain the real angle: angle = float_angle / 16.0
	// but the angle.x (and .y and .z) will store the angle * 1024
	// so: raw_angle / 16 * 1024 = raw_angle * 64
	

	// angle * 1024
	/*
	angle.x = (int32_t)(fx * 64);
	angle.y = (int32_t)(fy * 64);
	angle.z = (int32_t)(fz * 64);
	*/
	
	// real angle (float)
	angle.x = (fx / 16.0f);
	angle.y = (fy / 16.0f);
	angle.z = (fz / 16.0f);
	
	// calibration data
	angle.cal_sys = (_local.calib_state >> 6u) & 3u;
	angle.cal_gyro = (_local.calib_state >> 4u) & 3u;
	angle.cal_acc = (_local.calib_state >> 2u) & 3u;
	angle.cal_mag = (_local.calib_state) & 3u;
	
	SREG = flags;
	return angle;
}



int32_t BNO_read_reg(bno055_reg_t reg, uint8_t *data, uint8_t size)
{
	volatile I2C_stat_t status_w, status_r;
	bool enq;
	
	// write reg address
	enq = I2C_write(CONF_BNO055_ADDRESS, (uint8_t*)&reg, 1, &status_w);
	if (!enq) {
		return -1;
	}
	
	while (I2C_IN_PROGRESS == status_w) 
		;
		
	if (I2C_SUCCESS != status_w) {
		return status_w;
	}
	
	// read reg
	I2C_read(CONF_BNO055_ADDRESS, data, size, &status_r);
	// wait for write and read to complete
	while (I2C_IN_PROGRESS == status_r)
		;
	// check Read status
	if (I2C_SUCCESS != status_r) {
		return status_r;
	}
	
	// (value already written to destination)
	return I2C_SUCCESS;
}

int32_t BNO_write_reg(bno055_reg_t reg, uint8_t data)
{
	volatile I2C_stat_t status_w;
	uint8_t buff[2];
	
	buff[0] = reg;
	buff[1] = data;
	
	// write reg address
	I2C_write(CONF_BNO055_ADDRESS, buff, 2, &status_w);
	// wait for write to complete
	while (I2C_IN_PROGRESS == status_w)
		;
	
	return status_w;
}

/*
static int32_t _set_mode(uint8_t mode)
{
	I2C_stat_t status;
	
	_local.mode = mode;
	status = _write_reg(BNO055_OPR_MODE_ADDR, mode);
	_delay_ms(30);
	
	return status;
}
*/




int32_t BNO_init(bool block_on_failure)
{
	int32_t error = I2C_SUCCESS; 
	uint8_t id;
	
  
	// wait for boot
	_delay_ms(1000);
	
	// make sure the ADDRESS is correct and the sensor is present
	BNO_read_reg(BNO055_CHIP_ID_ADDR, &id, 1);	
	if(id != BNO055_ID) {
		error = -1;
		goto _gyro_init_err;
	}

	// Switch to config mode
	// and wait for switch
	BNO_write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
	_delay_ms(25);

	/* Set Power Mode: Normal */
	BNO_write_reg(BNO055_PWR_MODE_ADDR, POWER_MODE_NORMAL);
	_delay_ms(25);

	// select register page 0
	// (in order to access sensor data)
	BNO_write_reg(BNO055_PAGE_ID_ADDR, 0);

	/*
	// set calibration data to Zero
	for (bno055_reg_t reg = ACCEL_OFFSET_X_LSB_ADDR;
		reg <= MAG_RADIUS_MSB_ADDR;
		reg ++) {
		BNO_write_reg(reg, _calib_data[reg - ACCEL_OFFSET_X_LSB_ADDR]);
		//BNO_write_reg(reg, 0);
	}
	*/


	// TODO: re-add after adding calib save to EEPROM
	//BNO_write_cal();

	// Set Sensor Mode
	BNO_write_reg(BNO055_OPR_MODE_ADDR, CONF_BNO055_MODE);
	_delay_ms(25);

	return I2C_SUCCESS;

_gyro_init_err:
	if (block_on_failure) {
		IND_set_mode(IND_ERROR);
		while (1)
			;
	} 
	
	return error;
	
	// == Only Removed Code ==
	/*	
	// reset device
	_write_reg(BNO055_SYS_TRIGGER_ADDR, 0x20);
	
	// wait for device to be ready
	do {
		_read_reg(BNO055_CHIP_ID_ADDR, &data, 1);
	} while (data != BNO055_ID);
	
	// Set to normal power mode
	_write_reg(BNO055_PWR_MODE_ADDR, POWER_MODE_NORMAL);

	// select page 0
	_write_reg(BNO055_PAGE_ID_ADDR, 0);
	*/

	/* Set the output units */
	/*
	uint8_t unitsel = (0 << 7) | // Orientation = Android
	(0 << 4) | // Temperature = Celsius
	(0 << 2) | // Euler = Degrees
	(1 << 1) | // Gyro = Rads
	(0 << 0);  // Accelerometer = m/s^2
	write8(BNO055_UNIT_SEL_ADDR, unitsel);
	*/

	/* Configure axis mapping (see section 3.4) */
	/*
	write8(BNO055_AXIS_MAP_CONFIG_ADDR, REMAP_CONFIG_P2); // P0-P7, Default is P1
	_delay_ms(10);
	write8(BNO055_AXIS_MAP_SIGN_ADDR, REMAP_SIGN_P2); // P0-P7, Default is P1
	_delay_ms(10);
	*/
  
  
	//_write_reg(BNO055_SYS_TRIGGER_ADDR, 0x0);
	//_delay_ms(10);
	
	/* Set the requested operating mode (see section 3.3) */
	//_set_mode(mode);
	//_write_reg(BNO055_OPR_MODE_ADDR, CONF_BNO055_MODE);
	//_delay_ms(25);
	
	//return I2C_SUCCESS;
}


void BNO_print_calib_on_change(void)
{
	static uint8_t  lvl_sys_old = 100, lvl_gyro_old = 100,
	lvl_acc_old = 100, lvl_mag_old = 100;
	static uint32_t calib_time = 0;
	
	// read calibration status
	if (CTRL_get_time_elapsed(calib_time) > 100) {
		uint8_t lvl_sys, lvl_gyro, lvl_acc, lvl_mag;
		
		calib_time = CTRL_get_time();
		BNO_read_calib_levels(&lvl_sys, &lvl_gyro, &lvl_acc, &lvl_mag);
		
		if (lvl_sys != lvl_sys_old) {
			print("cal sys=%d\n", lvl_sys);
			lvl_sys_old = lvl_sys;
		}
		if (lvl_gyro != lvl_gyro_old) {
			print("cal gyro=%d\n", lvl_gyro);
			lvl_gyro_old = lvl_gyro;
		}
		if (lvl_acc != lvl_acc_old) {
			print("cal acc=%d\n", lvl_acc);
			lvl_acc_old = lvl_acc;
		}
		if (lvl_mag != lvl_mag_old) {
			print("cal mag=%d\n", lvl_mag);
			lvl_mag_old = lvl_mag;
		}
	}
}

void BNO_print_data(void)
{
	static uint32_t _time = 0;
	BNO_angle_t angle;
	uint8_t lvl_sys, lvl_gyro, lvl_acc, lvl_mag;
	
	if (CTRL_get_time_elapsed(_time) > 300) {
		_time = CTRL_get_time();
		
		// request data
		BNO_start_reading();
		// wait for data
		while (I2C_IN_PROGRESS == _local._stat_r)
		;
		// print angle
		if (I2C_SUCCESS == _local._stat_r) {
			angle = BNO_angle();
			print("============\n\r");
			print("x=%f\n\r", angle.x);
			print("y=%f\n\r", angle.y);
			print("z=%f\n\r", angle.z);
			BNO_read_calib_levels(&lvl_sys, &lvl_gyro, &lvl_acc, &lvl_mag);
			print("c_s=%d\n\r", lvl_sys);
			print("c_g=%d\n\r", lvl_gyro);
			print("c_a=%d\n\r", lvl_acc);
			print("c_m=%d\n\r", lvl_mag);
		} else {
			UART_write_str("BNO: Failed to read data\r\n");
		}
	}
}

bool _BNO_test_1(void)
{
	I2C_stat_t status;
	uint8_t data;
	bool enq;
	
	print("== BNO055 Test 1: Read ==\n");
	// wait for I2C to be ready
	while (!I2C_available_tasks())
		;
	
	print("Writing reg address\n");
	data = BNO055_CHIP_ID_ADDR;
	enq = I2C_write(CONF_BNO055_ADDRESS, &data, 1, &status);
	if (enq) {
		print("Task added to queue\n");
		} else {
		print("Failed to add to queue\n");
		return false;
	}
	
	while (I2C_IN_PROGRESS == status)
	;
	print("DONE\n");
	
	if (status != I2C_SUCCESS) {
		// handle errors
		if (I2C_ADDRESS_NACK == status) {
			print("I2C: NACK\n");
			} else if (I2C_WRITE_NACK == status) {
			print("I2C: WRITE NACK\n");
		}
		return status;
	}
	
	print("Reading reg value .. ");
	data = 0;
	I2C_read(CONF_BNO055_ADDRESS, &data, 1, &status);
	while (I2C_IN_PROGRESS == status)
	;
	print("DONE\n");
	
	if (I2C_SUCCESS != status) {
		// handle errors
		if (I2C_ADDRESS_NACK == status) {
			print("I2C: NACK\n");
			} else if (I2C_WRITE_NACK == status) {
			print("I2C: WRITE NACK\n");
		}
		return status;
	}
	
	print("RW SUCCESS!\n");
	
	print("Check Value .. ");
	
	if (BNO055_ID == data) {
		print("OK\n");
		} else {
		print("FAIL=%x\n", data);
	}
	
	return I2C_SUCCESS;
}



bool _BNO_test_2(void)
{
	uint8_t data;
	I2C_stat_t stat;
	
	
	print(" == BNO: Test 2 ==\n");
	data = 0;
	stat = BNO_read_reg(BNO055_CHIP_ID_ADDR, &data, 1);
	
	if (I2C_SUCCESS != stat) {
		print("I2C error=%d\n", stat);
		} else {
		if (BNO055_ID == data) {
			print("OK\n");
			return true;
			} else {
			print("FAIL val=%x\n", data);
		}
	}
	
	
	return false;
}


bool _BNO_test_3(void)
{
	I2C_stat_t stat;
	
	print("BNO: init\n");
	stat = BNO_init(false);
	if (I2C_SUCCESS != stat) {
		IND_set_mode(IND_ERROR);
		// loop forever
		while (1)
			;
	}
	print("BNO: OK\n");
	
	return true;
}


bool _BNO_test_4(void)
{
	int i;
	uint8_t data;
	
	BNO_write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
	_delay_ms(30);
	
	
	// FIX ME PLS!!!
	// 160 != 0
	for (i = 0; i <= 12; i++) {
		data = 0xff;
		BNO_write_reg(GYRO_OFFSET_Z_LSB_ADDR, i);
		BNO_read_reg(GYRO_OFFSET_Z_LSB_ADDR, &data, 1);
		
		if (data == i) {
			print("%d == %d\n", data, i);
			} else {
			print("%d != %d\n", data, i);
		}
		
		_delay_ms(300);
	}
	
	BNO_write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_NDOF);
	
	return true;
}

bool _BNO_test_5(void)
{
	uint8_t data;
	
	BNO_read_reg(BNO055_CHIP_ID_ADDR, &data, 1);
	
	return true;
}