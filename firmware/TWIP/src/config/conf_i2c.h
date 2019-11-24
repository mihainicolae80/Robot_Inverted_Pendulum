/*
 * conf_i2c.h
 *
 * Created: 3/29/2019 8:33:24 PM
 *  Author: mihai
 */ 


#ifndef CONF_I2C_H_
#define CONF_I2C_H_

// !! must be power of two !!
#define CONF_I2C_TASK_BUFF_SIZE (8u)

// I2C frequency (100Kbps / 400Kbps)
// !! Configured as 400Kbps !!
// computed by formula: I2C_freq = CPU_freq / ()
// I2C_freq = CPU_freq / (16 + 2 * I2C_BIT_RATE_REG *I2C_PRESCALER)
#define CONF_I2C_BIT_RATE_REG	(3u)

/* Reg value -> prescaler value
 *		0	->	1	
 *		1	->	4
 *		2	->	16
 *		3	->	64
 */
#define CONF_I2C_PRESCALER		(1u)


#endif /* CONF_I2C_H_ */