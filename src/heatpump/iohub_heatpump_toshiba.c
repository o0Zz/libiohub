#include "heatpump/iohub_heatpump_toshiba.h"
#include "utils/iohub_types.h"
#include "utils/iohub_logs.h"
#include "utils/iohub_time.h"
#include "utils/iohub_errors.h"
#include "platform/iohub_platform.h"

//https://github.com/burrocargado/toshiba-aircon-mqtt-bridge/tree/master
//https://github.com/ormsport/ToshibaCarrierHvac/blob/main/src/ToshibaCarrierHvac.cpp

#define DEBUG

#ifdef DEBUG
#	define LOG_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_BUFFER(buf, size)		IOHUB_LOG_BUFFER(buf, size)
#else
#	define LOG_DEBUG(...)				do{}while(0)
#	define LOG_BUFFER(buf, size)		do{}while(0)
#endif

/* -------------------------------------------------------------- */

#define TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS		600
#define TOSHIBA_TIME_BETWEEN_PACKET_QUERY_MS		200

// Toshiba protocol constants
#define TOSHIBA_MAX_PACKET_SIZE				17
#define TOSHIBA_HANDSHAKE_TIMEOUT_MS		5000
#define TOSHIBA_COMMAND_TIMEOUT_MS			1000
#define TOSHIBA_PACKET_READ_TIMEOUT_MS		250

// Packet types
#define TOSHIBA_PACKET_TYPE_COMMAND			16
#define TOSHIBA_PACKET_TYPE_FEEDBACK		17
#define TOSHIBA_PACKET_TYPE_SYN_ACK			128
#define TOSHIBA_PACKET_TYPE_ACK				130
#define TOSHIBA_PACKET_TYPE_REPLY			144

// Function codes
#define TOSHIBA_FUNCTION_STATE				128
#define TOSHIBA_FUNCTION_PSEL				135
#define TOSHIBA_FUNCTION_STATUS				136
#define TOSHIBA_FUNCTION_ONTIMER			144
#define TOSHIBA_FUNCTION_OFFTIMER			148
#define TOSHIBA_FUNCTION_FANMODE			160
#define TOSHIBA_FUNCTION_SWING				163
#define TOSHIBA_FUNCTION_MODE				176
#define TOSHIBA_FUNCTION_SETPOINT			179
#define TOSHIBA_FUNCTION_ROOMTEMP			187
#define TOSHIBA_FUNCTION_OUTSIDETEMP		190
#define TOSHIBA_FUNCTION_PURE				199
#define TOSHIBA_FUNCTION_WIFILED1			222
#define TOSHIBA_FUNCTION_WIFILED2			223
#define TOSHIBA_FUNCTION_OP					247
#define TOSHIBA_FUNCTION_GROUP1				248

// Status values
#define TOSHIBA_STATUS_READY				66

// State values  
#define TOSHIBA_STATE_ON					49
#define TOSHIBA_STATE_OFF					48

// Mode values
#define TOSHIBA_MODE_AUTO					65
#define TOSHIBA_MODE_COOL					66
#define TOSHIBA_MODE_HEAT					67
#define TOSHIBA_MODE_DRY					68
#define TOSHIBA_MODE_FAN_ONLY				69

// Fan mode values
#define TOSHIBA_FANMODE_QUIET				49
#define TOSHIBA_FANMODE_LVL1				50
#define TOSHIBA_FANMODE_LVL2				51
#define TOSHIBA_FANMODE_LVL3				52
#define TOSHIBA_FANMODE_LVL4				53
#define TOSHIBA_FANMODE_LVL5				54
#define TOSHIBA_FANMODE_AUTO				65

// Swing values
#define TOSHIBA_SWING_FIX					49
#define TOSHIBA_SWING_VERTICAL				65
#define TOSHIBA_SWING_HORIZONTAL			66
#define TOSHIBA_SWING_BOTH					67
#define TOSHIBA_SWING_POS1					80
#define TOSHIBA_SWING_POS2					81
#define TOSHIBA_SWING_POS3					82
#define TOSHIBA_SWING_POS4					83
#define TOSHIBA_SWING_POS5					84

/* -------------------------------------------------------------- */

// Protocol constants
static const u8 PACKET_HEADER[3] = {2, 0, 3};
static const u8 HANDSHAKE_HEADER[3] = {2, 0, 0};
static const u8 CONFIRM_HEADER[3] = {2, 0, 2};

// Handshake packets
static const u8 HANDSHAKE_SYN_PACKET_1[8]  = {2, 255, 255, 0, 0, 0, 0, 2};
static const u8 HANDSHAKE_SYN_PACKET_2[9]  = {2, 255, 255, 1, 0, 0, 1, 2, 254};
static const u8 HANDSHAKE_SYN_PACKET_3[10] = {2, 0, 0, 0, 0, 0, 2, 2, 2, 250};
static const u8 HANDSHAKE_SYN_PACKET_4[10] = {2, 0, 1, 129, 1, 0, 2, 0, 0, 123};
static const u8 HANDSHAKE_SYN_PACKET_5[10] = {2, 0, 1, 2, 0, 0, 2, 0, 0, 254};
static const u8 HANDSHAKE_SYN_PACKET_6[8]  = {2, 0, 2, 0, 0, 0, 0, 254};

static const u8 HANDSHAKE_ACK_PACKET_1[10] = {2, 0, 2, 1, 0, 0, 2, 0, 0, 251};
static const u8 HANDSHAKE_ACK_PACKET_2[10] = {2, 0, 2, 2, 0, 0, 2, 0, 0, 250};

typedef struct heatpump_pkt_s
{
	u8 mHeader[3];
	u8 mType;
	u8 mUnknown1;
	u8 mUnknown2;
	u8 mSize;
	u8 mData[TOSHIBA_MAX_PACKET_SIZE];
	u8 mChecksum;
}heatpump_pkt;


	/* -------------------------------------------------------------- */
	
	static u8 iohub_heatpump_toshiba_crc(u16 baseKey, const u8 *data, u8 dataLen)
	{
		s16 result = baseKey - (dataLen * 2);
		
		for (s8 i = 0; i < dataLen; i++) 
			result -= data[i];
		
		return (u8)result; 
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
		
	static ret_code_t iohub_heatpump_toshiba_read_byte(heatpump_toshiba_ctx *ctx, u8 *byte, u16 aTimeoutMs)
	{
		u32 startTime = iohub_time_now_ms();
		
		while ((iohub_time_now_ms() - startTime) < aTimeoutMs) 
		{
			if (iohub_uart_read_byte(ctx->mUartCtx, byte) == SUCCESS)
				return SUCCESS;
			
			iohub_time_delay_ms(1);
		}
		
		return E_TIMEOUT;
	}

	/* -------------------------------------------------------------- */
		
	static ret_code_t iohub_heatpump_toshiba_read_pkt(heatpump_toshiba_ctx *ctx, heatpump_pkt *aPkt)
	{
		u8 buffer[TOSHIBA_MAX_PACKET_SIZE + 8]; // Extra space for header and metadata
		int bytesRead = 0;
		u32 startTime = iohub_time_now_ms();
		
		// Read with timeout
		while (bytesRead < sizeof(buffer) && (iohub_time_now_ms() - startTime) < TOSHIBA_PACKET_READ_TIMEOUT_MS) 
		{
			ret_code_t ret = iohub_heatpump_toshiba_read_byte(ctx, &buffer[bytesRead], 10);
			if (ret == SUCCESS) 
			{
				bytesRead++;
				startTime = iohub_time_now_ms(); // Reset timeout on successful read
			} else {
				iohub_time_delay_ms(1);
			}
		}

		if (bytesRead == 0) {
			LOG_DEBUG("No data received within timeout");
			ctx->mConnected = FALSE; // Mark as disconnected - Sound like toshiba disconnect
			return E_TIMEOUT;
		}
		
		if (bytesRead < 8) { // Minimum packet size
			LOG_DEBUG("Packet too short: %d bytes", bytesRead);
			return E_INVALID_DATA;
		}
		
		// Parse packet
		if (memcmp(buffer, PACKET_HEADER, 3) == 0 
			||  memcmp(buffer, HANDSHAKE_HEADER, 3) == 0 
			|| memcmp(buffer, CONFIRM_HEADER, 3) == 0) 
		{
			memcpy(aPkt->mHeader, buffer, 3);
			aPkt->mType = buffer[3];
			aPkt->mSize = buffer[6];
			
			// Copy data payload
			int dataStart = (aPkt->mType == TOSHIBA_PACKET_TYPE_REPLY) ? 14 : 12;
			int dataLength = (bytesRead > dataStart) ? (bytesRead - dataStart - 1) : 0; // -1 for checksum
			
			if (dataLength > 0 && dataLength <= TOSHIBA_MAX_PACKET_SIZE) {
				memcpy(aPkt->mData, &buffer[dataStart], dataLength);
			}
			
			if (bytesRead > dataStart + dataLength) {
				aPkt->mChecksum = buffer[bytesRead - 1];
			}
			
			LOG_DEBUG("Received packet: type=0x%02X, size=%d, dataLen=%d", aPkt->mType, aPkt->mSize, dataLength);
			LOG_BUFFER(buffer, bytesRead);
			
			ctx->mLastReceiveTime = iohub_time_now_ms();
			return SUCCESS;
		}
		
		LOG_DEBUG("Invalid packet header");
		return E_INVALID_DATA;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_read_pkt_until(heatpump_toshiba_ctx *ctx, heatpump_pkt *aPkt, const u8 *expectedFunction, u8 size_min)
	{
		u32 startTime = iohub_time_now_ms();
		while ((iohub_time_now_ms() - startTime) < TOSHIBA_COMMAND_TIMEOUT_MS) 
		{
			ret_code_t ret = iohub_heatpump_toshiba_read_pkt(ctx, aPkt);
			if (ret == SUCCESS) 
			{
				if (aPkt->mSize >= size_min)
				{
					u8 i = 0;
					while(expectedFunction[i] != 0) 
					{
						if (aPkt->mData[0] == expectedFunction[i])
							return SUCCESS;
					}
				}
			}
		}
		
		return E_TIMEOUT;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_write_pkt(heatpump_toshiba_ctx *ctx, heatpump_pkt *aPkt, u16 aTimeBetweenPacketMs)
	{
		u8 buffer[TOSHIBA_MAX_PACKET_SIZE + 16]; // Extra space for packet overhead
		int bufferPos = 0;
		
		// Add header
		memcpy(&buffer[bufferPos], PACKET_HEADER, 3);
		bufferPos += 3;
		
		// Add packet type
		buffer[bufferPos++] = aPkt->mType;
		
		// Add unknown bytes (pattern from reference)
		buffer[bufferPos++] = 0; // Unknown1
		buffer[bufferPos++] = 0; // Unknown2
		
		// Calculate and add size
		int dataLen = aPkt->mSize;
		buffer[bufferPos] = (aPkt->mType == TOSHIBA_PACKET_TYPE_REPLY) ? (14 + dataLen + 1 - 8) : (12 + dataLen + 1 - 8);
		bufferPos++;
		
		// Add pattern bytes from reference implementation
		buffer[bufferPos++] = 1;
		buffer[bufferPos++] = 48;
		buffer[bufferPos++] = 1;
		
		if (aPkt->mType == TOSHIBA_PACKET_TYPE_REPLY) {
			buffer[bufferPos++] = 0; // Extra byte for REPLY packets
		}
		
		// Add data length
		buffer[bufferPos++] = dataLen;
		
		// Add data payload
		if (dataLen > 0) {
			memcpy(&buffer[bufferPos], aPkt->mData, dataLen);
			bufferPos += dataLen;
		}
		
		// Calculate and add checksum
		u16 checksumBase = (aPkt->mType == TOSHIBA_PACKET_TYPE_REPLY) ? 308 : 438;
		buffer[bufferPos++] = iohub_heatpump_toshiba_crc(checksumBase, aPkt->mData, dataLen);
		
		LOG_DEBUG("Sending packet: type=0x%02X, size=%d, total=%d", aPkt->mType, dataLen, bufferPos);
		LOG_BUFFER(buffer, bufferPos);
		
		ret_code_t ret = iohub_uart_write(ctx->mUartCtx, buffer, bufferPos);
		
		iohub_time_delay_ms(aTimeBetweenPacketMs);
		
		return ret;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_send_handshake_syn(heatpump_toshiba_ctx *ctx)
	{
		ret_code_t ret;
		
		// Send handshake SYN packets in sequence
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKET_1, sizeof(HANDSHAKE_SYN_PACKET_1));
		if (ret != SUCCESS) return ret;
		iohub_time_delay_ms(200);
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKET_2, sizeof(HANDSHAKE_SYN_PACKET_2));
		if (ret != SUCCESS) return ret;
		iohub_time_delay_ms(200);
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKET_3, sizeof(HANDSHAKE_SYN_PACKET_3));
		if (ret != SUCCESS) return ret;
		iohub_time_delay_ms(200);
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKET_4, sizeof(HANDSHAKE_SYN_PACKET_4));
		if (ret != SUCCESS) return ret;
		iohub_time_delay_ms(200);
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKET_5, sizeof(HANDSHAKE_SYN_PACKET_5));
		if (ret != SUCCESS) return ret;
		iohub_time_delay_ms(200);
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_SYN_PACKET_6, sizeof(HANDSHAKE_SYN_PACKET_6));
		if (ret != SUCCESS) return ret;
		
		ctx->mConnectionState = toshiba_state_handshake_syn;
		
		LOG_DEBUG("Handshake SYN packets sent");
		return SUCCESS;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_send_handshake_ack(heatpump_toshiba_ctx *ctx)
	{
		ret_code_t ret;
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_ACK_PACKET_1, sizeof(HANDSHAKE_ACK_PACKET_1));
		if (ret != SUCCESS) return ret;
		iohub_time_delay_ms(200);
		
		ret = iohub_uart_write(ctx->mUartCtx, HANDSHAKE_ACK_PACKET_2, sizeof(HANDSHAKE_ACK_PACKET_2));
		if (ret != SUCCESS) 
			return ret;

		ctx->mConnectionState = toshiba_state_handshake_ack;
		
		LOG_DEBUG("Handshake ACK packets sent");
		return SUCCESS;
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_process_packet(heatpump_toshiba_ctx *ctx, heatpump_pkt *aPkt)
	{
		switch (aPkt->mType) {
			case TOSHIBA_PACKET_TYPE_SYN_ACK:
				LOG_DEBUG("Received SYN/ACK");
				ctx->mHandshakeReceived = TRUE;
				return iohub_heatpump_toshiba_send_handshake_ack(ctx);
				
			case TOSHIBA_PACKET_TYPE_ACK:
				LOG_DEBUG("Received ACK - connection established");
				ctx->mConnected = TRUE;
				ctx->mConnectionState = toshiba_state_connected;
				return SUCCESS;
				
			case TOSHIBA_PACKET_TYPE_FEEDBACK:
			case TOSHIBA_PACKET_TYPE_REPLY:
				// Check for READY status
				if (aPkt->mSize >= 2 && aPkt->mData[0] == TOSHIBA_FUNCTION_STATUS && aPkt->mData[1] == TOSHIBA_STATUS_READY) {
					LOG_DEBUG("Device is READY");
					ctx->mConnected = TRUE;
					ctx->mConnectionState = toshiba_state_connected;
				}
				return SUCCESS;
				
			default:
				LOG_DEBUG("Unknown packet type: 0x%02X", aPkt->mType);
				return E_INVALID_DATA;
		}
	}

	/* -------------------------------------------------------------- */

	static ret_code_t iohub_heatpump_toshiba_connect(heatpump_toshiba_ctx *ctx)
	{
		heatpump_pkt packet;
		
		if (!ctx->mInitialized || ctx->mConnectionState == toshiba_state_disconnected) 
		{
			ret_code_t ret = iohub_heatpump_toshiba_send_handshake_syn(ctx);
			if (ret != SUCCESS) 
				return ret;
		}
		
		// Wait for handshake response
		u32 startTime = iohub_time_now_ms();
		while ((iohub_time_now_ms() - startTime) < TOSHIBA_HANDSHAKE_TIMEOUT_MS) 
		{
			ret_code_t ret = iohub_heatpump_toshiba_read_pkt(ctx, &packet);
			if (ret == SUCCESS) 
			{
				ret = iohub_heatpump_toshiba_process_packet(ctx, &packet);
				if (ret != SUCCESS) 
					return ret;
				
				if (ctx->mConnected) {
					LOG_DEBUG("Connection established successfully");
					return SUCCESS;
				}
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
	ctx->mConnectionState = toshiba_state_disconnected;
	ctx->mConnected = FALSE;
	ctx->mHandshakeReceived = FALSE;
	ctx->mInitialized = FALSE;

	ret_code_t ret = iohub_uart_open(ctx->mUartCtx, 9600, IOHubUartParity_Even, 1);
	if (ret != SUCCESS) {
		LOG_DEBUG("Failed to open UART: %d", ret);
		return ret;
	}

	ctx->mInitialized = TRUE;
	
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
		ctx->mInitialized = FALSE;
		ctx->mConnected = FALSE;
		ctx->mConnectionState = toshiba_state_disconnected;
	}
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_set_state(heatpump_toshiba_ctx *ctx, const IoHubHeatpumpSettings *aSettings)
{
	if (!ctx || !aSettings) {
		return E_INVALID_PARAMETERS;
	}
	
	if (!ctx->mConnected) {
		// Try to reconnect
		ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
		if (ret != SUCCESS) {
			return E_INVALID_NOT_CONNECTED;
		}
	}
	
	heatpump_pkt packet;
	packet.mType = TOSHIBA_PACKET_TYPE_COMMAND;
	
	// Send power state
	packet.mSize = 2;
	packet.mData[0] = TOSHIBA_FUNCTION_STATE;
	packet.mData[1] = (aSettings->mAction == HeatpumpAction_ON) ? TOSHIBA_STATE_ON : TOSHIBA_STATE_OFF;
	
	ret_code_t ret = iohub_heatpump_toshiba_write_pkt(ctx, &packet, TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS);
	if (ret != SUCCESS) 
		return ret;

	// Send temperature setpoint if unit is ON
	if (aSettings->mAction == HeatpumpAction_ON && aSettings->mTemperature >= 17 && aSettings->mTemperature <= 30) {
		packet.mSize = 2;
		packet.mData[0] = TOSHIBA_FUNCTION_SETPOINT;
		packet.mData[1] = aSettings->mTemperature;
		
		ret = iohub_heatpump_toshiba_write_pkt(ctx, &packet, TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS);
		if (ret != SUCCESS) 
			return ret;
		
		// Send mode
		packet.mSize = 2;
		packet.mData[0] = TOSHIBA_FUNCTION_MODE;
		packet.mData[1] = iohub_heatpump_toshiba_mode_to_byte(aSettings->mMode);
		
		ret = iohub_heatpump_toshiba_write_pkt(ctx, &packet, TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS);
		if (ret != SUCCESS) 
			return ret;
		
		// Send fan speed
		packet.mSize = 2;
		packet.mData[0] = TOSHIBA_FUNCTION_FANMODE;
		packet.mData[1] = iohub_heatpump_toshiba_fanspeed_to_byte(aSettings->mFanSpeed);
		
		ret = iohub_heatpump_toshiba_write_pkt(ctx, &packet, TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS);
		if (ret != SUCCESS) 
			return ret;

		// Send vane mode (swing)
		packet.mSize = 2;
		packet.mData[0] = TOSHIBA_FUNCTION_SWING;
		packet.mData[1] = iohub_heatpump_toshiba_vane_to_byte(aSettings->mVaneMode);
		
		ret = iohub_heatpump_toshiba_write_pkt(ctx, &packet, TOSHIBA_TIME_BETWEEN_PACKET_COMMAND_MS);
		if (ret != SUCCESS) 
			return ret;
	}
	
	LOG_DEBUG("Settings sent successfully");
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_get_state(heatpump_toshiba_ctx *ctx, IoHubHeatpumpSettings *aSettings)
{
	heatpump_pkt queryPacket;
	heatpump_pkt response;

	if (!ctx || !aSettings) {
		return E_INVALID_PARAMETERS;
	}
	
	if (!ctx->mConnected) {
		// Try to reconnect
		ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
		if (ret != SUCCESS) {
			return E_INVALID_NOT_CONNECTED;
		}
	}

	queryPacket.mType = TOSHIBA_PACKET_TYPE_COMMAND;
	queryPacket.mSize = 1;
	
	// Query group 1 data (mode, setpoint, fanmode, operation) - function code 248
	queryPacket.mData[0] = TOSHIBA_FUNCTION_GROUP1;
	
	ret_code_t ret = iohub_heatpump_toshiba_write_pkt(ctx, &queryPacket, TOSHIBA_TIME_BETWEEN_PACKET_QUERY_MS);
	if (ret != SUCCESS) 
		return ret;
	
	// Wait for response
	u32 startTime = iohub_time_now_ms();
	while ((iohub_time_now_ms() - startTime) < 1000)  // 1 second timeout
	{
		ret = iohub_heatpump_toshiba_read_pkt(ctx, &response);
		if (ret == SUCCESS) 
		{
			// Process group 1 response (5 bytes: function, mode, setpoint, fanmode, operation)
			if (response.mSize >= 5 && response.mData[0] == TOSHIBA_FUNCTION_GROUP1) 
			{
				aSettings->mMode = iohub_heatpump_toshiba_byte_to_mode(response.mData[1]);
				aSettings->mTemperature = response.mData[2];
				aSettings->mFanSpeed = iohub_heatpump_toshiba_byte_to_fanspeed(response.mData[3]);
				// response.mData[4] is operation mode, not directly mapped to our structure
				
				aSettings->mAction = HeatpumpAction_ON; // Assume ON if we got valid data
				aSettings->mVaneMode = HeatpumpVaneMode_Auto; // Default since not in group 1
				
				LOG_DEBUG("State retrieved: mode=%d, temp=%d, fan=%d", aSettings->mMode, aSettings->mTemperature, aSettings->mFanSpeed);
				return SUCCESS;
			}
			
			// Process individual function responses
			if (response.mSize >= 2) {
				switch (response.mData[0]) {
					case TOSHIBA_FUNCTION_STATE:
						aSettings->mAction = (response.mData[1] == TOSHIBA_STATE_ON) ? HeatpumpAction_ON : HeatpumpAction_OFF;
						break;
					case TOSHIBA_FUNCTION_MODE:
						aSettings->mMode = iohub_heatpump_toshiba_byte_to_mode(response.mData[1]);
						break;
					case TOSHIBA_FUNCTION_SETPOINT:
						aSettings->mTemperature = response.mData[1];
						break;
					case TOSHIBA_FUNCTION_FANMODE:
						aSettings->mFanSpeed = iohub_heatpump_toshiba_byte_to_fanspeed(response.mData[1]);
						break;
					case TOSHIBA_FUNCTION_SWING:
						aSettings->mVaneMode = iohub_heatpump_toshiba_byte_to_vane(response.mData[1]);
						break;
				}
			}
		}
	}

	queryPacket.mData[0] = TOSHIBA_FUNCTION_STATE;
	iohub_heatpump_toshiba_write_pkt(ctx, &queryPacket, TOSHIBA_TIME_BETWEEN_PACKET_QUERY_MS);
	u8 expected_state[] = {TOSHIBA_FUNCTION_STATE, 0};
	if ( iohub_heatpump_toshiba_read_pkt_until(ctx, &response, expected_state, 2) == SUCCESS) 
		aSettings->mAction = (response.mData[1] == TOSHIBA_STATE_ON) ? HeatpumpAction_ON : HeatpumpAction_OFF;
	

	queryPacket.mData[0] = TOSHIBA_FUNCTION_SWING;
	iohub_heatpump_toshiba_write_pkt(ctx, &queryPacket, TOSHIBA_TIME_BETWEEN_PACKET_QUERY_MS);
	u8 expected_vane[] = {TOSHIBA_FUNCTION_SWING, 0};
	if ( iohub_heatpump_toshiba_read_pkt_until(ctx, &response, expected_vane, 2) == SUCCESS) 
		aSettings->mVaneMode = iohub_heatpump_toshiba_byte_to_vane(response.mData[1]);
		
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t iohub_heatpump_toshiba_get_room_temperature(heatpump_toshiba_ctx *ctx, float *aTemperature)
{
	if (!ctx || !aTemperature) {
		return E_INVALID_PARAMETERS;
	}
	
	if (!ctx->mConnected) {
		// Try to reconnect
		ret_code_t ret = iohub_heatpump_toshiba_connect(ctx);
		if (ret != SUCCESS) {
			return E_INVALID_NOT_CONNECTED;
		}
	}
	
	heatpump_pkt queryPacket;
	heatpump_pkt response;

	queryPacket.mType = TOSHIBA_PACKET_TYPE_COMMAND;
	queryPacket.mSize = 1;
	queryPacket.mData[0] = TOSHIBA_FUNCTION_ROOMTEMP;
	
	ret_code_t ret = iohub_heatpump_toshiba_write_pkt(ctx, &queryPacket, TOSHIBA_TIME_BETWEEN_PACKET_QUERY_MS);
	if (ret != SUCCESS) 
		return ret;
	
	// Wait for response
	u8 expected[] = {TOSHIBA_FUNCTION_ROOMTEMP, 0};
	if ( iohub_heatpump_toshiba_read_pkt_until(ctx, &response, expected, 2) == SUCCESS) 
	{
		*aTemperature = (float)((s8)(response.mData[1]));
		LOG_DEBUG("Room temperature: %.1f°C", *aTemperature);
		return SUCCESS;
	}

	return E_TIMEOUT;
}
