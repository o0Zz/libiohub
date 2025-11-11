#include "heatpump/iohub_heatpump_midea.h"
#include "utils/iohub_logs.h"

#define DRV_CLIM_R51L1_BGE_RECV_ACCURACY     0.2

#define NEC_CLIM_HDR_MARK		4500
#define NEC_CLIM_HDR_SPACE	 	4300
#define NEC_CLIM_BIT_MARK		600 //512 - 704
#define NEC_CLIM_ONE_SPACE	 	1600 //1580

#define NEC_CLIM_ZERO_SPACE		500   //400 - 592
#define NEC_CLIM_RPT_SPACE	 	2250

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

#define TEMPERATURE_NONE	 	0xE0

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

const u8 kHeatpumpModeMidea[] =
{
	0x0F, //None //Invalid !
	
	0x00, //Cold
	0x04, //Dry HeatpumpFanSpeed_Auto
	0x05, //Fan TEMPERATURE_NONE + HeatpumpMode_Dry
	0x08, //Auto HeatpumpFanSpeed_Auto
	0x0C, //Heat
};

const u8 kHeatpumpFanSpeedMidea[] =
{
	0x0F, //None
	
	0xBF, //Auto
	0x3F, //High
	0x5F, //Med
	0x9F, //Low
};

u8 kHeatpumpFanSpeedMideaDryAuto = 0x1F;

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

int iohub_heatpump_midea_init(heatpump_midea *ctx, digital_async_receiver *receiver, u32 txPin)
{
	int theRet = SUCCESS;

	memset(ctx, 0x00, sizeof(heatpump_midea));

	ctx->mDigitalPinTx = txPin;
	if (ctx->mDigitalPinTx != IOHUB_GPIO_PIN_INVALID)
		iohub_digital_set_pin_mode(ctx->mDigitalPinTx, PinMode_Output);

	iohub_digital_async_receiver_register(receiver, iohub_heatpump_midea_get_interface(), ctx);
 
	return theRet;
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
	
	iohub_heatpump_midea_mark(ctx, NEC_CLIM_HDR_MARK);
	iohub_heatpump_midea_space(ctx, NEC_CLIM_HDR_SPACE);
	
	for (u8 i = 0; i < sizeof(theData); i++)
	{
		for (u8 k = 0; k < 8; k++)
		{
			iohub_heatpump_midea_mark(ctx, NEC_CLIM_BIT_MARK);
		
			if (theData[i] & 0x80)
				iohub_heatpump_midea_space(ctx, NEC_CLIM_ONE_SPACE);
			else
				iohub_heatpump_midea_space(ctx, NEC_CLIM_ZERO_SPACE);
			
			theData[i] <<= 1;
		}
	}
	
	iohub_heatpump_midea_mark(ctx, NEC_CLIM_BIT_MARK);
	iohub_heatpump_midea_space(ctx, NEC_CLIM_HDR_SPACE);
}

/* ----------------------------------------------------------------- */

int iohub_heatpump_midea_set_state(heatpump_midea *ctx, const IoHubHeatpumpSettings *aSettings)
{
	u8				theValue = 0;
	u8	 			theData[6] = {0x00};

	//Start bits
	theValue = 0xB2;
	theData[0] = theValue;
	theData[1] = (theValue ^ 0xFF);
	
	//Set Fan Speed
	if (aSettings->mAction == HeatpumpAction_OFF)
		theValue = 0x7B;
	else if (aSettings->mAction == HeatpumpAction_Direction)
		theValue = HeatpumpFanSpeed_None;
	else if (aSettings->mAction == HeatpumpAction_ON)
	{
		theValue = kHeatpumpFanSpeedMidea[aSettings->mFanSpeed];
		if (aSettings->mMode == HeatpumpMode_Dry || aSettings->mMode == HeatpumpMode_Auto)
			theValue = kHeatpumpFanSpeedMideaDryAuto; //Ignore Fan speed: AUTO
	}
	else
	{
		return E_INVALID_PARAMETERS;
	}

	theData[2] = theValue;
	theData[3] = (theValue ^ 0xFF);

	//Set Temperature and Mode
	if (aSettings->mAction == HeatpumpAction_OFF || aSettings->mAction == HeatpumpAction_Direction)
		theValue = TEMPERATURE_NONE;
	else if (aSettings->mMode == HeatpumpMode_Fan)
		theValue = TEMPERATURE_NONE | HeatpumpMode_Dry;
	else
	{
		if (aSettings->mTemperature < 17 || aSettings->mTemperature > 30)
			return E_INVALID_PARAMETERS;

		theValue = kTemperatureMidea[aSettings->mTemperature - 17] | kHeatpumpModeMidea[aSettings->mMode];
	}

	theData[4] = theValue;
	theData[5] = (theValue ^ 0xFF);
 
	LOG_DEBUG("Sending");
	LOG_BUFFER(theData, sizeof(theData));
	
	for (u8 k = 0; k < 2; k++) //Send code 2 times
		iohub_heatpump_midea_send_nec(ctx, theData);

	return SUCCESS;
}

/* ----------------------------------------------------- */

static u16 iohub_heatpump_midea_read_timing(heatpump_midea *ctx) 
{
	LOG_DEBUG("%d, ", ctx->mTimings[ctx->mTimingReadIdx]);
	return ctx->mTimings[ctx->mTimingReadIdx++];
}

/* ----------------------------------------------------- */

BOOL iohub_heatpump_midea_read_bit(heatpump_midea *ctx, u8 *aBit) 
{
	BOOL 	theRet = FALSE;
    u32 	theTimeMs;

	theTimeMs = iohub_heatpump_midea_read_timing(ctx);
	if (!IS_EXPECTED_TIME(theTimeMs, NEC_CLIM_BIT_MARK, DRV_CLIM_R51L1_BGE_RECV_ACCURACY))
		return FALSE;

	theTimeMs = iohub_heatpump_midea_read_timing(ctx);
    if (IS_EXPECTED_TIME(theTimeMs, NEC_CLIM_ZERO_SPACE, DRV_CLIM_R51L1_BGE_RECV_ACCURACY))
    {
        *aBit = 0;
        theRet = TRUE;
    }
    if (IS_EXPECTED_TIME(theTimeMs, NEC_CLIM_ONE_SPACE, DRV_CLIM_R51L1_BGE_RECV_ACCURACY))
    {
        *aBit = 1;
        theRet = TRUE;
    }
	
	//LOG_DEBUG("%u", *aBit);
	
    return theRet;
}

/* ----------------------------------------------------- */

BOOL iohub_heatpump_midea_get_state(heatpump_midea *ctx, IoHubHeatpumpSettings *aSettings) 
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
				return FALSE;
			
			theData[i] <<= 1;
			theData[i] |= theBit;
		}
	}
	
	LOG_DEBUG("Received:");
	LOG_BUFFER(theData, sizeof(theData));
	
	if (theData[0] != (theData[1] ^ 0xFF))
		return FALSE;
	if (theData[2] != (theData[3] ^ 0xFF))
		return FALSE;
	if (theData[4] != (theData[5] ^ 0xFF))
		return FALSE;
	
	if (theData[2] == 0x7B)
	{
		aSettings->mAction = HeatpumpAction_OFF;
		aSettings->mFanSpeed = HeatpumpFanSpeed_None;
	}
	else if (theData[2] == HeatpumpFanSpeed_None)
	{
		aSettings->mAction = HeatpumpAction_Direction;
		aSettings->mFanSpeed = HeatpumpFanSpeed_None;
	}
	else
	{
		aSettings->mAction = HeatpumpAction_ON;
		aSettings->mFanSpeed = HeatpumpFanSpeed_Auto; //In case of kHeatpumpFanSpeedMideaDryAuto, aFanSpeed will be set to Auto

		for (int i=0; i<HeatpumpFanSpeed_Count; i++)
		{
			if (theData[2] == kHeatpumpFanSpeedMidea[i])
			{
				aSettings->mFanSpeed = (IoHubHeatpumpFanSpeed)i;
				break;
			}
		}

	}

	if (theData[4] == TEMPERATURE_NONE) //OFF or direction
	{
		aSettings->mTemperature = 0;
		aSettings->mMode = HeatpumpMode_None;
	}
	else if (theData[4] == (TEMPERATURE_NONE | HeatpumpMode_Dry))
	{
		aSettings->mMode = HeatpumpMode_Fan;
	}
	else
	{
		for (int i=0; i<HeatpumpMode_Count; i++)
		{
			if ((theData[4] & 0x0F) == kHeatpumpModeMidea[i])
			{
				aSettings->mMode = (IoHubHeatpumpMode)i;
				break;
			}	
		}
		
		for (int i=0; i<(sizeof(kTemperatureMidea)/sizeof(u8)); i++)
		{
			if ((theData[4] & 0xF0) == kTemperatureMidea[i])
			{
				aSettings->mTemperature = i + 17;
				break;
			}
		}
	}

	return TRUE;
}

/* ----------------------------------------------------- */

BOOL iohub_heatpump_midea_detectPacket(digital_async_receiver_interface_ctx *ctx, u16 aDurationUs)
{
	heatpump_midea *theCtx = (heatpump_midea *)ctx;
	
	theCtx->mTimings[theCtx->mTimingCount++] = aDurationUs;
	
	switch(theCtx->mTimingCount)
	{
		case 1:
			if (!IS_EXPECTED_TIME(aDurationUs, NEC_CLIM_HDR_MARK, DRV_CLIM_R51L1_BGE_RECV_ACCURACY))
				theCtx->mTimingCount = 0;
			break;
			
		case 2:
			if (!IS_EXPECTED_TIME(aDurationUs, NEC_CLIM_HDR_SPACE, DRV_CLIM_R51L1_BGE_RECV_ACCURACY))
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
