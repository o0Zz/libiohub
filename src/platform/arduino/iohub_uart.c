#include <platform/iohub_uart.h>


//IMPORTANT: This implementation don't support parity bit, stop bits and number of data bits (Only works for 8N1)
#ifndef ARDUINO_UART_SOFTWARE
#	define ARDUINO_UART_SOFTWARE	0
#endif

#if ARDUINO_UART_SOFTWARE

/*
	This implementation only support 1 instance of uart_ctx !
	Read https://docs.arduino.cc/learn/built-in-libraries/software-serial#listen for more information
*/

#include <SoftwareSerial.h>

#define LOG_UART
#ifdef LOG_UART
#	define LOG_UART_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#else
#	define LOG_UART_DEBUG(...)				do{}while(0)
#endif

static SoftwareSerial *sSerial = NULL;

ret_code_t iohub_uart_init(uart_ctx *aCtx, u8 aTxPin, u8 aRxPin, u32 aBaudrate, u16 aMode)
{
	memset(aCtx, 0x00, sizeof(aCtx));
	
	sSerial = new SoftwareSerial(aRxPin, aTxPin);
	
	pinMode( aRxPin, INPUT );
	pinMode( aTxPin, OUTPUT );
	
	sSerial->begin( aBaudrate );
	
	if (aMode != UART_8N1)
		log_error("Error: Serial not supporting this mode: %d", aMode);
	else
		LOG_UART_DEBUG("Serial opened (RX=%d TX=%d Baud=%d)", aRxPin, aTxPin, aBaudrate);
}

/* ------------------------------------------------------------- */

u16 iohub_uart_data_available(uart_ctx *aCtx)
{
	return sSerial->available();
}

/* ------------------------------------------------------------- */

u8 iohub_uart_read_byte(uart_ctx *aCtx)
{
	return sSerial->read();
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_read(uart_ctx *aCtx, u8 *aBuffer, u16 *aSize)
{
	u16 theIdx = 0;
	
	while((sSerial->available()) && (theIdx < *aSize))
		aBuffer[theIdx++] = sSerial->read();
	
	*aSize = theIdx;
	return SUCCESS;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_write(uart_ctx *aCtx, u8 *aBuffer, u16 aSize)
{
	for (u16 i=0; i<aSize; i++)
		sSerial->write(aBuffer[i]);
	
	return SUCCESS;
}

/* ------------------------------------------------------------- */

void iohub_uart_close(uart_ctx *aCtx)
{
	delete sSerial;
	sSerial = NULL;
}

#endif