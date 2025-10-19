#include "drv_chacon_dio.h"

//#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG(...)				log_debug(__VA_ARGS__)
#	define LOG_ERROR(...)				log_error(__VA_ARGS__)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_ERROR(...)				do{}while(0)
#endif

#define DRV_CHACON_DIO_RECV_ACCURACY     0.2

	//Timing tuned for reception
#define DRV_CHACON_DIO_HEADER_1			10700
#define DRV_CHACON_DIO_HEADER_2			2700

#define DRV_CHACON_DIO_CLK				335
#define DRV_CHACON_DIO_BIT_0			240
#define DRV_CHACON_DIO_BIT_1			1300

#ifdef ARDUINO
	#define 	DRV_CHACON_WRITE(aLevel)	drv_digital_write_4(aLevel)
#else
	#define 	DRV_CHACON_WRITE(aLevel)	drv_digital_write(aCtx->mDigitalPinTx, aLevel)
#endif

/*
	//Original timings for send
#define DRV_CHACON_DIO_HEADER_1			9900
#define DRV_CHACON_DIO_HEADER_2			2675

#define DRV_CHACON_DIO_CLK				310
#define DRV_CHACON_DIO_BIT_0			275
#define DRV_CHACON_DIO_BIT_1			1340
*/

/*
    Begin emission:	1->335us puis 0->10700
    Begin frame:    1->335us puis 0->2700
    bit 0:          1->335us puis 0->230
    bit 1:          1->335us puis 0->1300
    End frame:      1->335us puis 0->2700
*/

/* ----------------------------------------------------------------- */

int drv_chacon_dio_init(chacon_dio *aCtx, u8 aGPIOTx)
{
    int theRet = SUCCESS;

    memset(aCtx, 0x00, sizeof(chacon_dio));

    aCtx->mDigitalPinTx = aGPIOTx;

#ifdef ARDUINO
    if (aCtx->mDigitalPinTx == PIN_INVALID)
        aCtx->mDigitalPinTx = 4; //Default pin on arduino !
#endif

    drv_digital_set_pin_mode(aCtx->mDigitalPinTx, PinMode_Output);
	
    return theRet;
}

/* ----------------------------------------------------------------- */

void drv_chacon_dio_uninit(chacon_dio *aCtx)
{
}

/* ----------------------------------------------------- */

void drv_chacon_dio_send_bit(chacon_dio *aCtx, u8 aBit) 
{
    DRV_CHACON_WRITE(PinLevel_High);
    time_delay_us(DRV_CHACON_DIO_CLK);
    DRV_CHACON_WRITE(PinLevel_Low);
    time_delay_us(aBit ? DRV_CHACON_DIO_BIT_1 : DRV_CHACON_DIO_BIT_0);
}

/* ----------------------------------------------------- */

void drv_chacon_dio_send_pair(chacon_dio *aCtx, u8 aBit) 
{
	//LOG_DEBUG("%d ", (int)aBit);
    drv_chacon_dio_send_bit(aCtx, aBit);
    drv_chacon_dio_send_bit(aCtx, !aBit);
}

/* ----------------------------------------------------- */

void drv_chacon_dio_send(chacon_dio *aCtx, BOOL afON, u32 aSenderID, u8 aReceiverID)
{
    int         i;

	for (int k=0; k<2; k++) //Send data 2 times
	{
			//Header
		DRV_CHACON_WRITE(PinLevel_High);
		time_delay_us(DRV_CHACON_DIO_CLK);
		DRV_CHACON_WRITE(PinLevel_Low);
		time_delay_us(DRV_CHACON_DIO_HEADER_1);   
		DRV_CHACON_WRITE(PinLevel_High);
		time_delay_us(DRV_CHACON_DIO_CLK);
		DRV_CHACON_WRITE(PinLevel_Low);
		time_delay_us(DRV_CHACON_DIO_HEADER_2);
		DRV_CHACON_WRITE(PinLevel_High);
	 
			//Send emitor ID
		for (i = 0; i < 26; i++)
			drv_chacon_dio_send_pair(aCtx, (aSenderID >> (25 - i)) & 0x01);

			// 26th bit -- grouped command
		drv_chacon_dio_send_pair(aCtx, 0);
	 
			// 27th bit -- On or off
		drv_chacon_dio_send_pair(aCtx, afON ? 1 : 0);
	 
			// 4 last bits -- interruptor ID
		for (i = 0; i < 4; i++) 
			drv_chacon_dio_send_pair(aCtx, (aReceiverID >> (3 - i)) & 0x01);
	 
			//Footer
		DRV_CHACON_WRITE(PinLevel_High);
		time_delay_us(DRV_CHACON_DIO_CLK);
		DRV_CHACON_WRITE(PinLevel_Low);
		time_delay_us(DRV_CHACON_DIO_CLK);
	}
}
	
/* ----------------------------------------------------- */

static u16 drv_chacon_dio_read_timing(chacon_dio *aCtx) 
{
	//LOG_DEBUG("%d, ", aCtx->mTimings[aCtx->mTimingReadIdx]);
	return aCtx->mTimings[aCtx->mTimingReadIdx++];
}

/* ----------------------------------------------------- */

BOOL drv_chacon_dio_read_bit(chacon_dio *aCtx, u8 *aBit) 
{
	BOOL 	theRet = FALSE;
    u32 	theTimeMs;

	theTimeMs = drv_chacon_dio_read_timing(aCtx);
	if (!IS_EXPECTED_TIME(theTimeMs, DRV_CHACON_DIO_CLK, DRV_CHACON_DIO_RECV_ACCURACY))
		return FALSE;

	theTimeMs = drv_chacon_dio_read_timing(aCtx);
    if (IS_EXPECTED_TIME(theTimeMs, DRV_CHACON_DIO_BIT_0, DRV_CHACON_DIO_RECV_ACCURACY))
    {
        *aBit = 0;
        theRet = TRUE;
    }
    if (IS_EXPECTED_TIME(theTimeMs, DRV_CHACON_DIO_BIT_1, DRV_CHACON_DIO_RECV_ACCURACY))
    {
        *aBit = 1;
        theRet = TRUE;
    }
	
	//LOG_DEBUG("%u", *aBit);
	
    return theRet;
}

/* ----------------------------------------------------- */

BOOL drv_chacon_dio_read_pair(chacon_dio *aCtx, u8 *aBit) 
{
    u8 theBit = 0, theBitReversed = 0;

    if (!drv_chacon_dio_read_bit(aCtx, &theBit))
        return FALSE;
	
    if (!drv_chacon_dio_read_bit(aCtx, &theBitReversed))
        return FALSE;
	
    if (theBit != !theBitReversed)
        return FALSE;

    *aBit = theBit;
	
	//LOG_DEBUG("%d ", (int)*aBit);
    return TRUE;
}

/* ----------------------------------------------------- */

BOOL drv_chacon_dio_read(chacon_dio *aCtx, u32 *aSenderID, u8 *aReceiverID, BOOL *afON) 
{
    u8          theBit = 0;

	*aSenderID = 0;
	*aReceiverID = 0;
	*afON = 0;
	
        //Header
	drv_chacon_dio_read_timing(aCtx); //DRV_CHACON_DIO_CLK
	drv_chacon_dio_read_timing(aCtx); //10500
	drv_chacon_dio_read_timing(aCtx); //DRV_CHACON_DIO_CLK
	drv_chacon_dio_read_timing(aCtx); //2800
   
        //Emitter
    for (u32 i = 0; i < 26; i++)
    {
        if (!drv_chacon_dio_read_pair(aCtx, &theBit))
            return FALSE;
		
		*aSenderID <<= 1;
        *aSenderID |= theBit;
    }

        // 26th bit -- grouped command
    if (!drv_chacon_dio_read_pair(aCtx, &theBit))
        return FALSE;

        // 27th bit -- On or off
    if (!drv_chacon_dio_read_pair(aCtx, &theBit))
         return FALSE;
 
	*afON = theBit;
	
        // 4 last bits -- interruptor ID
    for (u32 i = 0; i < 4; i++) 
    {
        if (!drv_chacon_dio_read_pair(aCtx, &theBit))
            return FALSE;
		
		*aReceiverID <<= 1;
        *aReceiverID |= theBit;
    }
 
		//Footer
	drv_chacon_dio_read_timing(aCtx); //DRV_CHACON_DIO_CLK

    return TRUE;
}

/* ----------------------------------------------------- */

BOOL drv_chacon_dio_detectPacket(digital_async_receiver_interface_ctx *aCtx, u32 aDurationUs)
{
	chacon_dio *theCtx = (chacon_dio *)aCtx;
	
	theCtx->mTimings[theCtx->mTimingCount++] = aDurationUs;
	
	switch(theCtx->mTimingCount)
	{
		case 1:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_CHACON_DIO_CLK, DRV_CHACON_DIO_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
		case 2:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_CHACON_DIO_HEADER_1, DRV_CHACON_DIO_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
		case 3:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_CHACON_DIO_CLK, DRV_CHACON_DIO_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
		case 4:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_CHACON_DIO_HEADER_2, DRV_CHACON_DIO_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
			
		default:
			if (theCtx->mTimingCount == sizeof(theCtx->mTimings)/sizeof(u16))
			{
				//LOG_DEBUG("chacon_dio: Signal detected !\r\n");
				theCtx->mTimingReadIdx = 0;
				return TRUE;
			}
			break;
	}
	
	return FALSE;
}

/* ----------------------------------------------------- */

void drv_chacon_dio_packetHandled(digital_async_receiver_interface_ctx *aCtx)
{
	chacon_dio *theCtx = (chacon_dio *)aCtx;
	
	theCtx->mTimingCount = 0;
}

/* ----------------------------------------------------- */

void drv_chacon_dio_dump_timings(chacon_dio *aCtx)
{
#ifdef DEBUG
	for (u32 i=0; i<aCtx->mTimingCount; i++)
		LOG_DEBUG("%d, ", aCtx->mTimings[i]);
		
	LOG_DEBUG("\r\n");
#endif
}

/* ----------------------------------------------------- */

const digital_async_receiver_interface  *drv_chacon_dio_get_interface(void)
{
	static const digital_async_receiver_interface sInterface = 
	{
		DRV_CHACON_DIO_ID,
		drv_chacon_dio_detectPacket,
		drv_chacon_dio_packetHandled
	};
	
	return &sInterface;
}