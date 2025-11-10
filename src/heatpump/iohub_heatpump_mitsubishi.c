#include "heatpump/iohub_heatpump_mitsubishi.h"
#include "utils/iohub_types.h"
#include "utils/iohub_logs.h"
#include "utils/iohub_time.h"

//https://github.com/hadleyrich/MQMitsi/blob/master/mitsi.py
//https://github.com/SwiCago/HeatPump/blob/master/src/HeatPump.cpp

#define DEBUG

#ifdef DEBUG
#	define LOG_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_BUFFER(buf, size)		IOHUB_LOG_BUFFER(buf, size)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_BUFFER(buf, size)		do{}while(0)
#endif

typedef enum 
{
	MitsubishiPacketType_Unknown = 0x00,
	MitsubishiPacketType_SetSettingsInformation,
	MitsubishiPacketType_GetSettingsInformation,
	MitsubishiPacketType_GetRoomTemperature,
	MitsubishiPacketType_SetTimer,
	MitsubishiPacketType_GetTimer,
	MitsubishiPacketType_GetStatus
}MitsubishiPacketType;

/* 
----------------------------------------
				Packets
----------------------------------------
	0xFC CMD 0x01 0x30 SIZE DATA CRC
*/

#define MITSUBISHI_STX					0xFC
#define MITSUBISHI_TIMEOUT_MS			200

#define MITSUBISHI_PROTO_REPLY			0x20

static const u8 kHeatpumpModeMitsubishi[HeatpumpMode_Count] = 
{
	0xFF, //None
	0x03, //Cold
	0x02, //Dry
	0x07, //Fan
	0x08, //Auto
	0x01 //Heat
};
	
static const u8 kFanSpeedMitsubishi[HeatpumpFanSpeed_Count] = 
{
	0xFF, //None
	0x00, //Auto
	
		//Fan goes from 0x01 to 0x06 (Choose arbritrary values)
	0x06, //High = 05 or 06
	0x03, //Med = 03 or 04
	0x01  //Low = 01 or 02
};

static const u8 kVaneModeMitsubishi[HeatpumpVaneMode_Count] = 
{
	0x00, //Auto
	0x01, //1
	0x02, //2
	0x03, //3
	0x04, //4
	0x05, //5
	0x07  //Swing
};

typedef struct heatpump_pkt_s
{
	u8 mSTX;
	u8 mCmd;
	u8 mHeader[2];
	u8 mSize;
	u8 mData[16];
	u8 mChecksum;
}__attribute__((packed))  heatpump_pkt;

/* -------------------------------------------------------------- */

	static u8 iohub_heatpump_mitsubishi_crc(u8 *aBuffer, u8 aSize)
	{
		u32 crc = 0;

		// Skip first byte (STX)
		for (u8 i=1; i<aSize; i++)
			crc += aBuffer[i];
		
		return (u8)(-((u8)crc));
	}
		
	/* -------------------------------------------------------------- */
		
	static ret_code_t iohub_heatpump_mitsubishi_read_byte(heatpump_mitsubishi_ctx *ctx, u8 *byte, u16 timeoutMs)
	{
		u16 size = 0;
		
		u32 theStartTime = IOHUB_TIMER_START();

		while ((IOHUB_TIMER_ELAPSED(theStartTime) < timeoutMs) && (size == 0))
		{
			size = 1;
			iohub_uart_read(ctx->mUartCtx, byte, &size);
		}

		if (size == 1)
		{
			//LOG_DEBUG("Mitsu ReadByte: %X", *byte);
			return SUCCESS;
		}
		
		IOHUB_LOG_ERROR("Mitsu ReadByte Timeout (%d ms)", timeoutMs);
		return E_TIMEOUT;
	}

	/* -------------------------------------------------------------- */
		
	static ret_code_t iohub_heatpump_mitsubishi_read_pkt(heatpump_mitsubishi_ctx *ctx, heatpump_pkt *aPkt)
	{
		aPkt->mSTX = 0x00;
		while (aPkt->mSTX != MITSUBISHI_STX)
		{
			if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mSTX, MITSUBISHI_TIMEOUT_MS) == E_TIMEOUT)
			{
				//ctx->mfConnected = FALSE; // Mark as disconnected to retry connection
				return E_TIMEOUT;
			}

			if (aPkt->mSTX != MITSUBISHI_STX) {
				IOHUB_LOG_WARNING("Mitsu: Discarded byte: 0x%02X", aPkt->mSTX);
			}
		}
		if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mCmd, MITSUBISHI_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mHeader[0], MITSUBISHI_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mHeader[1], MITSUBISHI_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mSize, MITSUBISHI_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
			
		for (int i=0; i<aPkt->mSize; i++)
		{
			if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mData[i], MITSUBISHI_TIMEOUT_MS) != SUCCESS)
				return E_TIMEOUT;
		}

		if (iohub_heatpump_mitsubishi_read_byte(ctx, &aPkt->mChecksum, MITSUBISHI_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;

		u8 calculatedCrc = iohub_heatpump_mitsubishi_crc((u8*)aPkt, aPkt->mSize + 5);
		if (calculatedCrc != aPkt->mChecksum)
			IOHUB_LOG_ERROR("Mitsu: Invalid crc rcv=0x%02X, calc=0x%02X continue...", aPkt->mChecksum, calculatedCrc);

		return SUCCESS;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_mitsubishi_write_pkt(heatpump_mitsubishi_ctx *ctx, heatpump_pkt *aPkt)
	{
		u8 theBuffer[32] = {MITSUBISHI_STX, aPkt->mCmd, 0x01, 0x30, aPkt->mSize};
		memcpy(&theBuffer[5], aPkt->mData, aPkt->mSize);
		theBuffer[5 + aPkt->mSize] = iohub_heatpump_mitsubishi_crc(theBuffer, aPkt->mSize + 5);
		
		LOG_DEBUG("Mitsu Write:");
		LOG_BUFFER(theBuffer, aPkt->mSize + 6);
		
		return iohub_uart_write(ctx->mUartCtx, theBuffer, aPkt->mSize + 6);
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_mitsubishi_connect(heatpump_mitsubishi_ctx *ctx)
	{
		heatpump_pkt thePacket = 
		{
			.mCmd = 0x5A,
			.mSize = 0x02,
			.mData = {0xCA, 0x01}
		};

		ret_code_t theRet = iohub_heatpump_mitsubishi_write_pkt(ctx, &thePacket);
		if (theRet == SUCCESS)
		{
			theRet = iohub_heatpump_mitsubishi_read_pkt(ctx, &thePacket);
			if (theRet == SUCCESS)	
			{	
				if (thePacket.mCmd == (0x5A | MITSUBISHI_PROTO_REPLY))
				{
					IOHUB_LOG_INFO("Mitsubishi connected !");
					ctx->mfConnected = TRUE;
					return SUCCESS;
				}
			}
		}

		IOHUB_LOG_ERROR("Mitsubishi not connected");
		return E_INVALID_NOT_CONNECTED;
	}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_init(heatpump_mitsubishi_ctx *ctx, uart_ctx *anUART)
{
	memset(ctx, 0x00, sizeof(heatpump_mitsubishi_ctx));
	
	ctx->mUartCtx = anUART;

	ret_code_t ret = iohub_uart_open(ctx->mUartCtx, 2400, IOHubUartParity_Even, 1);
	if (ret != SUCCESS)
		return ret;

	return iohub_heatpump_mitsubishi_connect(ctx);
}

/* -------------------------------------------------------------- */

void iohub_heatpump_mitsubishi_uninit(heatpump_mitsubishi_ctx *ctx)
{
	iohub_uart_close(ctx->mUartCtx);
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_set_state(heatpump_mitsubishi_ctx *ctx, const IoHubHeatpumpSettings *aSettings)
{
	ret_code_t 		theRet;
	heatpump_pkt 	thePkt;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	
	if (!ctx->mfConnected)
	{
		theRet = iohub_heatpump_mitsubishi_connect(ctx);
		if (theRet != SUCCESS)
			return theRet;
	}
	
	thePkt.mCmd = 0x41; //Set
	thePkt.mSize = 16;
	
	thePkt.mData[0] = MitsubishiPacketType_SetSettingsInformation;
	thePkt.mData[1] |= 0x0F; // Inform we want to change power 0x01, mode 0x02, temp 0x04, fan 0x08, (VANE:0x10 DIR:0x80 not changed) 
	
	//Power
	thePkt.mData[3] = (aSettings->mAction == HeatpumpAction_ON) ? 0x01 : 0x00;
	
	//Mode
	thePkt.mData[4] = kHeatpumpModeMitsubishi[aSettings->mMode];

	//Temp
	thePkt.mData[5] = 0xF - (aSettings->mTemperature - 16);

	//Fan
	thePkt.mData[6] = kFanSpeedMitsubishi[aSettings->mFanSpeed];

	//Vane
	thePkt.mData[7] = kVaneModeMitsubishi[aSettings->mVaneMode]; //0x00=Auto, 0x01=1, 0x02=2 ..., 0x07=SWING
	
	//Dir
	thePkt.mData[10] = 0x00;
	
	theRet = iohub_heatpump_mitsubishi_write_pkt(ctx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	theRet = iohub_heatpump_mitsubishi_read_pkt(ctx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	if (thePkt.mCmd != (0x41 | MITSUBISHI_PROTO_REPLY))
		return E_INVALID_REPLY; 
	
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_get_state(heatpump_mitsubishi_ctx *ctx, IoHubHeatpumpSettings *aSettings)
{
	ret_code_t 		theRet;
	heatpump_pkt 	thePkt;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
			
	if (!ctx->mfConnected)
	{
		theRet = iohub_heatpump_mitsubishi_connect(ctx);
		if (theRet != SUCCESS)
			return theRet;
	}

	memset(aSettings, 0x00, sizeof(IoHubHeatpumpSettings));

	thePkt.mCmd = 0x42; //Get
	thePkt.mSize = 16;
	thePkt.mData[0] = MitsubishiPacketType_GetSettingsInformation; //Get Settings Info
	
	theRet = iohub_heatpump_mitsubishi_write_pkt(ctx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	theRet = iohub_heatpump_mitsubishi_read_pkt(ctx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;

	if (thePkt.mData[0] != MitsubishiPacketType_GetSettingsInformation)
		return E_INVALID_DATA;

		//Action
	aSettings->mAction = (thePkt.mData[3] == 0x01) ? HeatpumpAction_ON : HeatpumpAction_OFF;

		//Mode
	for (int theMode = HeatpumpMode_None; theMode < HeatpumpMode_Count; theMode++)
	{
		if (thePkt.mData[4] == kHeatpumpModeMitsubishi[theMode])
		{
			aSettings->mMode = (IoHubHeatpumpMode)theMode;
			break;
		}
	}
	
		//Temp 0x0F = 16 ... 0x06 = 25
	aSettings->mTemperature = (0xF - thePkt.mData[5]) + 16;

		//Fan (see kFanSpeedMitsubishi for more information)
	if (thePkt.mData[6] == 0x01 || thePkt.mData[6] == 0x02)
		aSettings->mFanSpeed = HeatpumpFanSpeed_Low;
	else if (thePkt.mData[6] == 0x03 || thePkt.mData[6] == 0x04)
		aSettings->mFanSpeed = HeatpumpFanSpeed_Med;
	else if (thePkt.mData[6] == 0x05 || thePkt.mData[6] == 0x06)
		aSettings->mFanSpeed = HeatpumpFanSpeed_High;
	else //Include 0x00 and all others
		aSettings->mFanSpeed = HeatpumpFanSpeed_Auto;

		//Vane
	for (int vaneMode = HeatpumpVaneMode_Auto; vaneMode < HeatpumpVaneMode_Count; vaneMode++)
	{
		if (thePkt.mData[7] == kVaneModeMitsubishi[vaneMode])
		{
			aSettings->mVaneMode = (IoHubHeatpumpVaneMode)vaneMode;
			break;
		}
	}

	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_get_room_temperature(heatpump_mitsubishi_ctx *ctx, float *aTemperature)
{
	ret_code_t 		theRet;
	heatpump_pkt 	thePkt;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
			
	if (!ctx->mfConnected)
	{
		theRet = iohub_heatpump_mitsubishi_connect(ctx);
		if (theRet != SUCCESS)
			return theRet;
	}

	thePkt.mCmd = 0x42; //Get
	thePkt.mSize = 16;
	thePkt.mData[0] = MitsubishiPacketType_GetRoomTemperature;
	
	theRet = iohub_heatpump_mitsubishi_write_pkt(ctx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	theRet = iohub_heatpump_mitsubishi_read_pkt(ctx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;

	if (thePkt.mData[0] != MitsubishiPacketType_GetRoomTemperature)
		return E_INVALID_DATA;

	*aTemperature = thePkt.mData[3] + 10.0;
	if (thePkt.mData[6] != 0x00) //More accurate value
		*aTemperature = (thePkt.mData[6] & 0x7F) / 2.0;
	
	return SUCCESS;
}
