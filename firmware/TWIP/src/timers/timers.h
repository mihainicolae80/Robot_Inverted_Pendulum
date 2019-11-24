/*
 * timers.h
 *
 * Created: 4/6/2019 3:31:18 PM
 *  Author: mihai
 */ 


#ifndef TIMERS_H_
#define TIMERS_H_

typedef enum {
	TC0_CLK_OFF	= 0u,
	TC0_CLK_1	= 1u,
	TC0_CLK_8	= 2u,
	TC0_CLK_64	= 3u,
	TC0_CLK_256	= 4u,
	TC0_CLK_1024= 5u,
} TC0_clk_t;

typedef enum {
	TC2_CLK_OFF	= 0u,
	TC2_CLK_1	= 1u,
	TC2_CLK_8	= 2u,
	TC2_CLK_32	= 3u,
	TC2_CLK_64	= 4u,
	TC2_CLK_128	= 5u,
	TC2_CLK_256	= 6u,
	TC2_CLK_1024= 7u,
} TC2_clk_t;


#endif /* TIMERS_H_ */