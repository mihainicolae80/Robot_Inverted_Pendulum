/*
 * i2c.h
 *
 * Created: 3/29/2019 8:31:09 PM
 *  Author: mihai
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include <stdbool.h>

typedef volatile enum {
	// transmission ended successfully
	I2C_SUCCESS,
	// Addressed device did not ACK
	I2C_ADDRESS_NACK,
	// Slave refused byte
	I2C_WRITE_NACK,
	// Task in progress
	I2C_IN_PROGRESS
} I2C_stat_t;


/* Initialize the interrupt based 
 * I2C peripheral driver
*/
void I2C_init(void);

/* Returns the number of tasks that can be enqueued
 * in the internal task circular buffer 
 */
uint8_t I2C_available_tasks(void);

bool _I2C_enqueue_task(bool read, uint8_t addr, uint8_t *buff, uint16_t max_len, volatile I2C_stat_t *status);

/* Enqueue read task to the internal I2C task queue
 * Returns true if operation permitted.
 * Non blocking
 * Will write operation status in 'status' argument
 * as the transmission unfolds
 */
static inline bool I2C_read(uint8_t addr, uint8_t *buff, uint16_t max_len, volatile I2C_stat_t *status)
{
	return _I2C_enqueue_task(true, addr, buff, max_len, status);
}

/*
 * Enqueue a write task.
 * Similar to the read operation.
 */
static inline bool I2C_write(uint8_t addr, uint8_t *buff, uint16_t len, volatile I2C_stat_t *status)
{
	return _I2C_enqueue_task(false, addr, buff, len, status);
}

#endif /* I2C_H_ */