/*
 * control_system.h
 *
 * Created: 4/6/2019 3:11:36 PM
 *  Author: mihai
 */ 


#ifndef CONTROL_SYSTEM_H_
#define CONTROL_SYSTEM_H_

#include <stdint.h>

#include "BNO055/BNO055.h"


#define CALIB_VALIDATION (0xa5a5a5a5u)


struct CTRL_calib_t {
	uint32_t validation;
	// PID terms
	float bp, bi, bd;
	// gyro sensor angle offset
	float angle_off;
	// BNO055 calibration
	uint8_t bno[MAG_RADIUS_MSB_ADDR - ACCEL_OFFSET_X_LSB_ADDR + 1];
};


/* Init the automatic control system */
void CTRL_init(void);

/* Start the automatic control system */
void CTRL_start(void);

/* Stop the automatic control system */
void CTRL_stop(void);

/* Execute an iteration of the control system */
void CTRL_handle(void);


/* Set the terms of the PID controller 
*/
void CTRL_set_PID_terms(float bp, float bi, float bd);

void CTRL_set_PID_bp(float bp);

void CTRL_set_PID_bi(float bi);

void CTRL_set_PID_bd(float bd);

void CTRL_set_angle_off(float off);


/* Return last read angle */
int32_t CTRL_get_last_angle(void);

struct CTRL_calib_t CTRL_get_calib(void);


/* Attempt to load the calibration data form EEPROM */
void CTRL_load_calib_from_EEPROM(void);


/* Get current system time in ms.
 * System time is incremented with 20ms at a time.
*/
uint32_t CTRL_get_time_ms(void);

/* Compute elapsed time since a time in the past 
 * (20 ms resolution)
*/
uint32_t CTRL_get_elapsed_ms(uint32_t past);



#endif /* CONTROL_SYSTEM_H_ */