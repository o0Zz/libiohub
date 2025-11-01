#include "heater/iohub_sauter_heater.h"

//#define DEBUG_SAUTER
#ifdef DEBUG_SAUTER
#	define LOG_DEBUG_SAUTER(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#	define LOG_ERROR_SAUTER(...)				IOHUB_LOG_ERROR(__VA_ARGS__)
#else
#	define LOG_DEBUG_SAUTER(...)				do{}while(0)
#	define LOG_ERROR_SAUTER(...)				do{}while(0)
#endif

ret_code_t iohub_sauter_heater_init(sauter_heater *aCtx, cc1101_ctx *aCC1101)
{
    memset(aCtx, 0x00, sizeof(sauter_heater));
	
	aCtx->mCC1101 = aCC1101;
	
	return SUCCESS;
}

/* -------------------------------------------------------------------- */

void iohub_sauter_heater_uninit(sauter_heater *aCtx)
{
}

/* -------------------------------------------------------------------- */
#define SAUTER_HEATER_OFFSET_MODE			5
#define SAUTER_HEATER_OFFSET_TEMP			6
#define SAUTER_HEATER_OFFSET_CRC			22


ret_code_t iohub_sauter_heater_send(sauter_heater *aCtx, BOOL afOn, float aTemperature, float aBoostTimeMinute)
{
	u8 buf[32] = {0x20/*STX*/, 0x5A,0xB5,0x07,0x52,0x00/*Mode*/,0x00/*Temp*/,0x00,0x54,0x00,0x00,0x00,0x62,0x00,0x56/*??*/,0x02/*??*/,0x70,0x00,0x00,0x00,0x08,0x0B,0x00/*CRC*/,0x77/*ETX*/};

	float theDiff = (aTemperature - 12.0);
	//u8 theTempIdx = (u8)(theDiff * 2.0);
	
	buf[SAUTER_HEATER_OFFSET_CRC] = iohub_sauter_heater_crc(buf, sizeof(buf) - 2); //-2 = Footer + CRC

	return SUCCESS;
}

/* -------------------------------------------------------------------- */

ret_code_t iohub_sauter_heater_read(sauter_heater *aCtx, BOOL *afOn, float *aTemperature, float *aBoostTimeMinute)
{
	
}

/* -------------------------------------------------------------------- */

u8 iohub_sauter_heater_crc(u8 *aBuffer, u16 aBufferSize)
{
	u8 crc = 0x00;
	
	for (u16 i=0; i<aBufferSize; i++)
		crc ^= aBuffer[i];
	
	return crc;
}