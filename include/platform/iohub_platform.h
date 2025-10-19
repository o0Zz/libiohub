#ifndef _BOARD_H_
#define _BOARD_H_

typedef enum 
{
    PinMode_Output,
    PinMode_Input
}PinMode;

typedef enum 
{
    PinLevel_Low = 0,
    PinLevel_High
}PinLevel;

#if defined (ARDUINO)
	#include "board/arduino/board.h"
#elif defined (RASPBERRY)
	#include "board/rpi/board.h"
#elif defined (MPSSE)
	#include "board/mpsse/board.h"
#endif

#include "board/drv_i2c.h"
#include "board/drv_spi.h"
#include "board/drv_uart.h"

//#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG(...)				log_debug(__VA_ARGS__)
#	define LOG_BUFFER(aBuffer, aSize)	log_buffer(aBuffer, aSize)
#	define LOG_ERROR(...)				log_error(__VA_ARGS__)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_BUFFER(aBuffer, aSize)	do{}while(0)
#	define LOG_ERROR(...)				do{}while(0)
#endif

#ifndef MIN
# 	define MIN(x, y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
# 	define MAX(x, y) ((x)>(y)?(x):(y))
#endif

#endif