/*
 * conf_board.h
 *
 * Created: 3/23/2019 8:06:20 AM
 *  Author: mihai
 */ 


#ifndef CONF_BOARD_H_
#define CONF_BOARD_H_

/* Select between 8MHz and 16MHz board clocks */
//#define BOARD_CLOCK_8M
#define BOARD_CLOCK_16M



// Set delay value
#ifdef F_CPU
#undef F_CPU
#endif

#ifdef BOARD_CLOCK_8M
#define F_CPU 8000000ul
#elif defined (BOARD_CLOCK_16M)
#define F_CPU 16000000ul
#endif

#endif /* CONF_BOARD_H_ */