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


#define IOHUB_GPIO_PIN_INVALID                	0xFF

#define IOHUB_GPIO_IS_BIT_SET(aVar, aBit)							((aVar) & (0x1 << (aBit)))
#define IOHUB_GPIO_BIT_SET(aVar, aBit)								do { (aVar) |= (0x1 << aBit); }while(0)
#define IOHUB_GPIO_BIT_CLEAR(aVar, aBit)							do { (aVar) &= ~(0x1 << aBit); }while(0)

#if defined(IOHUB_PLATFORM_ARDUINO)
	#include "platform/arduino/iohub_platform.h"
#elif defined(IOHUB_PLATFORM_RPI)
	#include "platform/rpi/iohub_platform.h"
#elif defined(IOHUB_PLATFORM_MPSSE)
	#include "platform/mpsse/iohub_platform.h"
#elif defined(IOHUB_PLATFORM_ESP32)
	#include "platform/esp32/iohub_platform.h"
#endif