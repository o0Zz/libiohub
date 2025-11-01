#pragma once

#include "utils/iohub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_5N1 	501
#define UART_6N1 	601
#define UART_7N1 	701
#define UART_8N1 	801

#define UART_5E1	521
#define UART_6E1    621
#define UART_7E1    721
#define UART_8E1    821

#define UART_5O1 	511
#define UART_6O1 	611
#define UART_7O1 	711
#define UART_8O1 	811

#define UART_NOT_USED_PIN		0xFF

/* -------------------------------------------------------------- */

typedef struct uart_ctx_s
{
	u8 reserved;
}uart_ctx;

/* -------------------------------------------------------------- */

ret_code_t 			iohub_uart_init(uart_ctx *aCtx, u8 aTxPin, u8 aRxPin, u32 aBaudrate, u16 aMode);

u16	    			iohub_uart_data_available(uart_ctx *aCtx);

u8    				iohub_uart_read_byte(uart_ctx *aCtx);

ret_code_t    		iohub_uart_read(uart_ctx *aCtx, u8 *aBuffer, u16 *aSize);

ret_code_t    		iohub_uart_write(uart_ctx *aCtx, u8 *aBuffer, u16 aSize);

void    			iohub_uart_close(uart_ctx *aCtx);

#ifdef __cplusplus
}
#endif