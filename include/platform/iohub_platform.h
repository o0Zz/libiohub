#pragma once

typedef enum 
{
    PinMode_Output,
    PinMode_Input
}IOHubPinMode;

typedef enum 
{
    PinLevel_Low = 0,
    PinLevel_High
}IOHubPinLevel;

#if defined(IOHUB_PLATFORM_ARDUINO)
	#include "platform/arduino/iohub_platform.h"
#elif defined(IOHUB_PLATFORM_RPI)
	#include "platform/rpi/iohub_platform.h"
#elif defined(IOHUB_PLATFORM_MPSSE)
	#include "platform/mpsse/iohub_platform.h"
#endif

#include "platform/iohub_i2c.h"
#include "platform/iohub_spi.h"
#include "platform/iohub_uart.h"