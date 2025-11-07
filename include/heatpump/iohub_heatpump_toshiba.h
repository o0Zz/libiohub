#pragma once

#include "platform/iohub_uart.h"
#include "heatpump/iohub_heatpump_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heatpump_toshiba_ctx_s
{
    uart_ctx						*mUartCtx;
	bool							mfConnected;
} heatpump_toshiba_ctx;

/* -------------------------------------------------------------- */

ret_code_t  		iohub_heatpump_toshiba_init(heatpump_toshiba_ctx *aCtx, uart_ctx *anUART);
void    			iohub_heatpump_toshiba_uninit(heatpump_toshiba_ctx *aCtx);

ret_code_t			iohub_heatpump_toshiba_set_state(heatpump_toshiba_ctx *aCtx, const IoHubHeatpumpSettings *aSettings);
ret_code_t 			iohub_heatpump_toshiba_get_state(heatpump_toshiba_ctx *aCtx, IoHubHeatpumpSettings *aSettings);

ret_code_t 			iohub_heatpump_toshiba_get_room_temperature(heatpump_toshiba_ctx *aCtx, float *aTemperature);

#ifdef __cplusplus
}
#endif