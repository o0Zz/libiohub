#pragma once

#include "iohub_types.h"
#include "platform/iohub_platform.h"

#define TIMER_START()	 			(iohub_time_now_ms())
#define TIMER_ELAPSED(aTimeStart)	((u32)iohub_time_now_ms() - (u32)aTimeStart)

#define IS_EXPECTED_TIME(aTimeUs, anExpectedTimeUs, anAccuracyPourcentage) \
	((aTimeUs > (anExpectedTimeUs * (1.0 - anAccuracyPourcentage))) && (aTimeUs < (anExpectedTimeUs * (1.0 + anAccuracyPourcentage))))

/* -------------------------------------------------------------- */

static inline u32 iohub_time_remaining(u32 aStartTimeMs, u32 aTimeoutMs)
{
    u32 theTimeElapsed = ((iohub_time_now_us()/1000) - aStartTimeMs);

    if (theTimeElapsed >= aTimeoutMs)
        return 0;

    return aTimeoutMs - theTimeElapsed;
}

