/*
 * circular_buff.c
 *
 * Created: 3/21/2019 5:50:53 PM
 *  Author: mihai
 */ 

#include "misc/circular_buff.h"

void circ_buff_init(struct circ_buff_t *buff)
{
	// sanity check
	if (NULL == buff) {
		return;
	}
	
	buff->head = 0;
	buff->tail = 0;
	buff->size = 0;
}

void circ_buff_put(struct circ_buff_t *buff, uint8_t data)
{
	// sanity check
	if (NULL == buff) {
		return;
	}
	
	// discard excess bytes
	if (buff->size >= CIRC_BUFF_CAPACITY) {
		return;
	}
	
	buff->data[buff->tail] = data;
	buff->tail = (buff->tail + 1) & (CIRC_BUFF_CAPACITY - 1);
	buff->size ++;
}

uint8_t circ_buff_get(struct circ_buff_t *buff)
{
	uint8_t tmp;
	
	// sanity check
	if (NULL == buff) {
		return 0xff;
	}
	
	if (buff->size == 0) {
		return 0;
	}
	
	tmp = buff->data[buff->head];
	buff->head = (buff->head + 1) & (CIRC_BUFF_CAPACITY - 1);
	buff->size --;
	
	return tmp;
}
