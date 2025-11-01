#include "sensor/iohub_digoo_r8h.h"

//#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_ERROR(...)				IOHUB_LOG_ERROR(__VA_ARGS__)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_ERROR(...)				do{}while(0)
#endif

#define DRV_DIGOO_R8H_RECV_ACCURACY     0.1
#define DRV_DIGOO_R8H_CLK				550
#define DRV_DIGOO_R8H_BIT_0				890
#define DRV_DIGOO_R8H_BIT_1				1880

/* ----------------------------------------------------------------- */

int iohub_digoo_r8h_init(digoo_r8h *aCtx)
{
    int theRet = SUCCESS;

    memset(aCtx, 0x00, sizeof(digoo_r8h));

    return theRet;
}

/* ----------------------------------------------------------------- */

void iohub_digoo_r8h_uninit(digoo_r8h *aCtx)
{
}

/* ----------------------------------------------------- */

static u16 iohub_digoo_r8h_read_timing(digoo_r8h *aCtx) 
{
	//LOG_DEBUG("%d, ", aCtx->mTimings[aCtx->mTimingReadIdx]);
	return aCtx->mTimings[aCtx->mTimingReadIdx++];
}

/* ----------------------------------------------------- */

BOOL iohub_digoo_r8h_read_bit(digoo_r8h *aCtx, u8 *aBit) 
{
	BOOL theRet = FALSE;
    u32 theTimeMs;

	theTimeMs = iohub_digoo_r8h_read_timing(aCtx);
	if (!IS_EXPECTED_TIME(theTimeMs, DRV_DIGOO_R8H_CLK, DRV_DIGOO_R8H_RECV_ACCURACY))
		return FALSE;

	theTimeMs = iohub_digoo_r8h_read_timing(aCtx);
    if (IS_EXPECTED_TIME(theTimeMs, DRV_DIGOO_R8H_BIT_0, DRV_DIGOO_R8H_RECV_ACCURACY))
    {
        *aBit = 0;
        theRet = TRUE;
    }
    if (IS_EXPECTED_TIME(theTimeMs, DRV_DIGOO_R8H_BIT_1, DRV_DIGOO_R8H_RECV_ACCURACY))
    {
        *aBit = 1;
        theRet = TRUE;
    }

	//LOG_DEBUG("%u", *aBit);
	
    return theRet;
}

/* ----------------------------------------------------- */

BOOL iohub_digoo_r8h_read(digoo_r8h *aCtx, u8 *aSensorID, u8 *aChannelID, float *aTemperature, u8 *anHumidity)
{
	u8 			i;
    u8          theBit = 0;
	u16			theTemperature = 0;
	
	*aSensorID 		= 0;
	*aChannelID 	= 0;
	*aTemperature 	= 0;
	*anHumidity 	= 0;
	
	for(i = 0; i < 8; i++)
	{
		if (!iohub_digoo_r8h_read_bit(aCtx, &theBit))
			return FALSE;
	
		*aSensorID <<= 1;
		*aSensorID |= theBit;
	}

	if (!iohub_digoo_r8h_read_bit(aCtx, &theBit)) //Battery low
		return FALSE;
	
	if (!iohub_digoo_r8h_read_bit(aCtx, &theBit)) //??
		return FALSE;

	for(i = 0; i < 2; i++)
	{
		if (!iohub_digoo_r8h_read_bit(aCtx, &theBit))
			return FALSE;
		
		*aChannelID <<= 1;
		*aChannelID |= theBit;
	}

	for(i = 0; i < 12; i++)
	{
		if (!iohub_digoo_r8h_read_bit(aCtx, &theBit))
			return FALSE;
		
		theTemperature <<= 1;
		theTemperature |= theBit;
	}
	*aTemperature = (float)theTemperature / 10;
		
		//??
	for(i = 0; i < 4; i++)
	{
		if (!iohub_digoo_r8h_read_bit(aCtx, &theBit))
			return FALSE;
	}
	
    for(i = 0; i < 8; i++)
	{
		if (!iohub_digoo_r8h_read_bit(aCtx, &theBit))
			return FALSE;
	
		*anHumidity <<= 1;
		*anHumidity |= theBit;
	}
	
    return TRUE;
}

/* ----------------------------------------------------- */

void iohub_digoo_r8h_dump_timings(digoo_r8h *aCtx)
{
#ifdef DEBUG
	for (u32 i=0; i<aCtx->mTimingCount; i++)
		LOG_DEBUG("%d, ", aCtx->mTimings[i]);
		
	LOG_DEBUG("");
#endif
}

/* ----------------------------------------------------- */

BOOL iohub_digoo_r8h_detectPacket(digital_async_receiver_interface_ctx *aCtx, u16 aDurationUs)
{
	digoo_r8h *theCtx = (digoo_r8h *)aCtx;
	
	theCtx->mTimings[theCtx->mTimingCount++] = aDurationUs;
	
	if (aDurationUs > 450 && aDurationUs < 2000)
	{
		if (theCtx->mTimingCount == sizeof(theCtx->mTimings)/sizeof(u16))
		{
			//LOG_DEBUG("digoo_r8h: Signal detected !");
			theCtx->mTimingReadIdx = 0;
			return TRUE;
		}
	}
	else
	{
		theCtx->mTimingCount = 0;
	}
	
	return FALSE;
}

/* ----------------------------------------------------- */

void iohub_digoo_r8h_packetHandled(digital_async_receiver_interface_ctx *aCtx)
{
	digoo_r8h *theCtx = (digoo_r8h *)aCtx;
	
	theCtx->mTimingCount = 0;
}

/* ----------------------------------------------------- */

const digital_async_receiver_interface  *iohub_digoo_r8h_get_interface(void)
{
	static const digital_async_receiver_interface sInterface = 
	{
		DRV_DIGOO_R8H_ID,
		iohub_digoo_r8h_detectPacket,
		iohub_digoo_r8h_packetHandled
	};
	
	return &sInterface;
}
