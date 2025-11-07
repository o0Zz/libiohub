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

ret_code_t iohub_uart_init(uart_ctx *aCtx, u8 txPin, u8 rxPin)
{
	memset(aCtx, 0x00, sizeof(aCtx));

	sSerial = new SoftwareSerial(rxPin, txPin);

	pinMode( rxPin, INPUT );
	pinMode( txPin, OUTPUT );
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_open(uart_ctx *ctx, u32 baudrate, IOHubUartParity parity, u8 stopBits)
{
	uint16_t configuration = 0;

	// Data bits
	configuration += 8 * 100; // 8 data bits

	// Parity
	switch(parity) {
		case IOHubUartParity_None:
			configuration += 0 * 10;
			break;
		case IOHubUartParity_Odd:
			configuration += 1 * 10;
			break;
		case IOHubUartParity_Even:
			configuration += 2 * 10;
			break;
	}

	// Stop bits
	configuration += stopBits;

	if (parity != IOHubUartParity_None)
	{
		IOHUB_LOG_ERROR("Error: Serial not supporting this parity: %d", parity);
		return E_INVALID_PARAMETERS;
	}
	
	sSerial->begin(baudrate, configuration);
	LOG_UART_DEBUG("Serial opened on baudrate %d", baudrate);

	return SUCCESS;
}

/* ------------------------------------------------------------- */

u16 iohub_uart_data_available(uart_ctx *aCtx)
{
	return sSerial->available();
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_read_byte(uart_ctx *aCtx, u8 *byte)
{
	if (sSerial->available() == 0)
		return E_READ_ERROR;
		
	*byte = sSerial->read();
	return SUCCESS;
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

ret_code_t iohub_uart_write(uart_ctx *aCtx, const u8 *aBuffer, u16 aSize)
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