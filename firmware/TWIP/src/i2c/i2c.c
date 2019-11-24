/*
 * i2c.c
 *
 * Created: 3/29/2019 8:31:21 PM
 *  Author: mihai
 */ 

#include "i2c/i2c.h"
#include <avr/io.h>
#include "config/conf_i2c.h"
#include <avr/interrupt.h>
#include "console/console.h"
#include "uart/uart.h"
#include "config/conf_board.h"
#include <util/delay.h>
#include <avr/common.h>

#define _EN_TRANSFER ((1 << TWINT)\
					| (1 << TWEN)\
					| (1 << TWIE))

typedef struct {
	uint8_t *data;
	uint8_t addr;
	uint8_t len;
	volatile I2C_stat_t *status_ptr;
	bool read;
} _task_t;


static volatile struct {
	_task_t tasks[CONF_I2C_TASK_BUFF_SIZE];
	uint8_t task_data_index;
	uint8_t task_head, task_tail;
	int8_t tasks_num;
} _local;


/* Crt task failed, mark as failed, send stop condition
 * and start next one if available */
static void _task_ended(void)
{
	//UART_write_str("STO\n\r");
	// send stop condition
	TWCR = _EN_TRANSFER | (1 << TWSTO);
	// remove task from queue
	_local.task_head = (_local.task_head + 1) & (CONF_I2C_TASK_BUFF_SIZE - 1);
	_local.tasks_num --;
	// if other tasks available, start next one
	if (_local.tasks_num) {
		//UART_write_str("STA\n\r");
		//_delay_ms(500);
		TWCR = _EN_TRANSFER | (1 << TWSTA);
	}
}

// handle I2C events
ISR(TWI_vect)
{
	volatile _task_t *crt_task_ptr;
	// if there is at least one task active
	if (_local.tasks_num > 0) {
		// pointer to crt task
		crt_task_ptr = &(_local.tasks[_local.task_head]);
		// Read task
		if (crt_task_ptr->read) {
			// react on transmission status
			switch (TWSR & 0xF8u) {
				
				// START/REPEATED START transmitted
				case 0x08:
				case 0x10:
					// add slave address and R bit
					_local.task_data_index = 0;
					TWDR = ((crt_task_ptr->addr) << 1) | 1u;
					TWCR = _EN_TRANSFER;
					//UART_write_str("_STA_R\n\r");
					break;
					
				// SLA+R transmitted
				// NACK received
				case 0x48:
					// set status to NACK
					*(crt_task_ptr->status_ptr) = I2C_ADDRESS_NACK;
					//UART_write_str("_NA_R\n\r");
					// end transmit task
					_task_ended();
					break;
					
				// Data byte received by master
				// ACK sent
				case 0x50:
				// NACK sent
				case 0x58:
					crt_task_ptr->data[_local.task_data_index++] = TWDR;
					//UART_write_str("RXD\r\n");
					// ! Fall through !
				// SLA+R+ACK transmitted
				case 0x40:
					// more data expected
					if (_local.task_data_index < crt_task_ptr->len) {
						// request more data
						//UART_write_str("RQD\r\n");
						// 'many' (>1) more bytes
						if (_local.task_data_index + 1 < crt_task_ptr->len) {
							TWCR = _EN_TRANSFER | (1 << TWEA);	
						// just one more byte, pls!
						} else {
							TWCR = _EN_TRANSFER;
						}
					} else {
						// end transfer
						*(crt_task_ptr->status_ptr) = I2C_SUCCESS;
						_task_ended();
					}
					break;
					
				// Data byte received by master
				// NACK sent to slave
				/*
				case 0x58:
					// OBS: Should read byte?
					//crt_task_ptr->data[_local.task_data_index++] = TWDR;
					*(crt_task_ptr->status_ptr) = I2C_SUCCESS;
					_task_ended();
					break;
				*/
					
			}
		// Write task
		} else {
			switch (TWSR & 0xF8u) {
				
				// (REPEATED) START sent
				case 0x08:
				case 0x10:
					// add slave address and W bit
					_local.task_data_index = 0;
					TWDR = (crt_task_ptr->addr << 1);
					TWCR = _EN_TRANSFER;
					//UART_write_str("_STA_W\n\r");
					break;
					
				// slave send NACK on SLA+W
				case 0x20:
					*(crt_task_ptr->status_ptr) = I2C_ADDRESS_NACK;
					_task_ended();
					//UART_write_str("_NA_W\n\r");
					break;
					
				// slave sent ACK 
				// on (SLA+W or Data write)
				case 0x18:
				case 0x28:
					// if more data to send
					if (crt_task_ptr->len > _local.task_data_index) {
						TWDR = crt_task_ptr->data[_local.task_data_index++];
						TWCR = _EN_TRANSFER;
						//UART_write_str("TXD\n\r");
					} else {
						*(crt_task_ptr->status_ptr) = I2C_SUCCESS;
						_task_ended();
					}
					break;
				// slave sent NACK on data send
				case 0x30:
					*(crt_task_ptr->status_ptr) = I2C_WRITE_NACK;
					_task_ended();
					//UART_write_str("_ND_W\n\r");
					break;
			}
		}
	} else {
		UART_write_str("I2C_ERR:no task!\n");
	}
}

void I2C_init(void)
{
	// init task circular buffer
	_local.task_head = 0;
	_local.task_tail = 0;
	_local.tasks_num = 0;
	_local.task_data_index = 0;
	
	// init I2C peripheral
	// configure frequency
	TWBR = CONF_I2C_BIT_RATE_REG;
	TWSR = CONF_I2C_PRESCALER & 0x3;
}


uint8_t I2C_available_tasks(void)
{
	return CONF_I2C_TASK_BUFF_SIZE - _local.tasks_num;
}


bool _I2C_enqueue_task(bool read, uint8_t addr, uint8_t *buff_ptr, uint16_t max_len, volatile I2C_stat_t *status_ptr)
{
	uint8_t flags;
	
	// save interrupt flag state
	flags = SREG;
	cli();
	
	if (_local.tasks_num >= CONF_I2C_TASK_BUFF_SIZE) {
		SREG = flags;
		return false;
	}
	
	// enqueue task
	_local.tasks[_local.task_tail].data = buff_ptr;
	_local.tasks[_local.task_tail].addr = addr;
	_local.tasks[_local.task_tail].len = max_len;
	_local.tasks[_local.task_tail].read = read;
	_local.tasks[_local.task_tail].status_ptr = status_ptr;
	*status_ptr = I2C_IN_PROGRESS;
	_local.task_tail = (_local.task_tail + 1) 
						& (CONF_I2C_TASK_BUFF_SIZE - 1);
	_local.tasks_num ++;
	
	// if this is the first task added to the queue
	// (queue is empty)
	if (1 == _local.tasks_num) {
		// send start condition
		_local.task_data_index = 0;
		TWCR = _EN_TRANSFER | (1 << TWSTA);
	}
	
	// restore interrupt flag		
	SREG = flags;
	
	return true;
}

