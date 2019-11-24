/*
 * motors.h
 *
 * Created: 4/6/2019 2:04:56 PM
 *  Author: mihai
 */ 


#ifndef MOTORS_H_
#define MOTORS_H_

#include <stdint.h>
#include <avr/io.h>
#include "config/conf_motor.h"


/* Init Timer 0 to PWD signal and direction pins */
void MOTOR_init(void);

/* Set motor A speed and direction. 
 * Accepts a value between -255 and 255 
 */
void MOTOR_A_speed(int16_t speed);


/* Set motor A speed and direction. 
 * Accepts a value between -255 and 255 
 */
void MOTOR_B_speed(int16_t speed);

/* Start both motors */
static inline void MOTORS_on(void)
{
	TCCR0B = CONF_MOTOR_CLK;
	DDRD |= (1 << PORTD6) | (1 << PORTD5);
}


/* Stop both motors */ 
static inline void MOTORS_off(void)
{
	TCCR0B = TC0_CLK_OFF;
	DDRD &= ~(1 << PORTD6) & ~(1 << PORTD5);
}

#endif /* MOTORS_H_ */