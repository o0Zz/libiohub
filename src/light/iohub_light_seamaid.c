#include "light/iohub_light_seamaid.h"

//#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_ERROR(...)				IOHUB_LOG_ERROR(__VA_ARGS__)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_ERROR(...)				do{}while(0)
#endif

#define DRV_LIGHT_SEAMAID_RECV_ACCURACY     0.15

#define DRV_LIGHT_SEAMAID_BIT_LOW			250
#define DRV_LIGHT_SEAMAID_BIT_HIGH			1250

#define DRV_LIGHT_SEAMAID_BIT_TIMING		1680

#define DRV_LIGHT_SEAMAID_START_BIT_1		2900
#define DRV_LIGHT_SEAMAID_START_BIT_2		9400

#ifdef ARDUINO
	#define 	DRV_LIGHT_SEAMAID_WRITE(aLevel)	iohub_digital_write_4(aLevel)
#else
	#define 	DRV_LIGHT_SEAMAID_WRITE(aLevel)	iohub_digital_write(aCtx->mDigitalPinTx, aLevel)
#endif

/* ----------------------------------------------------------------- */

int iohub_light_seamaid_init(light_seamaid *aCtx, u8 aGPIOTx)
{
    int theRet = SUCCESS;

    memset(aCtx, 0x00, sizeof(light_seamaid));

    aCtx->mDigitalPinTx = aGPIOTx;
	
    if (aCtx->mDigitalPinTx != PIN_INVALID)
        iohub_digital_set_pin_mode(aCtx->mDigitalPinTx, PinMode_Output);
	
    return theRet;
}

/* ----------------------------------------------------------------- */

void iohub_light_seamaid_uninit(light_seamaid *aCtx)
{
}

/* ----------------------------------------------------- */

void iohub_light_seamaid_send_bit(light_seamaid *aCtx, u8 aBit) 
{
	if (aBit)
	{
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_BIT_HIGH);
		
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_BIT_LOW);
	}
	else
	{
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_BIT_LOW);
		
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_BIT_HIGH);
	}
}

/* ----------------------------------------------------- */

void iohub_light_seamaid_send(light_seamaid *aCtx, u16 anAddr, u8 aCmd)
{
	for (int k=0; k<4; k++) //Send data 4 times
	{
			//Header
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_START_BIT_1);
		
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_START_BIT_2);   
	 
			//Send Addr
		for (int i = 0; i < 16; i++)
			iohub_light_seamaid_send_bit(aCtx, (anAddr >> (15 - i)) & 0x01);
		
			//Send Cmd
		for (int i = 0; i < 8; i++)
			iohub_light_seamaid_send_bit(aCtx, (aCmd >> (7 - i)) & 0x01);
		
			//Footer
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_High);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_BIT_HIGH);
		DRV_LIGHT_SEAMAID_WRITE(PinLevel_Low);
		iohub_time_delay_us(DRV_LIGHT_SEAMAID_BIT_HIGH);   
		
		time_delay_ms(2);
	}
}

/* ----------------------------------------------------- */

static u16 iohub_light_seamaid_read_timing(light_seamaid *aCtx) 
{
	//LOG_DEBUG("%d, ", aCtx->mTimings[aCtx->mTimingReadIdx]);
	return aCtx->mTimings[aCtx->mTimingReadIdx++];
}

/* ----------------------------------------------------- */

BOOL iohub_light_seamaid_read_bit(light_seamaid *aCtx, u8 *aBit) 
{
	BOOL theRet = FALSE;
    u32 theTimeMs;

	u32 theTime1Ms = iohub_light_seamaid_read_timing(aCtx);
	u32 theTime2Ms = iohub_light_seamaid_read_timing(aCtx);
	
	if (!IS_EXPECTED_TIME(theTime1Ms + theTime2Ms, DRV_LIGHT_SEAMAID_BIT_TIMING, DRV_LIGHT_SEAMAID_RECV_ACCURACY))
		return FALSE;

	*aBit = 0;
	if (theTime1Ms > theTime2Ms)
		*aBit = 1;

    return TRUE;
}

/* ----------------------------------------------------- */

BOOL iohub_light_seamaid_read(light_seamaid *aCtx, u16 *anAddr, u8 *aCmd)
{
	u8 			i;
    u8          theBit = 0;
	
	*anAddr 		= 0;
	*aCmd 			= 0;
	
        //Header
	iohub_light_seamaid_read_timing(aCtx); //DRV_LIGHT_SEAMAID_START_BIT_1
	iohub_light_seamaid_read_timing(aCtx); //DRV_LIGHT_SEAMAID_START_BIT_2

	for(i = 0; i < 16; i++)
	{
		if (!iohub_light_seamaid_read_bit(aCtx, &theBit))
		{
			//LOG_ERROR("Invalid bit at idx %d\r\n", i);
			return FALSE;
		}
		
		*anAddr <<= 1;
		*anAddr |= theBit;
	}
	
	for(i = 0; i < 8; i++)
	{
		if (!iohub_light_seamaid_read_bit(aCtx, &theBit))
		{
			//LOG_ERROR("Invalid bit at idx %d\r\n", i);
			return FALSE;
		}
		
		*aCmd <<= 1;
		*aCmd |= theBit;
	}
	
    return TRUE;
}

/* ----------------------------------------------------- */

void iohub_light_seamaid_dump_timings(light_seamaid *aCtx)
{
#ifdef DEBUG
	for (u32 i=0; i<aCtx->mTimingCount; i++)
		LOG_DEBUG("%d, ", aCtx->mTimings[i]);
		
	LOG_DEBUG("");
#endif
}

/* ----------------------------------------------------- */

BOOL iohub_light_seamaid_detectPacket(digital_async_receiver_interface_ctx *aCtx, u16 aDurationUs)
{
	light_seamaid *theCtx = (light_seamaid *)aCtx;
	
	theCtx->mTimings[theCtx->mTimingCount++] = aDurationUs;
	
	switch(theCtx->mTimingCount)
	{
		case 1:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_LIGHT_SEAMAID_START_BIT_1, DRV_LIGHT_SEAMAID_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
			
		case 2:
			if (!IS_EXPECTED_TIME(aDurationUs, DRV_LIGHT_SEAMAID_START_BIT_2, DRV_LIGHT_SEAMAID_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
		
		default:
			if (theCtx->mTimingCount == sizeof(theCtx->mTimings)/sizeof(u16))
			{
				LOG_DEBUG("iohub_light_seamaid: Signal detected !");
				theCtx->mTimingReadIdx = 0;
				return TRUE;
			}
			break;
	}
	
	return FALSE;
}

/* ----------------------------------------------------- */

void iohub_light_seamaid_packetHandled(digital_async_receiver_interface_ctx *aCtx)
{
	light_seamaid *theCtx = (light_seamaid *)aCtx;
	
	theCtx->mTimingCount = 0;
}

/* ----------------------------------------------------- */

const digital_async_receiver_interface  *iohub_light_seamaid_get_interface(void)
{
	static const digital_async_receiver_interface sInterface = 
	{
		DRV_LIGHT_SEAMAID_ID,
		iohub_light_seamaid_detectPacket,
		iohub_light_seamaid_packetHandled
	};
	
	return &sInterface;
}
