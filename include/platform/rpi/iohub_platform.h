#include "wiringPi.h"

#include "ozTools/ozLog.h"
#include "ozTools/ozTime.h"
#include "ozTools/ozProcess.h"
#include "ozTools/ozThreadC.h"
#include "ozTools/ozOS.h"

#include "wiringPi.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_USLEEP
#   include <unistd.h>
#endif

#	define INLINE												inline

	INLINE void	board_init()
	{
		ozOS_SetEnv("WIRINGPI_DEBUG", "1");
		wiringPiSetup();
	}

#	define drv_digital_set_pin_mode(aPin, aPinMode)			    pinMode(aPin, (aPinMode == PinMode_Output) ?  OUTPUT : INPUT)
#	define drv_digital_write(aPin, aPinLevel)					digitalWrite (aPin, (aPinLevel == PinLevel_High) ? HIGH : LOW)
#	define drv_digital_read(aPin)								(digitalRead(aPin) == HIGH ? PinLevel_High : PinLevel_Low)
                                                  
#	define log_debug(aLog, ...)									ozLog_Debug(aLog);
#	define log_buffer(aBuffer, aLen)							do{ for (int i=0; i<aLen; i++) log_debug("0x%02X ", ((u8)aBuffer[i])); log_debug("\n"); }while(0)
#	define log_assert(aFunction, aLine, anExpr)					do{ log_error("ASSERT: %s:%d\r\n", aFunction, aLine); while(1); }while(0)

#	define time_delay_ms(anMSDelay)								ozThread_Sleep(anMSDelay)

#	ifdef HAVE_USLEEP
#		define time_delay_us(anUSDelay)							usleep(anUSDelay);
#	else
#		define time_delay_us(anUSDelay)							ozThread_Sleep((anUSDelay+999)/1000)
#	endif

#	define time_now_us()										ozTime_MonotonicTimeMicro()
#	define set_real_time(afON)									ozProcess_SetPriority(afON ? kozProccessPriority_RealTime : kozProccessPriority_Normal)
