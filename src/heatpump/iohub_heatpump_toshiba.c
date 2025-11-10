#include "heatpump/iohub_heatpump_toshiba.h"
#include "utils/iohub_types.h"
#include "utils/iohub_logs.h"
#include "utils/iohub_time.h"
#include "utils/iohub_errors.h"
#include "platform/iohub_platform.h"

//https://github.com/burrocargado/toshiba-aircon-mqtt-bridge/tree/master
//https://github.com/ormsport/ToshibaCarrierHvac/blob/main/src/ToshibaCarrierHvac.cpp
// https://github.com/IntExCZ/Toshiba-HVAC/blob/main/HomeAssistant/AppDaemon/apps/toshiba-hvac.py

//Daikin: https://github.com/Arnold-n/P1P2MQTT
#define DEBUG

#ifdef DEBUG
#	define LOG_DEBUG(...)				IOHUB_LOG_INFO(__VA_ARGS__)
#	define LOG_BUFFER(buf, size)		IOHUB_LOG_BUFFER(buf, size)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_BUFFER(buf, size)		do{}while(0)
#endif

typedef enum 
{
	ToshibaPacketType_Query = 0x01,
	ToshibaPacketType_Command = 0x02
} ToshibaPacketType;

/* -------------------------------------------------------------- */

#define TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS		600
#define TOSHIBA_TIME_BETWEEN_PACKET_QUERY_MS		200

// Toshiba protocol constants
#define TOSHIBA_MAX_PACKET_SIZE				0xFF
#define TOSHIBA_HANDSHAKE_TIMEOUT_MS		5000

#define TOSHIBA_COMMAND_TIMEOUT_MS			1000
#define TOSHIBA_PACKET_READ_TIMEOUT_MS		250

#define TOSHIBA_PACKET_STX					0x02

// Packet types
#define TOSHIBA_PACKET_TYPE_REPLY_MASK		0x80

#define TOSHIBA_PACKET_TYPE_SYN				0x00
#define TOSHIBA_PACKET_TYPE_UNKNOWN1		0x01
#define TOSHIBA_PACKET_TYPE_UNKNOWN2		0x02
#define TOSHIBA_PACKET_TYPE_COMMAND			0x10
#define TOSHIBA_PACKET_TYPE_STATUS		0x11

// Function codes
#define TOSHIBA_FUNCTION_POWER_STATE		0x80
#define TOSHIBA_FUNCTION_POWER_SELECTION	0x87
#define TOSHIBA_FUNCTION_STATUS				0x88
#define TOSHIBA_FUNCTION_TIMER_ON			0x90
#define TOSHIBA_FUNCTION_TIMER_OFF			0x94
#define TOSHIBA_FUNCTION_FANMODE			0xA0
#define TOSHIBA_FUNCTION_SWING				0xA3
#define TOSHIBA_FUNCTION_UNIT_MODE			0xB0
#define TOSHIBA_FUNCTION_SETPOINT			0xB3
#define TOSHIBA_FUNCTION_ROOMTEMP			0xBB
#define TOSHIBA_FUNCTION_OUTSIDETEMP		0xBE
#define TOSHIBA_FUNCTION_PURE				0xC7
#define TOSHIBA_FUNCTION_WIFILED1			0xDE
#define TOSHIBA_FUNCTION_WIFILED2			0xDF
#define TOSHIBA_FUNCTION_SPECIAL_MODE		0xF7
#define TOSHIBA_FUNCTION_GROUP1				0xF8

// Status values
#define TOSHIBA_STATUS_READY				0x42

// State values  
#define TOSHIBA_POWER_STATE_ON				0x30
#define TOSHIBA_POWER_STATE_OFF				0x31

// Power selection values
#define TOSHIBA_FUNCTION_POWER_SELECTION_50	0x32 //50%
#define TOSHIBA_FUNCTION_POWER_SELECTION_75	0x4B //75%
#define TOSHIBA_FUNCTION_POWER_SELECTION_100 0x64 //100%

// Mode values
#define TOSHIBA_MODE_AUTO					0x41
#define TOSHIBA_MODE_COOL					0x42
#define TOSHIBA_MODE_HEAT					0x43
#define TOSHIBA_MODE_DRY					0x44
#define TOSHIBA_MODE_FAN_ONLY				0x45

// Special mode values
#define TOSHIBA_SPECIAL_MODE_HIPOWER		0x01
#define TOSHIBA_SPECIAL_MODE_ECO_CFTSLP		0x03
#define TOSHIBA_SPECIAL_MODE_8C				0x04
#define TOSHIBA_SPECIAL_MODE_SILENT1		0x02
#define TOSHIBA_SPECIAL_MODE_SILENT2		0x0A
#define TOSHIBA_SPECIAL_MODE_FRPL1			0x20
#define TOSHIBA_SPECIAL_MODE_FRPL2			0x30

// Fan mode values
#define TOSHIBA_FANMODE_QUIET				0x31
#define TOSHIBA_FANMODE_LVL1				0x32
#define TOSHIBA_FANMODE_LVL2				0x33
#define TOSHIBA_FANMODE_LVL3				0x34
#define TOSHIBA_FANMODE_LVL4				0x35
#define TOSHIBA_FANMODE_LVL5				0x36
#define TOSHIBA_FANMODE_AUTO				0x41

// Swing values
#define TOSHIBA_SWING_FIX					0x31
#define TOSHIBA_SWING_VERTICAL				0x41
#define TOSHIBA_SWING_HORIZONTAL			0x42
#define TOSHIBA_SWING_BOTH					0x43
#define TOSHIBA_SWING_POS1					0x50
#define TOSHIBA_SWING_POS2					0x51
#define TOSHIBA_SWING_POS3					0x52
#define TOSHIBA_SWING_POS4					0x53
#define TOSHIBA_SWING_POS5					0x54

/* -------------------------------------------------------------- */

// Handshake packets
static const u8 HANDSHAKE_SYN_PACKET_1[8]  = {0x02, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x02};
static const u8 HANDSHAKE_SYN_PACKET_2[9]  = {0x02, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x01, 0x02, 0xFE};
static const u8 HANDSHAKE_SYN_PACKET_3[10] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0xFA};
static const u8 HANDSHAKE_SYN_PACKET_4[10] = {0x02, 0x00, 0x01, 0x81, 0x01, 0x00, 0x02, 0x00, 0x00, 0x7B}; //ACK for HVAC request
static const u8 HANDSHAKE_SYN_PACKET_5[10] = {0x02, 0x00, 0x01, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0xFB};
static const u8 HANDSHAKE_SYN_PACKET_6[8]  = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFE};
static const u8 HANDSHAKE_SYN_PACKET_7[10] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0xFB};
static const u8 HANDSHAKE_SYN_PACKET_8[10] = {0x02, 0x00, 0x02, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0xFA};

typedef struct heatpump_pkt_s
{
	u8 mSTX;
	u8 mHeader[2];
	u8 mType;
	u8 mUnknown1;
	u8 mUnknown2;
	u8 mSize; //Size of data field
	u8 mData[TOSHIBA_MAX_PACKET_SIZE];
	u8 mChecksum;
} __attribute__((packed)) heatpump_pkt;


	/* -------------------------------------------------------------- */
	
	static u8 iohub_heatpump_toshiba_crc(const u8 *data, u8 dataLen)
	{
		u32 crc = 0x00;
		
			//Skip first byte (STX)
		for (u8 i = 1; i < dataLen; i++) 
			crc += data[i];

    	return (u8)(-((u8)crc));
	}

	/* -------------------------------------------------------------- */
	
	static u8 iohub_heatpump_toshiba_mode_to_byte(IoHubHeatpumpMode mode)
	{
		switch (mode) {
			case HeatpumpMode_Auto: return TOSHIBA_MODE_AUTO;
			case HeatpumpMode_Cold: return TOSHIBA_MODE_COOL;
			case HeatpumpMode_Heat: return TOSHIBA_MODE_HEAT;
			case HeatpumpMode_Dry: return TOSHIBA_MODE_DRY;
			case HeatpumpMode_Fan: return TOSHIBA_MODE_FAN_ONLY;
			default: return TOSHIBA_MODE_AUTO;
		}
	}

	/* -------------------------------------------------------------- */
	
	static IoHubHeatpumpMode iohub_heatpump_toshiba_byte_to_mode(u8 val)
	{
		switch (val) {
			case TOSHIBA_MODE_AUTO: return HeatpumpMode_Auto;
			case TOSHIBA_MODE_COOL: return HeatpumpMode_Cold;
			case TOSHIBA_MODE_HEAT: return HeatpumpMode_Heat;
			case TOSHIBA_MODE_DRY: return HeatpumpMode_Dry;
			case TOSHIBA_MODE_FAN_ONLY: return HeatpumpMode_Fan;
			default: return HeatpumpMode_Auto;
		}
	}

	/* -------------------------------------------------------------- */
	
	static u8 iohub_heatpump_toshiba_fanspeed_to_byte(IoHubHeatpumpFanSpeed fanSpeed)
	{
		switch (fanSpeed) {
			case HeatpumpFanSpeed_Auto: return TOSHIBA_FANMODE_AUTO;
			case HeatpumpFanSpeed_Low: return TOSHIBA_FANMODE_LVL1;
			case HeatpumpFanSpeed_Med: return TOSHIBA_FANMODE_LVL3;
			case HeatpumpFanSpeed_High: return TOSHIBA_FANMODE_LVL5;
			default: return TOSHIBA_FANMODE_AUTO;
		}
	}

	/* -------------------------------------------------------------- */
	
	static IoHubHeatpumpFanSpeed iohub_heatpump_toshiba_byte_to_fanspeed(u8 val)
	{
		switch (val) {
			case TOSHIBA_FANMODE_AUTO: return HeatpumpFanSpeed_Auto;
			case TOSHIBA_FANMODE_QUIET:
			case TOSHIBA_FANMODE_LVL1: return HeatpumpFanSpeed_Low;
			case TOSHIBA_FANMODE_LVL2:
			case TOSHIBA_FANMODE_LVL3: return HeatpumpFanSpeed_Med;
			case TOSHIBA_FANMODE_LVL4:
			case TOSHIBA_FANMODE_LVL5: return HeatpumpFanSpeed_High;
			default: return HeatpumpFanSpeed_Auto;
		}
	}

	/* -------------------------------------------------------------- */
	
	static u8 iohub_heatpump_toshiba_vane_to_byte(IoHubHeatpumpVaneMode vaneMode)
	{
		switch (vaneMode) {
			case HeatpumpVaneMode_Auto: return TOSHIBA_SWING_FIX;
			case HeatpumpVaneMode_Swing: return TOSHIBA_SWING_VERTICAL;
			case HeatpumpVaneMode_1: return TOSHIBA_SWING_POS1;
			case HeatpumpVaneMode_2: return TOSHIBA_SWING_POS2;
			case HeatpumpVaneMode_3: return TOSHIBA_SWING_POS3;
			case HeatpumpVaneMode_4: return TOSHIBA_SWING_POS4;
			case HeatpumpVaneMode_5: return TOSHIBA_SWING_POS5;
			default: return TOSHIBA_SWING_FIX;
		}
	}

	/* -------------------------------------------------------------- */
	
	static IoHubHeatpumpVaneMode iohub_heatpump_toshiba_byte_to_vane(u8 val)
	{
		switch (val) {
			case TOSHIBA_SWING_FIX: return HeatpumpVaneMode_Auto;
			case TOSHIBA_SWING_VERTICAL: 
			case TOSHIBA_SWING_HORIZONTAL:
			case TOSHIBA_SWING_BOTH: return HeatpumpVaneMode_Swing;
			case TOSHIBA_SWING_POS1: return HeatpumpVaneMode_1;
			case TOSHIBA_SWING_POS2: return HeatpumpVaneMode_2;
			case TOSHIBA_SWING_POS3: return HeatpumpVaneMode_3;
			case TOSHIBA_SWING_POS4: return HeatpumpVaneMode_4;
			case TOSHIBA_SWING_POS5: return HeatpumpVaneMode_5;
			default: return HeatpumpVaneMode_Auto;
		}
	}
	/* -------------------------------------------------------------- */
		
	static ret_code_t iohub_heatpump_toshiba_read_byte(heatpump_toshiba_ctx *ctx, u8 *byte, u16 timeoutMs)
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
			//LOG_DEBUG("Toshiba: ReadByte: %X: %c", *byte, (*byte >= 32 && *byte < 127) ? *byte : '.');
			return SUCCESS;
		}
		
		IOHUB_LOG_ERROR("Toshiba ReadByte Timeout (%d ms)", timeoutMs);
		return E_TIMEOUT;
	}

	/* -------------------------------------------------------------- */
		
	static ret_code_t iohub_heatpump_toshiba_read_pkt(heatpump_toshiba_ctx *ctx, heatpump_pkt *aPkt)
	{
		aPkt->mSTX = 0x00;
		while (aPkt->mSTX != TOSHIBA_PACKET_STX)
		{
			if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mSTX, TOSHIBA_PACKET_READ_TIMEOUT_MS) == E_TIMEOUT)
			{
				//ctx->mfConnected = FALSE; // Mark as disconnected to retry connection
				return E_TIMEOUT;
			}

			if (aPkt->mSTX != TOSHIBA_PACKET_STX) {
				IOHUB_LOG_WARNING("Toshiba: Discarded byte: 0x%02X", aPkt->mSTX);
			}
		}

		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mHeader[0], TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mHeader[1], TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mType, TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mUnknown1, TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mUnknown2, TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mSize, TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;

		for (u8 i = 0; i < aPkt->mSize; i++) {
			if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mData[i], TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
				return E_TIMEOUT;
		}

		if (iohub_heatpump_toshiba_read_byte(ctx, &aPkt->mChecksum, TOSHIBA_PACKET_READ_TIMEOUT_MS) != SUCCESS)
			return E_TIMEOUT;
		
		//Checksum verification
		u8 calculatedCrc = iohub_heatpump_toshiba_crc((u8 *)aPkt, aPkt->mSize + 7);
		if (calculatedCrc != aPkt->mChecksum) 
		{
			IOHUB_LOG_ERROR("Toshiba: Invalid crc recv=0x%02X, calc=0x%02X", aPkt->mChecksum, calculatedCrc);
		}

		LOG_DEBUG("Received packet: type=0x%02X, size=%d", aPkt->mType, aPkt->mSize);
		LOG_BUFFER(aPkt, aPkt->mSize + 8);

		return SUCCESS;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_write_pkt(heatpump_toshiba_ctx *ctx, heatpump_pkt *aPkt, ToshibaPacketType packetType)
	{
		u8 buffer[64]; // Extra space for packet overhead
		int bufferPos = 0;

		// Add header
		buffer[bufferPos++] = TOSHIBA_PACKET_STX;
		buffer[bufferPos++] = 0x00; //Unknown
		buffer[bufferPos++] = 0x03; //Unknown

		// Add packet type
		buffer[bufferPos++] = aPkt->mType;
		
		// Add unknown bytes (pattern from reference)
		buffer[bufferPos++] = 0x00; // Unknown
		buffer[bufferPos++] = 0x00; // Unknown
		
		// Size
		buffer[bufferPos++] = aPkt->mSize + 5;
		
		// 5 bytes hardcoded payload before data
		buffer[bufferPos++] = 0x01;
		buffer[bufferPos++] = 0x30;
		buffer[bufferPos++] = 0x01;
		buffer[bufferPos++] = 0x00;
		buffer[bufferPos++] = packetType; // Query=0x01 OR Command(Set)=0x02 ?
		
		// Add data payload
		if (aPkt->mSize > 0) {
			memcpy(&buffer[bufferPos], aPkt->mData, aPkt->mSize);
			bufferPos += aPkt->mSize;
		}
		
		// Calculate and add checksum
		buffer[bufferPos] = iohub_heatpump_toshiba_crc(buffer, bufferPos);
		bufferPos++;

		LOG_DEBUG("Sending packet: type=0x%02X, size=%d, total=%d", aPkt->mType, aPkt->mSize, bufferPos);
		LOG_BUFFER(buffer, bufferPos);
		
		return iohub_uart_write(ctx->mUartCtx, buffer, bufferPos);
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_connect(heatpump_toshiba_ctx *ctx)
	{
		ret_code_t ret;
		heatpump_pkt packet;

		struct sync_pkt{ const u8* data; size_t size; };
		const struct sync_pkt HANDSHAKE_SYN_PACKETS[] = {
			{HANDSHAKE_SYN_PACKET_1, sizeof(HANDSHAKE_SYN_PACKET_1)},
			{HANDSHAKE_SYN_PACKET_2, sizeof(HANDSHAKE_SYN_PACKET_2)},
			{HANDSHAKE_SYN_PACKET_3, sizeof(HANDSHAKE_SYN_PACKET_3)},
			{HANDSHAKE_SYN_PACKET_4, sizeof(HANDSHAKE_SYN_PACKET_4)},
			{HANDSHAKE_SYN_PACKET_5, sizeof(HANDSHAKE_SYN_PACKET_5)},
			{HANDSHAKE_SYN_PACKET_6, sizeof(HANDSHAKE_SYN_PACKET_6)},
			{HANDSHAKE_SYN_PACKET_7, sizeof(HANDSHAKE_SYN_PACKET_7)},
			{HANDSHAKE_SYN_PACKET_8, sizeof(HANDSHAKE_SYN_PACKET_8)}
		};

		for  (int i = 0; i < 8; i++) 
		{
			LOG_DEBUG("Sending handshake SYN%d packet ...", i + 1);
			LOG_BUFFER(HANDSHAKE_SYN_PACKETS[i].data, HANDSHAKE_SYN_PACKETS[i].size);

			ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKETS[i].data, HANDSHAKE_SYN_PACKETS[i].size);
			if (ret != SUCCESS) {
				IOHUB_LOG_ERROR("Toshiba: Handshake SYN%d write failed: %d", i + 1, ret);
				return ret;
			}
			while(iohub_heatpump_toshiba_read_pkt(ctx, &packet) == SUCCESS);
		}

		ctx->mfConnected = TRUE;
		return SUCCESS;	
	}
	
	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_query(heatpump_toshiba_ctx *ctx, u8 function, heatpump_pkt *result)
	{
		heatpump_pkt queryPacket;

		if (!ctx) {
			return E_INVALID_PARAMETERS;
		}
		
		if (!ctx->mfConnected) 
		{
			ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
			if (ret != SUCCESS)
				return E_INVALID_NOT_CONNECTED;
		}

		queryPacket.mType = TOSHIBA_PACKET_TYPE_COMMAND;
		queryPacket.mSize = 1;
		queryPacket.mData[0] = function;
		
		ret_code_t ret = iohub_heatpump_toshiba_write_pkt(ctx, &queryPacket, ToshibaPacketType_Query);
		if (ret != SUCCESS) 
			return ret;
		
		while(iohub_heatpump_toshiba_read_pkt(ctx, result) == SUCCESS)
		{
			if (result->mType == (TOSHIBA_PACKET_TYPE_COMMAND | TOSHIBA_PACKET_TYPE_REPLY_MASK))
			{ 
				LOG_DEBUG("Query response received for function: '0x%X' (Size=%d)", function, result->mSize);
				return SUCCESS;
			}
		}

		return E_TIMEOUT;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_command(heatpump_toshiba_ctx *ctx, u8 function, u8 value)
	{
		heatpump_pkt packet;
		heatpump_pkt result;

		packet.mType = TOSHIBA_PACKET_TYPE_COMMAND;
		packet.mSize = 2;
		packet.mData[0] = function;
		packet.mData[1] = value;

		ret_code_t ret = iohub_heatpump_toshiba_write_pkt(ctx, &packet, ToshibaPacketType_Command);
		if (ret != SUCCESS) 
			return ret;

		while(iohub_heatpump_toshiba_read_pkt(ctx, &result) == SUCCESS)
		{
			if (result.mType == (TOSHIBA_PACKET_TYPE_COMMAND | TOSHIBA_PACKET_TYPE_REPLY_MASK))
			{ 
				LOG_DEBUG("Command response received for function: '0x%X' (Size=%d)", function, result.mSize);
				return SUCCESS;
			}
		}

		return E_TIMEOUT;
	}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_init(heatpump_toshiba_ctx *ctx, uart_ctx *anUART)
{
	if (!ctx || !anUART) {
		return E_INVALID_PARAMETERS;
	}
	
	memset(ctx, 0x00, sizeof(heatpump_toshiba_ctx));
	
	ctx->mUartCtx = anUART;
	ctx->mfConnected = FALSE;

	LOG_DEBUG("Opening UART for Toshiba Heatpump ...");

	ret_code_t ret = iohub_uart_open(ctx->mUartCtx, 9600, IOHubUartParity_Even, 1);
	if (ret != SUCCESS) {
		LOG_DEBUG("Failed to open UART: %d", ret);
		return ret;
	}

	// Attempt initial connection
	ret = iohub_heatpump_toshiba_connect(ctx);
	if (ret != SUCCESS) {
		LOG_DEBUG("Initial connection failed: %d", ret);
		// Don't fail init if connection fails - can retry later
	}

	return SUCCESS;
}

/* -------------------------------------------------------------- */

void iohub_heatpump_toshiba_uninit(heatpump_toshiba_ctx *ctx)
{
	if (ctx && ctx->mUartCtx) 
	{
		iohub_uart_close(ctx->mUartCtx);
		ctx->mfConnected = FALSE;
	}
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_set_state(heatpump_toshiba_ctx *ctx, const IoHubHeatpumpSettings *aSettings)
{
	if (!ctx || !aSettings) {
		return E_INVALID_PARAMETERS;
	}
	
	if (!ctx->mfConnected) 
	{
		ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
		if (ret != SUCCESS)
			return E_INVALID_NOT_CONNECTED;
	}

	ret_code_t ret = iohub_heatpump_toshiba_command(ctx, TOSHIBA_FUNCTION_POWER_STATE, (aSettings->mAction == HeatpumpAction_ON) ? TOSHIBA_POWER_STATE_ON : TOSHIBA_POWER_STATE_OFF);
	if (ret != SUCCESS) 
		return ret;

	if (aSettings->mAction == HeatpumpAction_ON) 
	{
		ret = iohub_heatpump_toshiba_command(ctx, TOSHIBA_FUNCTION_SETPOINT, aSettings->mTemperature);
		if (ret != SUCCESS) 
			return ret;
		
		ret = iohub_heatpump_toshiba_command(ctx, TOSHIBA_FUNCTION_UNIT_MODE, iohub_heatpump_toshiba_mode_to_byte(aSettings->mMode));
		if (ret != SUCCESS) 
			return ret;

		ret = iohub_heatpump_toshiba_command(ctx, TOSHIBA_FUNCTION_FANMODE, iohub_heatpump_toshiba_fanspeed_to_byte(aSettings->mFanSpeed));
		if (ret != SUCCESS) 
			return ret;

		ret = iohub_heatpump_toshiba_command(ctx, TOSHIBA_FUNCTION_SWING, iohub_heatpump_toshiba_vane_to_byte(aSettings->mVaneMode));
		if (ret != SUCCESS) 
			return ret;
	}
	
	LOG_DEBUG("Settings sent successfully");
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_get_state(heatpump_toshiba_ctx *ctx, IoHubHeatpumpSettings *aSettings)
{
	heatpump_pkt response;

	if (!ctx || !aSettings) {
		return E_INVALID_PARAMETERS;
	}
	
	if (!ctx->mfConnected) 
	{
		ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
		if (ret != SUCCESS)
			return E_INVALID_NOT_CONNECTED;
	}

	ret_code_t ret = iohub_heatpump_toshiba_query(ctx, TOSHIBA_FUNCTION_GROUP1, &response);
	if ((ret == SUCCESS) && (response.mSize >= 12) && (response.mData[7] == TOSHIBA_FUNCTION_GROUP1)) 
	{
		aSettings->mMode = iohub_heatpump_toshiba_byte_to_mode(response.mData[8]);
		aSettings->mTemperature = response.mData[9];
		aSettings->mFanSpeed = iohub_heatpump_toshiba_byte_to_fanspeed(response.mData[10]);
		// response.mData[4] is operation mode, not directly mapped to our structure
	}

	ret = iohub_heatpump_toshiba_query(ctx, TOSHIBA_FUNCTION_POWER_STATE, &response);
	if ((ret == SUCCESS) && (response.mSize >= 9) && (response.mData[7] == TOSHIBA_FUNCTION_POWER_STATE)) 
		aSettings->mAction = (response.mData[8] == TOSHIBA_POWER_STATE_ON) ? HeatpumpAction_ON : HeatpumpAction_OFF;

	ret = iohub_heatpump_toshiba_query(ctx, TOSHIBA_FUNCTION_SWING, &response);
	if ((ret == SUCCESS) && (response.mSize >= 9) && (response.mData[7] == TOSHIBA_FUNCTION_SWING)) 
		aSettings->mVaneMode = iohub_heatpump_toshiba_byte_to_vane(response.mData[1]);

	LOG_DEBUG("State retrieved: mode=%d, temp=%d, fan=%d, action=%d, vane=%d", aSettings->mMode, aSettings->mTemperature, aSettings->mFanSpeed, aSettings->mAction, aSettings->mVaneMode);
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_get_room_temperature(heatpump_toshiba_ctx *ctx, float *aTemperature)
{	
	heatpump_pkt response;

	if (!ctx || !aTemperature) {
		return E_INVALID_PARAMETERS;
	}
	
	if (!ctx->mfConnected) 
	{
		ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
		if (ret != SUCCESS)
			return E_INVALID_NOT_CONNECTED;
	}
	
	ret_code_t ret = iohub_heatpump_toshiba_query(ctx, TOSHIBA_FUNCTION_ROOMTEMP, &response);
	if ((ret == SUCCESS) && (response.mSize >= 9) && (response.mData[7] == TOSHIBA_FUNCTION_ROOMTEMP)) 
	{
		*aTemperature = (float)((s8)(response.mData[1]));
		LOG_DEBUG("Room temperature: %.1f°C", *aTemperature);
	}

	return SUCCESS;
}
