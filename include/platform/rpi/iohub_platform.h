#pragma once                     

#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include "wiringPi.h"

#include <stdio.h>
#include <string.h>

#define INLINE												inline

INLINE void	iohub_platform_init()
{
	ozOS_SetEnv("WIRINGPI_DEBUG", "1");
	wiringPiSetup();
}

#define iohub_digital_set_pin_mode(aPin, aPinMode)			    pinMode(aPin, (aPinMode == PinMode_Output) ?  OUTPUT : INPUT)
#define iohub_digital_write(aPin, aPinLevel)					digitalWrite (aPin, (aPinLevel == PinLevel_High) ? HIGH : LOW)
#define iohub_digital_read(aPin)								(digitalRead(aPin) == HIGH ? PinLevel_High : PinLevel_Low)

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
