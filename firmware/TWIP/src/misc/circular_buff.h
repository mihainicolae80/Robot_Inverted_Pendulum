/*
 * circular_buff.h
 *
 * Created: 3/21/2019 5:50:45 PM
 *  Author: mihai
 */ 


#ifndef CIRCULAR_BUFF_H_
#define CIRCULAR_BUFF_H_

#include <stdint.h>
#include "config/conf_circ_buff.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

struct circ_buff_t {
	uint8_t data[CIRC_BUFF_CAPACITY];
	uint8_t size;
	uint8_t head, tail;
};

/* Initialize circular buffer DS */
void circ_buff_init(struct circ_buff_t *buff);

/* Add byte to circular buffer */
void circ_buff_put(struct circ_buff_t *buff, uint8_t data);

/* Remove and return a byte from circular buffer */
uint8_t circ_buff_get(struct circ_buff_t *buff);

#endif /* CIRCULAR_BUFF_H_ */