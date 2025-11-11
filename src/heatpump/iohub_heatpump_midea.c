#include "heatpump/iohub_heatpump_midea.h"
#include "utils/iohub_logs.h"

#define HEATPUMP_R51L1_BGE_RECV_ACCURACY     0.2

#define NEC_HEATPUMP_HDR_MARK		4500
#define NEC_HEATPUMP_HDR_SPACE	 	4300
#define NEC_HEATPUMP_BIT_MARK		600 //512 - 704
#define NEC_HEATPUMP_ONE_SPACE	 	1600 //1580

#define NEC_HEATPUMP_ZERO_SPACE		500   //400 - 592
#define NEC_HEATPUMP_RPT_SPACE	 	2250

#define USE_PWM 			0

//#define DEBUG 1
#if defined(DEBUG)
#	define LOG_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_BUFFER(...)				IOHUB_LOG_BUFFER(__VA_ARGS__)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_BUFFER(...)				do{}while(0)
#endif

/* ----------------------------------------------------------------- */

#define TEMPERATURE_OFF	 	0xE0
#define TEMPERATURE_FAN	 	0xE4

const u8 kTemperatureMidea[] =
{
	0x00, //17
	0x10,
	0x30,
	0x20,
	0x60, //21
	0x70,
	0x50,
	0x40, //24
	0xC0,
	0xD0,
	0x90,
	0x80,
	0xA0,
	0xB0, //30
};

#define HEATPUMP_MODE_MIDEA_COLD			0x00
#define HEATPUMP_MODE_MIDEA_DRY				0x04
#define HEATPUMP_MODE_MIDEA_AUTO			0x08
#define HEATPUMP_MODE_MIDEA_HEAT			0x0C

#define HEATPUMP_FAN_SPEED_MIDEA_AUTO		0x1F
#define HEATPUMP_FAN_SPEED_MIDEA_LOW		0x9F
#define HEATPUMP_FAN_SPEED_MIDEA_MED		0x5F
#define HEATPUMP_FAN_SPEED_MIDEA_HIGH		0x3F
#define HEATPUMP_FAN_SPEED_MIDEA_DIRECTION	0x0F
#define HEATPUMP_FAN_SPEED_MIDEA_SWING		0x6B
#define HEATPUMP_FAN_SPEED_MIDEA_OFF		0x7B

/* ----------------------------------------------------------------- */

void iohub_heatpump_midea_mark(heatpump_midea *ctx, u16 aTimeUs)
{
#if defined(USE_PWM) && USE_PWM
	TIMER_ENABLE_PWM;
	iohub_time_delay_us(aTimeUs);
#else
	iohub_digital_write(ctx->mDigitalPinTx, PinLevel_High);
	iohub_time_delay_us(aTimeUs);
#endif
}

/* ----------------------------------------------------------------- */

void iohub_heatpump_midea_space(heatpump_midea *ctx, u16 aTimeUs)
{
#if defined(USE_PWM) && USE_PWM
	TIMER_DISABLE_PWM;
	iohub_time_delay_us(aTimeUs);
#else
	iohub_digital_write(ctx->mDigitalPinTx, PinLevel_Low);
	iohub_time_delay_us(aTimeUs);
#endif
}

/* ----------------------------------------------------------------- */

ret_code_t iohub_heatpump_midea_init(heatpump_midea *ctx, digital_async_receiver *receiver, u32 txPin)
{
	memset(ctx, 0x00, sizeof(heatpump_midea));

	ctx->mDigitalPinTx = txPin;
	if (ctx->mDigitalPinTx != IOHUB_GPIO_PIN_INVALID)
		iohub_digital_set_pin_mode(ctx->mDigitalPinTx, PinMode_Output);

	iohub_digital_async_receiver_register(receiver, iohub_heatpump_midea_get_interface(), ctx);
 
	return SUCCESS;
}

/* ----------------------------------------------------------------- */

void iohub_heatpump_midea_uninit(heatpump_midea *ctx)
{
}

/* ----------------------------------------------------------------- */

void iohub_heatpump_midea_send_nec(heatpump_midea *ctx, const u8 aData[6])
{
	u8 theData[6];

	memcpy(theData, aData, sizeof(theData));
	
	iohub_interrupts_disable();

	iohub_heatpump_midea_mark(ctx, NEC_HEATPUMP_HDR_MARK);
	iohub_heatpump_midea_space(ctx, NEC_HEATPUMP_HDR_SPACE);

	for (u8 i = 0; i < sizeof(theData); i++)
	{
		for (u8 k = 0; k < 8; k++)
		{
			iohub_heatpump_midea_mark(ctx, NEC_HEATPUMP_BIT_MARK);
		
			if (theData[i] & 0x80)
				iohub_heatpump_midea_space(ctx, NEC_HEATPUMP_ONE_SPACE);
			else
				iohub_heatpump_midea_space(ctx, NEC_HEATPUMP_ZERO_SPACE);
			
			theData[i] <<= 1;
		}
	}

	iohub_heatpump_midea_mark(ctx, NEC_HEATPUMP_BIT_MARK);
	iohub_heatpump_midea_space(ctx, NEC_HEATPUMP_HDR_SPACE);

	iohub_interrupts_enable();
}

/* ----------------------------------------------------------------- */

ret_code_t iohub_heatpump_midea_set_state(heatpump_midea *ctx, const IoHubHeatpumpSettings *aSettings)
{
	u8				theValue = 0;
	u8	 			theData[6] = {0x00};

	//Start bits
	theValue = 0xB2;
	theData[0] = theValue;
	theData[1] = (theValue ^ 0xFF);
	
	//Set Fan Speed
	if (aSettings->mAction == HeatpumpAction_OFF)
		theValue = HEATPUMP_FAN_SPEED_MIDEA_OFF; //Off
	else if (aSettings->mAction == HeatpumpAction_Direction)
		theValue = HEATPUMP_FAN_SPEED_MIDEA_DIRECTION; //Direction
	else if (aSettings->mAction == HeatpumpAction_ON)
	{
		if (aSettings->mMode == HeatpumpMode_Auto || aSettings->mMode == HeatpumpMode_Dry)
		{
				//Auto mode or Dry mode always use Auto fan speed
			theValue = HEATPUMP_FAN_SPEED_MIDEA_AUTO;
		}
		else
		{
			if (aSettings->mFanSpeed == HeatpumpFanSpeed_Auto)
				theValue = HEATPUMP_FAN_SPEED_MIDEA_AUTO | 0xA0;
			else if (aSettings->mFanSpeed == HeatpumpFanSpeed_Low)
				theValue = HEATPUMP_FAN_SPEED_MIDEA_LOW;
			else if (aSettings->mFanSpeed == HeatpumpFanSpeed_Med)
				theValue = HEATPUMP_FAN_SPEED_MIDEA_MED;
			else if (aSettings->mFanSpeed == HeatpumpFanSpeed_High)
				theValue = HEATPUMP_FAN_SPEED_MIDEA_HIGH;
			else
				return E_INVALID_PARAMETERS;
		}
	}
	else
	{
		return E_INVALID_PARAMETERS;
	}

	theData[2] = theValue;
	theData[3] = (theValue ^ 0xFF);

	//Set Temperature and Mode
	if (aSettings->mAction == HeatpumpAction_OFF || aSettings->mAction == HeatpumpAction_Direction)
		theValue = TEMPERATURE_OFF;
	else if (aSettings->mMode == HeatpumpMode_Fan)
		theValue = TEMPERATURE_FAN;
	else
	{
		if (aSettings->mTemperature < 17 || aSettings->mTemperature > 30)
			return E_INVALID_PARAMETERS;

		u8 mode = HEATPUMP_MODE_MIDEA_AUTO;
		if (aSettings->mMode == HeatpumpMode_Cold)
			mode = HEATPUMP_MODE_MIDEA_COLD;
		else if (aSettings->mMode == HeatpumpMode_Heat)
			mode = HEATPUMP_MODE_MIDEA_HEAT;
		else if (aSettings->mMode == HeatpumpMode_Auto)
			mode = HEATPUMP_MODE_MIDEA_AUTO;
		else if (aSettings->mMode == HeatpumpMode_Dry)
			mode = HEATPUMP_MODE_MIDEA_DRY;
		else
			return E_INVALID_PARAMETERS;

		theValue = kTemperatureMidea[aSettings->mTemperature - 17] | mode;
	}

	theData[4] = theValue;
	theData[5] = (theValue ^ 0xFF);
 
	LOG_DEBUG("Midea: Sending");
	LOG_BUFFER(theData, sizeof(theData));
	
	for (u8 k = 0; k < 2; k++) //Send code 2 times
		iohub_heatpump_midea_send_nec(ctx, theData);

	return SUCCESS;
}

/* ----------------------------------------------------- */

static u16 iohub_heatpump_midea_read_timing(heatpump_midea *ctx) 
{
	//LOG_DEBUG("Midea: Read timing: %d", ctx->mTimings[ctx->mTimingReadIdx]);
	return ctx->mTimings[ctx->mTimingReadIdx++];
}

/* ----------------------------------------------------- */

BOOL iohub_heatpump_midea_read_bit(heatpump_midea *ctx, u8 *aBit) 
{
	BOOL 	theRet = FALSE;
    u32 	theTimeMs;

	theTimeMs = iohub_heatpump_midea_read_timing(ctx);
	if (!IS_EXPECTED_TIME(theTimeMs, NEC_HEATPUMP_BIT_MARK, HEATPUMP_R51L1_BGE_RECV_ACCURACY))
		return FALSE;

	theTimeMs = iohub_heatpump_midea_read_timing(ctx);
    if (IS_EXPECTED_TIME(theTimeMs, NEC_HEATPUMP_ZERO_SPACE, HEATPUMP_R51L1_BGE_RECV_ACCURACY))
    {
        *aBit = 0;
        theRet = TRUE;
    }
    if (IS_EXPECTED_TIME(theTimeMs, NEC_HEATPUMP_ONE_SPACE, HEATPUMP_R51L1_BGE_RECV_ACCURACY))
    {
        *aBit = 1;
        theRet = TRUE;
    }
	
	//LOG_DEBUG("%u", *aBit);
	
    return theRet;
}

/* ----------------------------------------------------- */

ret_code_t iohub_heatpump_midea_get_state(heatpump_midea *ctx, IoHubHeatpumpSettings *aSettings) 
{
	u8          theBit = 0;
	u8	 		theData[6] = {0x00};
	
		//Header
	iohub_heatpump_midea_read_timing(ctx); //NEC_CLIM_HDR_MARK
	iohub_heatpump_midea_read_timing(ctx); //NEC_CLIM_HDR_SPACE

	for (u8 i = 0; i < sizeof(theData); i++)
	{
		for (u8 k = 0; k < 8; k++)
		{
			if (!iohub_heatpump_midea_read_bit(ctx, &theBit))
				return E_INVALID_DATA;
			
			theData[i] <<= 1;
			theData[i] |= theBit;
		}
	}
	
	LOG_DEBUG("Received:");
	LOG_BUFFER(theData, sizeof(theData));
	
	if (theData[0] != (theData[1] ^ 0xFF))
		return E_INVALID_CRC;
	if (theData[2] != (theData[3] ^ 0xFF))
		return E_INVALID_CRC;
	if (theData[4] != (theData[5] ^ 0xFF))
		return E_INVALID_CRC;
	
	if (theData[2] == HEATPUMP_FAN_SPEED_MIDEA_OFF) //OFF
	{
		aSettings->mAction = HeatpumpAction_OFF;
		aSettings->mFanSpeed = HeatpumpFanSpeed_None;
	}
	else if (theData[2] == HEATPUMP_FAN_SPEED_MIDEA_DIRECTION || theData[2] == HEATPUMP_FAN_SPEED_MIDEA_SWING) //Direction || Swing
	{
			//Direction mode
		aSettings->mAction = HeatpumpAction_Direction;
		aSettings->mFanSpeed = HeatpumpFanSpeed_None;
	}
	else
	{
		aSettings->mAction = HeatpumpAction_ON;
		aSettings->mFanSpeed = HeatpumpFanSpeed_Auto; //Default is Auto to handle (0x1F) and (0x1F | 0xA0 = 0xBF)

		if (theData[2] == HEATPUMP_FAN_SPEED_MIDEA_LOW)
			aSettings->mFanSpeed = HeatpumpFanSpeed_Low;
		else if (theData[2] == HEATPUMP_FAN_SPEED_MIDEA_MED)
			aSettings->mFanSpeed = HeatpumpFanSpeed_Med;
		else if (theData[2] == HEATPUMP_FAN_SPEED_MIDEA_HIGH)
			aSettings->mFanSpeed = HeatpumpFanSpeed_High;
	}
	
	if (theData[4] == TEMPERATURE_OFF) 
	{
		aSettings->mTemperature = 0;
		aSettings->mMode = HeatpumpMode_None;
	}
	else if (theData[4] == TEMPERATURE_FAN)
	{
		aSettings->mTemperature = 0;
		aSettings->mMode = HeatpumpMode_Fan;
	}
	else
	{
		aSettings->mMode = HeatpumpMode_None;
		if ((theData[4] & 0x0C) == HEATPUMP_MODE_MIDEA_AUTO)
			aSettings->mMode = HeatpumpMode_Auto;
		else if ((theData[4] & 0x0C) == HEATPUMP_MODE_MIDEA_COLD)
			aSettings->mMode = HeatpumpMode_Cold;
		else if ((theData[4] & 0x0C) == HEATPUMP_MODE_MIDEA_HEAT)
			aSettings->mMode = HeatpumpMode_Heat;
		else if ((theData[4] & 0x0C) == HEATPUMP_MODE_MIDEA_DRY)
			aSettings->mMode = HeatpumpMode_Dry;		

		for (int i=0; i<(sizeof(kTemperatureMidea)/sizeof(u8)); i++)
		{
			if ((theData[4] & 0xF0) == kTemperatureMidea[i])
			{
				aSettings->mTemperature = i + 17;
				break;
			}
		}
	}

	aSettings->mVaneMode = HeatpumpVaneMode_Auto; //Midea does not support vane mode setting

	LOG_DEBUG("Midea: Action=%d, Mode=%d, FanSpeed=%d, Temp=%d, VaneMode=%d",
				aSettings->mAction,
				aSettings->mMode,
				aSettings->mFanSpeed,
				aSettings->mTemperature,
				aSettings->mVaneMode);
	return SUCCESS;
}

/* ----------------------------------------------------- */

BOOL iohub_heatpump_midea_detectPacket(digital_async_receiver_interface_ctx *ctx, u16 aDurationUs)
{
	heatpump_midea *theCtx = (heatpump_midea *)ctx;
	
	theCtx->mTimings[theCtx->mTimingCount++] = aDurationUs;
	
	switch(theCtx->mTimingCount)
	{
		case 1:
			if (!IS_EXPECTED_TIME(aDurationUs, NEC_HEATPUMP_HDR_MARK, HEATPUMP_R51L1_BGE_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
			
		case 2:
			if (!IS_EXPECTED_TIME(aDurationUs, NEC_HEATPUMP_HDR_SPACE, HEATPUMP_R51L1_BGE_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
			
		default:
			if (theCtx->mTimingCount == sizeof(theCtx->mTimings)/sizeof(u16))
			{
				//LOG_DEBUG("heatpump_midea: Signal detected !");
				theCtx->mTimingReadIdx = 0;
				return TRUE;
			}
			break;
	}
	
	return FALSE;
}

/* ----------------------------------------------------- */

void iohub_heatpump_midea_packetHandled(digital_async_receiver_interface_ctx *ctx)
{
	heatpump_midea *theCtx = (heatpump_midea *)ctx;
	
	theCtx->mTimingCount = 0;
}

/* ----------------------------------------------------- */

void iohub_heatpump_midea_dump_timings(heatpump_midea *ctx)
{
#ifdef DEBUG
	for (u32 i=0; i<ctx->mTimingCount; i++)
		LOG_DEBUG("%d, ", ctx->mTimings[i]);
		
	LOG_DEBUG("");
#endif
}

/* ----------------------------------------------------- */

const digital_async_receiver_interface	*iohub_heatpump_midea_get_interface(void)
{
	static const digital_async_receiver_interface sInterface = 
	{
		RECEIVER_HEATPUMP_MIDEA_ID,
		iohub_heatpump_midea_detectPacket,
		iohub_heatpump_midea_packetHandled
	};
	
	return &sInterface;
}
