/*
 * conf_control_system.h
 *
 * Created: 4/6/2019 3:32:31 PM
 *  Author: mihai
 */ 


#ifndef CONF_CONTROL_SYSTEM_H_
#define CONF_CONTROL_SYSTEM_H_

#include "timers/timers.h"


// Enable max angle safety
#define SAFETY_ANGLE


/* Control system timer clock source 
 *(prescaler settings) 
 */
#define CONF_CTRL_CLK (TC2_CLK_1024)

/* Timer counter TOP value.
 * Control system frequency:
 * CTRL_CLK = F_CLK / CONF_CTRL_CLK / CONF_CTRL_TIMER_CNT
 * Current: CTRL_CLK = 16000000 / 1024 / 15 = ~1000Hz (1041Hz)
 */
#define CONF_CTRL_TIMER_CNT (15u)

/* Maximum angle value (degrees), beyond which 
 * the motor protection is activated 
 */
#define CONF_CTRL_MAX_ANGLE (35u)

/* Maximum sum of angles*/
#define CONF_CTRL_PID_MAX_SUM (100.0f)

#define CONF_CTRL_PID_MIN_CMD (90.0f)

/* Minimum time between Control Loop iterations */
#define CONF_CTRL_PID_LOOP_INTERVAL		(10u)

/* Minimum time between Calibration check iterations */
#define CONF_CTRL_CALIB_LOOP_INTERVAL	(100u)


#endif /* CONF_CONTROL_SYSTEM_H_ */