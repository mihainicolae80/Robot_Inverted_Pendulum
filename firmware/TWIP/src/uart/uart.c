/*
 * uart.c
 *
 * Created: 3/21/2019 5:28:43 PM
 *  Author: mihai
 */ 

#include "uart/uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "misc/circular_buff.h"
#include "config/conf_board.h"
#include <util/delay.h>
#include <avr/common.h>


static struct {
	volatile struct circ_buff_t buff_in, buff_out;
} _local;



// Called @data RX
ISR(USART_RX_vect) 
{
	circ_buff_put((struct circ_buff_t*)&(_local.buff_in), UDR0);
}

// Called when data can be written to
// UART data reg
// (TX) 
ISR(USART_UDRE_vect)
{
	// if there are data to be sent
	if (_local.buff_out.size > 0) {
		UDR0 = circ_buff_get((struct circ_buff_t*)&(_local.buff_out));
	} else {
		// no data to be sent
		// disable TX interrupt	
		UCSR0B &= ~(1 << UDRIE0);
	}
}

void UART_init(void)
{
	// 0b00000110
	// Async, 8N1
	UCSR0C = 0x06;
	// set baudrate 38400
	UBRR0H = 0;
#if defined (BOARD_CLOCK_8M)
	UBRR0L = 12;
#else
	UBRR0L = 25;
#endif
	// enable RX interrupt
	UCSR0B |= 1 << RXCIE0;
	// enable RX, TX
	UCSR0B |= (3 << 3);
	
	circ_buff_init((struct circ_buff_t*)&(_local.buff_in));
	circ_buff_init((struct circ_buff_t*)&(_local.buff_out));
}



void UART_write(uint8_t byte)
{	
	uint8_t flags;
	
	flags = SREG;
	// check if buff out is full
	cli();
	while (_local.buff_out.size >= CIRC_BUFF_CAPACITY) {
		// buffer full
		// re-enable interrupts
		sei();
		// wait a bit (enough to TX 1 byte)
		_delay_us(27);
		cli();
	}


	// add data to TX buffer
	circ_buff_put((struct circ_buff_t*)&(_local.buff_out), byte);
	
	// enable TX Data Reg Empty Interrupt
	UCSR0B |= 1 << UDRIE0;
	
	SREG = flags;
}

void UART_write_str(const char *str)
{
	uint16_t i;
	
	i = 0;
	while(str[i] != '\0') {
		UART_write(str[i++]);
	}
}


uint8_t UART_read(void)
{
	uint8_t tmp;
	
	cli();
	tmp = circ_buff_get((struct circ_buff_t*)&(_local.buff_in));
	sei();
	
	return tmp;
}


int	 UART_available(void)
{
	return (int)_local.buff_in.size;
}
