#pragma once

#include "platform/iohub_uart.h"
#include "heatpump/iohub_heatpump_common.h"

/*

IMPORTANT:
	UART is baud 2400 with parity EVEN !
	DO not forget parity or it will not works !
*/

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------- */

typedef struct heatpump_mitsubishi_ctx_s
{
    uart_ctx            *mUartCtx;
	BOOL				mfConnected;
}heatpump_mitsubishi_ctx;

/* -------------------------------------------------------------- */

ret_code_t  		iohub_heatpump_mitsubishi_init(heatpump_mitsubishi_ctx *aCtx, uart_ctx *anUART);
void    			iohub_heatpump_mitsubishi_uninit(heatpump_mitsubishi_ctx *aCtx);

ret_code_t			iohub_heatpump_mitsubishi_send(heatpump_mitsubishi_ctx *aCtx, IoHubHeatpumpAction anAction, int aTemperature, IoHubHeatpumpFanSpeed aFanSpeed, IoHubHeatpumpMode aMode);
ret_code_t 			iohub_heatpump_mitsubishi_read(heatpump_mitsubishi_ctx *aCtx, IoHubHeatpumpAction *anAction, int *aTemperature, IoHubHeatpumpFanSpeed *aFanSpeed, IoHubHeatpumpMode *aMode);

ret_code_t 			iohub_heatpump_mitsubishi_read_room_temperature(heatpump_mitsubishi_ctx *aCtx, float *aTemperature);

#ifdef __cplusplus
}
#endif
