#include "power/iohub_chacon_dio.h"
#include "platform/iohub_platform.h"

//#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_ERROR(...)				IOHUB_LOG_ERROR(__VA_ARGS__)
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
	#define 	DRV_CHACON_WRITE(aLevel)	iohub_digital_write_4(aLevel)
#else
	#define 	DRV_CHACON_WRITE(aLevel)	iohub_digital_write(aCtx->mDigitalPinTx, aLevel)
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

int iohub_chacon_dio_init(chacon_dio *aCtx, u8 aGPIOTx)
{
    int theRet = SUCCESS;

    memset(aCtx, 0x00, sizeof(chacon_dio));

    aCtx->mDigitalPinTx = aGPIOTx;

#ifdef ARDUINO
    if (aCtx->mDigitalPinTx == IOHUB_PIN_INVALID)
        aCtx->mDigitalPinTx = 4; //Default pin on arduino !
#endif

    iohub_digital_set_pin_mode(aCtx->mDigitalPinTx, PinMode_Output);
	
    return theRet;
}

/* ----------------------------------------------------------------- */

void iohub_chacon_dio_uninit(chacon_dio *aCtx)
{
}

/* ----------------------------------------------------- */

void iohub_chacon_dio_send_bit(chacon_dio *aCtx, u8 aBit) 
{
    DRV_CHACON_WRITE(PinLevel_High);
    iohub_time_delay_us(DRV_CHACON_DIO_CLK);
    DRV_CHACON_WRITE(PinLevel_Low);
    iohub_time_delay_us(aBit ? DRV_CHACON_DIO_BIT_1 : DRV_CHACON_DIO_BIT_0);
}

/* ----------------------------------------------------- */

void iohub_chacon_dio_send_pair(chacon_dio *aCtx, u8 aBit) 
{
	//LOG_DEBUG("%d ", (int)aBit);
    iohub_chacon_dio_send_bit(aCtx, aBit);
    iohub_chacon_dio_send_bit(aCtx, !aBit);
}

/* ----------------------------------------------------- */

void iohub_chacon_dio_send(chacon_dio *aCtx, BOOL afON, u32 aSenderID, u8 aReceiverID)
{
    int         i;

	for (int k=0; k<2; k++) //Send data 2 times
	{
			//Header
		DRV_CHACON_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_CHACON_DIO_CLK);
		DRV_CHACON_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_CHACON_DIO_HEADER_1);   
		DRV_CHACON_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_CHACON_DIO_CLK);
		DRV_CHACON_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_CHACON_DIO_HEADER_2);
		DRV_CHACON_WRITE(PinLevel_High);
	 
			//Send emitor ID
		for (i = 0; i < 26; i++)
			iohub_chacon_dio_send_pair(aCtx, (aSenderID >> (25 - i)) & 0x01);

			// 26th bit -- grouped command
		iohub_chacon_dio_send_pair(aCtx, 0);
	 
			// 27th bit -- On or off
		iohub_chacon_dio_send_pair(aCtx, afON ? 1 : 0);
	 
			// 4 last bits -- interruptor ID
		for (i = 0; i < 4; i++) 
			iohub_chacon_dio_send_pair(aCtx, (aReceiverID >> (3 - i)) & 0x01);
	 
			//Footer
		DRV_CHACON_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_CHACON_DIO_CLK);
		DRV_CHACON_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_CHACON_DIO_CLK);
	}
}
	
/* ----------------------------------------------------- */

static u16 iohub_chacon_dio_read_timing(chacon_dio *aCtx) 
{
	//LOG_DEBUG("%d, ", aCtx->mTimings[aCtx->mTimingReadIdx]);
	return aCtx->mTimings[aCtx->mTimingReadIdx++];
}

/* ----------------------------------------------------- */

BOOL iohub_chacon_dio_read_bit(chacon_dio *aCtx, u8 *aBit) 
{
	BOOL 	theRet = FALSE;
    u32 	theTimeMs;

	theTimeMs = iohub_chacon_dio_read_timing(aCtx);
	if (!IS_EXPECTED_TIME(theTimeMs, DRV_CHACON_DIO_CLK, DRV_CHACON_DIO_RECV_ACCURACY))
		return FALSE;

	theTimeMs = iohub_chacon_dio_read_timing(aCtx);
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

BOOL iohub_chacon_dio_read_pair(chacon_dio *aCtx, u8 *aBit) 
{
    u8 theBit = 0, theBitReversed = 0;

    if (!iohub_chacon_dio_read_bit(aCtx, &theBit))
        return FALSE;
	
    if (!iohub_chacon_dio_read_bit(aCtx, &theBitReversed))
        return FALSE;
	
    if (theBit != !theBitReversed)
        return FALSE;

    *aBit = theBit;
	
	//LOG_DEBUG("%d ", (int)*aBit);
    return TRUE;
}

/* ----------------------------------------------------- */

BOOL iohub_chacon_dio_read(chacon_dio *aCtx, u32 *aSenderID, u8 *aReceiverID, BOOL *afON) 
{
    u8          theBit = 0;

	*aSenderID = 0;
	*aReceiverID = 0;
	*afON = 0;
	
        //Header
	iohub_chacon_dio_read_timing(aCtx); //DRV_CHACON_DIO_CLK
	iohub_chacon_dio_read_timing(aCtx); //10500
	iohub_chacon_dio_read_timing(aCtx); //DRV_CHACON_DIO_CLK
	iohub_chacon_dio_read_timing(aCtx); //2800
   
        //Emitter
    for (u32 i = 0; i < 26; i++)
    {
        if (!iohub_chacon_dio_read_pair(aCtx, &theBit))
            return FALSE;
		
		*aSenderID <<= 1;
        *aSenderID |= theBit;
    }

        // 26th bit -- grouped command
    if (!iohub_chacon_dio_read_pair(aCtx, &theBit))
        return FALSE;

        // 27th bit -- On or off
    if (!iohub_chacon_dio_read_pair(aCtx, &theBit))
         return FALSE;
 
	*afON = theBit;
	
        // 4 last bits -- interruptor ID
    for (u32 i = 0; i < 4; i++) 
    {
        if (!iohub_chacon_dio_read_pair(aCtx, &theBit))
            return FALSE;
		
		*aReceiverID <<= 1;
        *aReceiverID |= theBit;
    }
 
		//Footer
	iohub_chacon_dio_read_timing(aCtx); //DRV_CHACON_DIO_CLK

    return TRUE;
}

/* ----------------------------------------------------- */

BOOL iohub_chacon_dio_detectPacket(digital_async_receiver_interface_ctx *aCtx, u16 aDurationUs)
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
				//LOG_DEBUG("chacon_dio: Signal detected !");
				theCtx->mTimingReadIdx = 0;
				return TRUE;
			}
			break;
	}
	
	return FALSE;
}

/* ----------------------------------------------------- */

void iohub_chacon_dio_packetHandled(digital_async_receiver_interface_ctx *aCtx)
{
	chacon_dio *theCtx = (chacon_dio *)aCtx;
	
	theCtx->mTimingCount = 0;
}

/* ----------------------------------------------------- */

void iohub_chacon_dio_dump_timings(chacon_dio *aCtx)
{
#ifdef DEBUG
	for (u32 i=0; i<aCtx->mTimingCount; i++)
		LOG_DEBUG("%d, ", aCtx->mTimings[i]);
		
	LOG_DEBUG("");
#endif
}

/* ----------------------------------------------------- */

const digital_async_receiver_interface  *iohub_chacon_dio_get_interface(void)
{
	static const digital_async_receiver_interface sInterface = 
	{
		DRV_CHACON_DIO_ID,
		iohub_chacon_dio_detectPacket,
		iohub_chacon_dio_packetHandled
	};
	
	return &sInterface;
}