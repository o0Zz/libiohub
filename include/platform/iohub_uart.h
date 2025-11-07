#pragma once

#include "utils/iohub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	IOHubUartParity_None = 0,
	IOHubUartParity_Even,
	IOHubUartParity_Odd
}IOHubUartParity;

#define UART_NOT_USED_PIN		0xFF

/* -------------------------------------------------------------- */

typedef struct uart_ctx_s
{
	void *mCtx;
}uart_ctx;

/* -------------------------------------------------------------- */

ret_code_t 			iohub_uart_init(uart_ctx *ctx, u8 txPin, u8 rxPin);

ret_code_t			iohub_uart_open(uart_ctx *ctx, u32 baudrate, IOHubUartParity parity, u8 stopBits);

u16	    			iohub_uart_data_available(uart_ctx *ctx);

ret_code_t    		iohub_uart_read_byte(uart_ctx *ctx, u8 *byte);

ret_code_t    		iohub_uart_read(uart_ctx *ctx, u8 *buffer, u16 *size);

ret_code_t    		iohub_uart_write(uart_ctx *ctx, const u8 *buffer, u16 size);

void    			iohub_uart_close(uart_ctx *ctx);

#ifdef __cplusplus
}
#endif