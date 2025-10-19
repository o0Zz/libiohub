#include "mpsse.h"

extern struct mpsse_context *gMPSSECtx;

#	define INLINE												inline

INLINE void	board_init()
{
	//struct mpsse_context *gMPSSECtx;
}
	
#	define 	drv_digital_set_pin_mode(aPin, aPinMode)\
		do{\
			if (gMPSSECtx == NULL || !gMPSSECtx->open)\
				gMPSSECtx = MPSSE(GPIO, 0, 0);\
			ASSERT (aPinMode != PinMode_Input || (aPin < 4)); /*Input is available on D4 to D7*/ \
			if (gMPSSECtx == NULL || !gMPSSECtx->open) \
				log_err("Device FTDI not found"); \
		}while(0)

#	define drv_digital_write(aPin, aPinLevel)					do { (aPinLevel == PinLevel_High) ? PinHigh(gMPSSECtx, aPin); PinLow(gMPSSECtx, aPin); } while(0)
#	define drv_digital_read(aPin)								(PinState(gMPSSECtx, aPin, -1) ? PinLevel_High : PinLevel_Low)
                                                  
#	define log_debug(aLog, ...)									ozLog_Debug(aLog);
#	define log_buffer(aBuffer, aLen)							do{ for (int i=0; i<aLen; i++) log_debug("0x%02X ", ((u8)aBuffer[i])); log_debug("\n"); }while(0)
#	define log_assert(aFunction, aLine, anExpr)					log_debug("ASSERT: %d", aLine)

#	define time_delay_ms(anMSDelay)									ozThread_Sleep(anMSDelay)

#	ifdef HAVE_USLEEP
#		define time_delay_us(anUSDelay)								usleep(anUSDelay);
#	else
#		define time_delay_us(anUSDelay)								ozThread_Sleep((anUSDelay+999)/1000)
#	endif

#	define time_now_us()											ozTime_MonotonicTimeMicro()
#	define set_real_time(afON)										ozProcess_SetPriority(afON ? kozProccessPriority_RealTime : kozProccessPriority_Normal)
