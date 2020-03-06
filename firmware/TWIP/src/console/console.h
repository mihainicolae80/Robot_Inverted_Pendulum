/*
 * console.h
 *
 * Created: 3/22/2019 4:59:35 PM
 *  Author: mihai
 */ 


#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdarg.h>
#include <stdbool.h>


extern volatile bool CONF_streaming_on;


/* Initialize the console */
void CONS_init(void);

/* Called in the main loop to handle UART commands */
void CONS_handle(void);

void print(const char *format, ...);


/////////////// TEST FUNCTION ////////////////
void _CONS_test_print(void);

#endif /* CONSOLE_H_ */