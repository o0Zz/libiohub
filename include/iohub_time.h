#ifndef _TIME_H_
#define _TIME_H_

#define TIMER_START()	 			(time_now_ms())
#define TIMER_ELAPSED(aTimeStart)	((u32)time_now_ms() - (u32)aTimeStart)

#define IS_EXPECTED_TIME(aTimeUs, anExpectedTimeUs, anAccuracyPourcentage) \
	((aTimeUs > (anExpectedTimeUs * (1.0 - anAccuracyPourcentage))) && (aTimeUs < (anExpectedTimeUs * (1.0 + anAccuracyPourcentage))))

/* -------------------------------------------------------------- */

INLINE u32 time_remaining(u32 aStartTimeMs, u32 aTimeoutMs)
{
    u32 theTimeElapsed = ((time_now_us()/1000) - aStartTimeMs);

    if (theTimeElapsed >= aTimeoutMs)
        return 0;

    return aTimeoutMs - theTimeElapsed;
}

#endif
