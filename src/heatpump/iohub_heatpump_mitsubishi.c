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

#define MITSUBISHI_BYTE_TIMEOUT			0xFFFF
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

typedef struct heatpump_pkt_s
{
	u8 mCmd;
	u8 mSize;
	u8 mData[16];
}heatpump_pkt;

/* -------------------------------------------------------------- */

	static u8 iohub_heatpump_mitsubishi_crc(u8 *aBuffer, u8 aSize)
	{
		u32 theSum = 0;
		for (u8 i=0; i<aSize; i++)
			theSum += aBuffer[i];
		
		return 0xFC - (theSum & 0xFF);
	}
		
	/* -------------------------------------------------------------- */
		
	static int iohub_heatpump_mitsubishi_read_byte(heatpump_mitsubishi_ctx *aCtx, u16 aTimeoutMs)
	{
		u8 theByte = 0xFF;
		u16 theSize = 0;
		
		u32 theStartTime = IOHUB_TIMER_START();
		
		while ((IOHUB_TIMER_ELAPSED(theStartTime) < aTimeoutMs) && (theSize == 0))
		{
			theSize = 1;
			iohub_uart_read(aCtx->mUartCtx, &theByte, &theSize);
		}
		
		if (theSize == 1)
		{
			LOG_DEBUG("Mitsu ReadByte: %X", theByte);
			return theByte;
		}
		
		IOHUB_LOG_ERROR("Mitsu ReadByte Timeout (%d ms)", aTimeoutMs);
		return MITSUBISHI_BYTE_TIMEOUT;
	}

	/* -------------------------------------------------------------- */
		
	static ret_code_t iohub_heatpump_mitsubishi_read_pkt(heatpump_mitsubishi_ctx *aCtx, heatpump_pkt *aPkt)
	{
		int theByte = 0x00;
		while (theByte != 0xFC)
		{
			theByte = iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS);
			if (theByte == MITSUBISHI_BYTE_TIMEOUT)
				return E_TIMEOUT;
		}
		
		theByte = iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS);
		if (theByte == MITSUBISHI_BYTE_TIMEOUT)
			return E_TIMEOUT;

		aPkt->mCmd = theByte & 0xFF;

		if (iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS) != 0x01)
			return E_INVALID_REPLY;
		
		if (iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS) != 0x30)
			return E_INVALID_REPLY;
		
		theByte = iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS);
		if (theByte == MITSUBISHI_BYTE_TIMEOUT)
			return E_TIMEOUT;

		aPkt->mSize = theByte & 0xFF;

		for (int i=0; i<aPkt->mSize; i++)
		{
			theByte = iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS);
			if (theByte == MITSUBISHI_BYTE_TIMEOUT)
				return E_TIMEOUT;
			aPkt->mData[i] = theByte & 0xFF;
		}
		
		int theChecksum = iohub_heatpump_mitsubishi_read_byte(aCtx, MITSUBISHI_TIMEOUT_MS) & 0xFF;
		if (theChecksum == MITSUBISHI_BYTE_TIMEOUT)
			return E_TIMEOUT;

		//TODO: Check CRC here
		
		return SUCCESS;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_mitsubishi_write_pkt(heatpump_mitsubishi_ctx *aCtx, heatpump_pkt *aPkt)
	{
		u8 theBuffer[32] = {0xFC, aPkt->mCmd, 0x01, 0x30, aPkt->mSize};
		memcpy(&theBuffer[5], aPkt->mData, aPkt->mSize);
		theBuffer[5 + aPkt->mSize] = iohub_heatpump_mitsubishi_crc(theBuffer, aPkt->mSize + 5);
		
		LOG_DEBUG("Mitsu Write:");
		LOG_BUFFER(theBuffer, aPkt->mSize + 6);
		
		return iohub_uart_write(aCtx->mUartCtx, theBuffer, aPkt->mSize + 6);
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_mitsubishi_connect(heatpump_mitsubishi_ctx *aCtx)
	{
		heatpump_pkt thePacket = 
		{
			.mCmd = 0x5A,
			.mSize = 0x02,
			.mData = {0xCA, 0x01}
		};

		ret_code_t theRet = iohub_heatpump_mitsubishi_write_pkt(aCtx, &thePacket);
		if (theRet == SUCCESS)
		{
			theRet = iohub_heatpump_mitsubishi_read_pkt(aCtx, &thePacket);
			if (theRet == SUCCESS)	
			{	
				if (thePacket.mCmd == (0x5A | MITSUBISHI_PROTO_REPLY))
				{
					IOHUB_LOG_INFO("Mitsubishi connected !");
					aCtx->mfConnected = TRUE;
					return SUCCESS;
				}
			}
		}

		IOHUB_LOG_ERROR("Mitsubishi not connected");
		return E_INVALID_NOT_CONNECTED;
	}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_init(heatpump_mitsubishi_ctx *aCtx, uart_ctx *anUART)
{
	memset(aCtx, 0x00, sizeof(heatpump_mitsubishi_ctx));
	
	aCtx->mUartCtx = anUART;

	ret_code_t ret = iohub_uart_open(aCtx->mUartCtx, 2400, IOHubUartParity_Even, 1);
	if (ret != SUCCESS)
		return ret;

	return iohub_heatpump_mitsubishi_connect(aCtx);
}

/* -------------------------------------------------------------- */

void iohub_heatpump_mitsubishi_uninit(heatpump_mitsubishi_ctx *aCtx)
{
	iohub_uart_close(aCtx->mUartCtx);
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_send(heatpump_mitsubishi_ctx *aCtx, IoHubHeatpumpAction anAction, int aTemperature, IoHubHeatpumpFanSpeed aFanSpeed, IoHubHeatpumpMode aMode)
{
	ret_code_t 		theRet;
	heatpump_pkt 	thePkt;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	
	if (!aCtx->mfConnected)
	{
		theRet = iohub_heatpump_mitsubishi_connect(aCtx);
		if (theRet != SUCCESS)
			return theRet;
	}
	
	thePkt.mCmd = 0x41; //Set
	thePkt.mSize = 16;
	
	thePkt.mData[0] = MitsubishiPacketType_SetSettingsInformation;
	thePkt.mData[1] |= 0x0F; // Inform we want to change power 0x01, mode 0x02, temp 0x04, fan 0x08, (VANE:0x10 DIR:0x80 not changed) 
	
	//Power
	thePkt.mData[3] = (anAction == HeatpumpAction_ON) ? 0x01 : 0x00;
	
	//Mode
	thePkt.mData[4] = kHeatpumpModeMitsubishi[aMode];

	//Temp
	thePkt.mData[5] = 0xF - (aTemperature - 16);

	//Fan
	thePkt.mData[6] = kFanSpeedMitsubishi[aFanSpeed];

	//Vane
	thePkt.mData[7] = 0x00; //0x00=Auto, 0x01=1, 0x02=2 ..., 0x07=SWING
	
	//Dir
	thePkt.mData[10] = 0x00;
	
	theRet = iohub_heatpump_mitsubishi_write_pkt(aCtx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	theRet = iohub_heatpump_mitsubishi_read_pkt(aCtx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	if (thePkt.mCmd != (0x41 | MITSUBISHI_PROTO_REPLY))
		return E_INVALID_REPLY; 
	
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_read(heatpump_mitsubishi_ctx *aCtx, IoHubHeatpumpAction *anAction, int *aTemperature, IoHubHeatpumpFanSpeed *aFanSpeed, IoHubHeatpumpMode *aMode)
{
	ret_code_t 		theRet;
	heatpump_pkt 	thePkt;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
			
	if (!aCtx->mfConnected)
	{
		theRet = iohub_heatpump_mitsubishi_connect(aCtx);
		if (theRet != SUCCESS)
			return theRet;
	}

	thePkt.mCmd = 0x42; //Get
	thePkt.mSize = 16;
	thePkt.mData[0] = MitsubishiPacketType_GetSettingsInformation; //Get Settings Info
	
	theRet = iohub_heatpump_mitsubishi_write_pkt(aCtx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	theRet = iohub_heatpump_mitsubishi_read_pkt(aCtx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;

	if (thePkt.mData[0] != MitsubishiPacketType_GetSettingsInformation)
		return E_INVALID_DATA;

		//Action
	*anAction = (thePkt.mData[3] == 0x01) ? HeatpumpAction_ON : HeatpumpAction_OFF;
	
		//Mode
	for (int theMode = HeatpumpMode_None; theMode < HeatpumpMode_Count; theMode++)
	{
		if (thePkt.mData[4] == kHeatpumpModeMitsubishi[theMode])
		{
			*aMode = (IoHubHeatpumpMode)theMode;
			break;
		}
	}
	
		//Temp 0x0F = 16 ... 0x06 = 25
	*aTemperature = (0xF - thePkt.mData[5]) + 16;

		//Fan (see kFanSpeedMitsubishi for more information)
	if (thePkt.mData[6] == 0x01 || thePkt.mData[6] == 0x02)
		*aFanSpeed = HeatpumpFanSpeed_Low;
	else if (thePkt.mData[6] == 0x03 || thePkt.mData[6] == 0x04)
		*aFanSpeed = HeatpumpFanSpeed_Med;
	else if (thePkt.mData[6] == 0x05 || thePkt.mData[6] == 0x06)
		*aFanSpeed = HeatpumpFanSpeed_High;
	else //Include 0x00 and all others
		*aFanSpeed = HeatpumpFanSpeed_Auto;
	
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_mitsubishi_read_room_temperature(heatpump_mitsubishi_ctx *aCtx, float *aTemperature)
{
	ret_code_t 		theRet;
	heatpump_pkt 	thePkt;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
			
	if (!aCtx->mfConnected)
	{
		theRet = iohub_heatpump_mitsubishi_connect(aCtx);
		if (theRet != SUCCESS)
			return theRet;
	}

	thePkt.mCmd = 0x42; //Get
	thePkt.mSize = 16;
	thePkt.mData[0] = MitsubishiPacketType_GetRoomTemperature;
	
	theRet = iohub_heatpump_mitsubishi_write_pkt(aCtx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;
	
	memset(&thePkt, 0x00, sizeof(thePkt));
	theRet = iohub_heatpump_mitsubishi_read_pkt(aCtx, &thePkt);
	if (theRet != SUCCESS)
		return theRet;

	if (thePkt.mData[0] != MitsubishiPacketType_GetRoomTemperature)
		return E_INVALID_DATA;

	*aTemperature = thePkt.mData[3] + 10.0;
	if (thePkt.mData[6] != 0x00) //More accurate value
		*aTemperature = (thePkt.mData[6] & 0x7F) / 2.0;
	
	return SUCCESS;
}
