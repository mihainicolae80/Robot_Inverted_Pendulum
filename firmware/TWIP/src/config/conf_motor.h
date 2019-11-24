/*
 * conf_motor.h
 *
 * Created: 4/6/2019 2:27:33 PM
 *  Author: mihai
 */ 


#ifndef CONF_MOTOR_H_
#define CONF_MOTOR_H_

#include "timers/timers.h"

/* Motor time (TC0) will be set in Fast PWM-Mode.
 * The final frequency will be:
 * F_MOTOR = F_CPU / 256 / CONF_MOTOR_CLK
 * Current: 160000 / 256 / 64 = ~976Hz
 */
#define CONF_MOTOR_CLK (TC0_CLK_64)

// Command to set the dir of motor A to Forward
#define CONF_MOTOR_A_DIR_FW() (PORTB |= (1u << PB0));

// Command to set the dir of motor A to Backwards
#define CONF_MOTOR_A_DIR_BW() (PORTB &= ~(1u << PB0));

// Command to set the dir of motor B to Forward
#define CONF_MOTOR_B_DIR_FW() (PORTB &= ~(1u << PB1));

// Command to set the dir of motor B to Backwards
#define CONF_MOTOR_B_DIR_BW() (PORTB |= (1u << PB1));

#endif /* CONF_MOTOR_H_ */