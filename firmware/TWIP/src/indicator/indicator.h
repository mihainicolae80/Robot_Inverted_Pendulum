/*
 * indicator.h
 *
 * Created: 4/6/2019 2:55:05 PM
 *  Author: mihai
 */ 


#ifndef INDICATOR_H_
#define INDICATOR_H_

typedef enum {
	IND_BOOTING,
	IND_RUNNING,
	IND_ERROR,
	IND_SIGNAL
} indicator_mode_t;

void IND_set_mode(indicator_mode_t mode);

void IND_iterate(void);



#endif /* INDICATOR_H_ */