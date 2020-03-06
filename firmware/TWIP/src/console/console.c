/*
 * console.c
 *
 * Created: 3/22/2019 4:59:44 PM
 *  Author: mihai
 */ 


#include "console/console.h"
#include "config/conf_console.h"

#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <util/atomic.h>

#include "uart/uart.h"
#include "control_system/control_system.h"
#include "BNO055/BNO055.h"
#include "EEPROM/eeprom.h"


static void _print_sint(int num);
static void _print_hex(int num);
static void _print_float(float num);
static bool _strcmp(const char * str1, const char * str2, int n);
void _handle_cmd(void);
static void _cmd_calib(void);


bool volatile CONF_streaming_on = true;


static struct {
	uint8_t data[CONS_CMD_LINE_SIZE];
	uint8_t index;
} _cmd;


void CONS_init(void)
{
	_cmd.index = 0;
}


bool _strcmp(const char * str1, const char * str2, int n)
{
	int i;
	for (i = 0; i != (n-1) ; i++) {
		if ((str1[i] == '\0') || (str2[i] == '\0')) {
			return str1[i] == str2[i];
		}
		
		if (str1[i] != str2[i]) {
			return false;
		}
	}
	
	return true;
}


void CONS_handle(void)
{
	while (UART_available() > 0) {
		// append byte
		_cmd.data[_cmd.index] = UART_read();
		// check for complete command
		if ('\n' == _cmd.data[_cmd.index]) {
			// replace newline with line terminator
			_cmd.data[_cmd.index] = '\0';
			// handle buffer content as command
			goto _cons_iterate_command;
		}
		else {
			if (_cmd.index >= CONS_CMD_LINE_SIZE) {
				_cmd.index = 0;
			}
			else {
				_cmd.index ++;
			}
		}
	}
	
	return;
	
_cons_iterate_command:
	_handle_cmd();
	_cmd.index = 0;
}

void _handle_cmd(void)
{
	int32_t tmp;
	
	
	/* P, I, D parameters are integers representing 
	 * the parameter value times 1000 
	 */
	// PID P term change
	if (('p' == _cmd.data[0]) 
	&&  ('=' == _cmd.data[1])) {
		if (_cmd.index >= 6) {
			tmp = ((uint32_t)_cmd.data[2])
			|	((uint32_t)_cmd.data[3] << 8)
			|	((uint32_t)_cmd.data[4] << 16)
			|	((uint32_t)_cmd.data[5] << 24);
			CTRL_set_PID_bp((float)tmp / 1000.0);
			UART_write_str("ok");
		}
	// PID I term change
	} else if ( ('i' == _cmd.data[0]) 
			&&  ('=' == _cmd.data[1])) {
		if (_cmd.index >= 6) {
			tmp = ((uint32_t)_cmd.data[2])
			|	((uint32_t)_cmd.data[3] << 8)
			|	((uint32_t)_cmd.data[4] << 16)
			|	((uint32_t)_cmd.data[5] << 24);
			CTRL_set_PID_bi((float)tmp / 1000.0);
			UART_write_str("ok");
		}
	// PID D term change
	} else if ( ('d' == _cmd.data[0]) 
			&&  ('=' == _cmd.data[1])) {
		if (_cmd.index >= 6) {
			tmp = ((uint32_t)_cmd.data[2])
			|	((uint32_t)_cmd.data[3] << 8)
			|	((uint32_t)_cmd.data[4] << 16)
			|	((uint32_t)_cmd.data[5] << 24);
			CTRL_set_PID_bd((float)tmp / 1000.0);
			UART_write_str("ok");
		}
	} else if (('a' == _cmd.data[0])
		   &&  ('=' == _cmd.data[1])) {
		if (_cmd.index >= 6) {
			tmp = ((uint32_t)_cmd.data[2])
			|	((uint32_t)_cmd.data[3] << 8)
			|	((uint32_t)_cmd.data[4] << 16)
			|	((uint32_t)_cmd.data[5] << 24);
			CTRL_set_angle_off((float)tmp / 1000.0f);
			UART_write_str("ok");
		}
	} else if ( ('t' == _cmd.data[0])
			&&  ('=' == _cmd.data[1])) {
		if ((_cmd.index > 2) 
		&& ('1' == _cmd.data[2])) {
			_BNO_test_1();
		} else if ((_cmd.index > 2)
		&& ('2' == _cmd.data[2])) {
			_BNO_test_2();
		} else if ((_cmd.index > 2)
		&& ('3' == _cmd.data[2])) {
			_BNO_test_3();
		} else if ((_cmd.index > 2)
		&& ('4' == _cmd.data[2])) {
			_BNO_test_4();
		}
	} else if ('c' == _cmd.data[0]){
		BNO_read_print_cal();
	} else if (_strcmp((char*)_cmd.data, "stream_on", -1)) {
		CONF_streaming_on = true;
	} else if (_strcmp((char*)_cmd.data, "stream_off", -1)) {
		CONF_streaming_on = false;
		UART_write_str("streaming off!\r\n");
	} else if (_strcmp((char*)_cmd.data, "echo", 4)) {
		UART_write_str("echo");
	} else if (_strcmp("calib", (char*)_cmd.data, 5)) {
		_cmd_calib();
		UART_write_str("Calib saved!\r\n");
	} else {
		UART_write_str("Invalid cmd!");
	}
}

void _cmd_calib(void)
{
	struct calibration_t calib;
	
	calib = CTRL_get_calib();
	
	EEPROM_write_array(0, (uint8_t*)&calib, sizeof(calib));
}


static void _print_sint(int num)
{
	int len;
	char buff[10];
	
	
	if (0 == num) {
		UART_write('0');
	} else {
		// if negative
		if (num < 0) {
			UART_write('-');
			num = -num;
		}
		
		// convert int to reverse char representation
		len = 0;
		while (num) {
			buff[len++] = '0' + (num % 10);
			num /= 10;
		}
		
		// print number
		for (len = len -1; len >= 0; len--) {
			UART_write(buff[len]);
		}
	}
}


static void _print_hex(int num)
{
	int len;
	char buff[10];
	
	if (0 == num) {
		UART_write('0');
	} else {
		// convert int to reverse char representation
		len = 0;
		while (num) {
			if ((num & 0xf) < 10) {
				buff[len++] = '0' + (num & 0xf);
			} else {
				buff[len++] = 'A' + (num & 0xf) - 10;
			}
			num = (unsigned int)num >> 4;
		}
		
		// print number
		for (len = len -1; len >= 0; len--) {
			UART_write(buff[len]);
		}
	}
}

static void _print_float(float num)
{
	int num_sign;
	
	num_sign = (num < 0) ? -1 : 1;
	num = fabs(num);
	
	// print integer part
	_print_sint((int)floor(num) * num_sign);
	UART_write('.');
	// print decimal part
	_print_sint((int)(100 * (num - floor(num))));
}

void print(const char *format, ...)
{
	va_list args;
	int index;
	
	va_start(args, format);
	
	// iterate format
	index = 0;
	while (format[index] != '\0') {
		if ('%' == format[index]) {
			switch (format[index + 1]) {
				case 'd':
				case 'i':
					_print_sint(va_arg(args, int));
					break;
				case 'x':
					_print_hex(va_arg(args, int));
					break;
				case 'f':
					_print_float((float)va_arg(args, double));
					break;
			}
			index ++;
		} else {
			// write to UART
			UART_write(format[index]);
		}
		index ++;
	}
	
	va_end(args); 
}




void _CONS_test_print(void)
{
	print("Test 1: Ana are mere!\r\n");
	print("Test 2: Ana are %d mere!\r\n", 123);
	print("Test 3: Ana are %d mere SI %d pere!\r\n", 123, -1);
	print("Test 4: Ana are %x mere\r\n", 0xfff3);
	print("Test 5: Ana are %f mere\r\n", 1234.456);
	print("Test 6: Ana are %f mere\r\n", -1234.456);
}