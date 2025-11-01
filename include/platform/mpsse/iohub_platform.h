#pragma once

#include "mpsse.h"

extern struct mpsse_context *gMPSSECtx;

#define IRAM_ATTR

static inline void	iohub_platform_init()
{
	//Nothing to do
}
	
#	define 	iohub_digital_set_pin_mode(aPin, aPinMode)\
		do{\
			if (gMPSSECtx == NULL || !gMPSSECtx->open)\
				gMPSSECtx = MPSSE(GPIO, 0, 0);\
			IOHUB_ASSERT (aPinMode != PinMode_Input || (aPin < 4)); /*Input is available on D4 to D7*/ \
			if (gMPSSECtx == NULL || !gMPSSECtx->open) \
				IOHUB_LOG_ERROR("Device FTDI not found"); \
		}while(0)

#	define iohub_digital_write(aPin, aPinLevel)							do { (aPinLevel == PinLevel_High) ? PinHigh(gMPSSECtx, aPin); PinLow(gMPSSECtx, aPin); } while(0)
#	define iohub_digital_read(aPin)										(PinState(gMPSSECtx, aPin, -1) ? PinLevel_High : PinLevel_Low)

#	define iohub_time_delay_ms(anMSDelay)								iohub_time_delay_us((anMSDelay) * 1000)

#ifdef _WIN32
    #include <windows.h>

    // microsecond delay
    static inline void iohub_time_delay_us(unsigned long us)
    {
        // Windows Sleep() has millisecond precision, so we use a busy wait for sub-ms
        if (us >= 1000)
            Sleep(us / 1000);
        else {
            LARGE_INTEGER freq, start, now;
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&start);
            double elapsed;
            do {
                QueryPerformanceCounter(&now);
                elapsed = (double)(now.QuadPart - start.QuadPart) * 1e6 / freq.QuadPart;
            } while (elapsed < us);
        }
    }

    // current monotonic time in microseconds
    static inline uint64_t iohub_time_now_us(void)
    {
        LARGE_INTEGER freq, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&now);
        return (uint64_t)((now.QuadPart * 1000000ULL) / freq.QuadPart);
    }

    // set process to real-time or normal priority
    static inline void iohub_set_real_time(int enable)
    {
        SetPriorityClass(GetCurrentProcess(), enable ? REALTIME_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
    }

#else //Linux / MacOS
    #include <unistd.h>
    #include <sys/time.h>
    #include <time.h>
    #include <sched.h>

    static inline void iohub_time_delay_us(unsigned long us)
    {
    #ifdef HAVE_USLEEP
        usleep(us);
    #else
        sleep((us + 999) / 1000000);
    #endif
    }

    static inline uint64_t iohub_time_now_us(void)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000ULL + (ts.tv_nsec / 1000ULL);
    }

    static inline void iohub_set_real_time(int enable)
    {
        struct sched_param param;
        param.sched_priority = enable ? 20 : 0;
        sched_setscheduler(0, enable ? SCHED_RR : SCHED_OTHER, &param);
    }

#endif