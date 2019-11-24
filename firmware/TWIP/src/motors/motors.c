/*
 * motors.c
 *
 * Created: 4/6/2019 2:05:03 PM
 *  Author: mihai
 */ 


#include "motors/motors.h"
#include "config/conf_motor.h"


void MOTOR_init(void)
{
	MOTORS_off();
	
	// Fast PWM Mode (TOP = 0xFF)
	// Set OC0A, OC0B @BOTTOM
	// Clear OC0A, OC0B @compare
	// (non-inverting mode)
	TCCR0A = (2u << COM0A0) | (2u << COM0B0) | (3u << WGM00);
	
	// reset counter value 
	TCNT0 = 0;
	
	// default compare values
	OCR0A = 50;
	OCR0B = 50;
	
	// desable all interrupts
	TIMSK0 = 0;
	
	// enable output pins
	// in PD5, PD6
	DDRB |= (1 << PORTB0) | (1 << PORTB1);
}


void MOTOR_A_speed(int16_t speed)
{
	if (speed > 0) {
		OCR0A = speed;
		CONF_MOTOR_A_DIR_FW();
	} else if (speed <= 0) {
		OCR0A = -speed;
		CONF_MOTOR_A_DIR_BW();
	}
}


void MOTOR_B_speed(int16_t speed)
{
	if (speed > 0) {
		OCR0B = speed;
		CONF_MOTOR_B_DIR_FW();
	} else if (speed <= 0) {
		OCR0B = -speed;
		CONF_MOTOR_B_DIR_BW();
	}
}