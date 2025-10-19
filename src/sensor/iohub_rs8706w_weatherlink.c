#include "drv_rs8706w_weatherlink.h"
#include <string.h>

//#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG(...)				log_debug(__VA_ARGS__)
#	define LOG_ERROR(...)				log_error(__VA_ARGS__)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_ERROR(...)				do{}while(0)
#endif

#define DRV_RS8706_WEATHERLINK_RECV_ACCURACY     	0.1
#define DRV_RS8706_WEATHERLINK_LOCK					8000
#define DRV_RS8706_WEATHERLINK_CLK					530
#define DRV_RS8706_WEATHERLINK_BIT_0				2000
#define DRV_RS8706_WEATHERLINK_BIT_1				3750

#define DRV_RS8706_WEATHERLINK_BUFFER_SIZE			5

/* ------------------------------------------------------------------ */

	static u8 drv_rs8706w_weatherlink_crc(rs8706w_weatherlink *aCtx, u32 aBuffer)
	{
		u8 theResult = 0;
		for (int i=0; i<8; i++)
			theResult += (aBuffer >> 28-(4*i)) & 0x0F;
		
		return theResult;
	}
	
/* ----------------------------------------------------- */

static u16 drv_rs8706w_weatherlink_read_timing(rs8706w_weatherlink *aCtx) 
{
	//LOG_DEBUG("%d, ", aCtx->mTimings[aCtx->mTimingReadIdx]);
	return aCtx->mTimings[aCtx->mTimingReadIdx++];
}

/* ------------------------------------------------------------------ */

int drv_rs8706w_weatherlink_init(rs8706w_weatherlink *aCtx)
{
    int theRet = SUCCESS;

    memset(aCtx, 0x00, sizeof(rs8706w_weatherlink));
    
    return theRet;
}

/* ------------------------------------------------------------------ */

void drv_rs8706w_weatherlink_uninit(rs8706w_weatherlink *aCtx)
{
}

/* ----------------------------------------------------- */

BOOL drv_rs8706w_weatherlink_read_bit(rs8706w_weatherlink *aCtx, u8 *aBit) 
{
	BOOL theRet = FALSE;
    u32 theTimeMs;

	theTimeMs = drv_rs8706w_weatherlink_read_timing(aCtx);
	if (!IS_EXPECTED_TIME(theTimeMs, DRV_RS8706_WEATHERLINK_CLK, DRV_RS8706_WEATHERLINK_RECV_ACCURACY))
		return FALSE;

	theTimeMs = drv_rs8706w_weatherlink_read_timing(aCtx);
    if (IS_EXPECTED_TIME(theTimeMs, DRV_RS8706_WEATHERLINK_BIT_0, DRV_RS8706_WEATHERLINK_RECV_ACCURACY))
    {
        *aBit = 0;
        theRet = TRUE;
    }
	
    if (IS_EXPECTED_TIME(theTimeMs, DRV_RS8706_WEATHERLINK_BIT_1, DRV_RS8706_WEATHERLINK_RECV_ACCURACY))
    {
        *aBit = 1;
        theRet = TRUE;
    }

	//LOG_DEBUG("%u", *aBit);
	
    return theRet;
}

/* ------------------------------------------------------------------ */

BOOL drv_rs8706w_weatherlink_read(rs8706w_weatherlink *aCtx, rs8706w_weatherlink_data *anOutputData)
{
	u8 theBit;
    u32 theBuffer = 0;
	u8 theCRC = 0;

	drv_rs8706w_weatherlink_read_timing(aCtx); //DRV_RS8706_WEATHERLINK_LOCK

    for (u8 i=0; i<32; i++)
    {
		if (!drv_rs8706w_weatherlink_read_bit(aCtx, &theBit))
			return FALSE;
		
		theBuffer <<= 1;
        theBuffer |= theBit;
    }
	
	for (u8 i=0; i<5; i++)
    {
		if (!drv_rs8706w_weatherlink_read_bit(aCtx, &theBit))
			return FALSE;
		
		theCRC <<= 1;
        theCRC |= theBit;
    }
	
    /*
	u8 theCRCComputed = drv_rs8706w_weatherlink_crc(aCtx, theBuffer);
    if ((theCRCComputed & 0x0F) != (theCRC & 0x0F))
    {
		LOG_DEBUG("INVALID CRC: Expected: %X, Got: %X\r\n", theCRCComputed & 0x0F, theCRC & 0x0F);
		return FALSE;
	}
	*/
	
	anOutputData->mStationID = (theBuffer >> 24) & 0xFF;
    anOutputData->mTemperatureCelsius = ((theBuffer >> 8) & 0xFFFF) / (float)10.0;
    anOutputData->mRainfallMillimeters = theBuffer & 0xFF;

    return TRUE;
}


/* ----------------------------------------------------- */

void drv_rs8706w_weatherlink_dump_timings(rs8706w_weatherlink *aCtx)
{
#ifdef DEBUG
	for (u32 i=0; i<aCtx->mTimingCount; i++)
		LOG_DEBUG("%d, ", aCtx->mTimings[i]);
		
	LOG_DEBUG("\r\n");
#endif
}

/* ----------------------------------------------------- */

BOOL drv_rs8706w_weatherlink_detectPacket(digital_async_receiver_interface_ctx *aCtx, u32 aDurationUs)
{
	rs8706w_weatherlink *theCtx = (rs8706w_weatherlink *)aCtx;
	
	theCtx->mTimings[theCtx->mTimingCount++] = aDurationUs;
	
	switch(theCtx->mTimingCount)
	{
		case 1:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_RS8706_WEATHERLINK_LOCK, DRV_RS8706_WEATHERLINK_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
		
		default:
			if (aDurationUs > 350 && aDurationUs < 4500)
			{
				if (theCtx->mTimingCount == sizeof(theCtx->mTimings)/sizeof(u16))
				{
					//LOG_DEBUG("rs8706w_weatherlink: Signal detected !\r\n");
					theCtx->mTimingReadIdx = 0;
					return TRUE;
				}
			}
			else
			{
				theCtx->mTimingCount = 0;
			}
			break;
	}
	
	return FALSE;
}

/* ----------------------------------------------------- */

void drv_rs8706w_weatherlink_packetHandled(digital_async_receiver_interface_ctx *aCtx)
{
	rs8706w_weatherlink *theCtx = (rs8706w_weatherlink *)aCtx;
	
	theCtx->mTimingCount = 0;
}

/* ----------------------------------------------------- */

const digital_async_receiver_interface  *drv_rs8706w_weatherlink_get_interface(void)
{
	static const digital_async_receiver_interface sInterface = 
	{
		DRV_RS8706W_ID,
		drv_rs8706w_weatherlink_detectPacket,
		drv_rs8706w_weatherlink_packetHandled
	};
	
	return &sInterface;
}
