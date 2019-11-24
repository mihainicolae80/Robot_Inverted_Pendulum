/*
 * uart.h
 *
 * Created: 3/21/2019 5:28:54 PM
 *  Author: mihai
 */ 


#ifndef UART_H_
#define UART_H_

#include <stdint.h>


/* Initialize UART peripheral */
void UART_init(void);

/* Write byte of data to internal TX buffer */
void UART_write(uint8_t byte);

/* Write string to UART */
void UART_write_str(const char *str);

/* Read one byte of data from internal RX buffer.
	The byte must be available
 */
uint8_t UART_read(void);

/* Return number of bytes available for reading */
int	 UART_available(void);


#endif /* UART_H_ */