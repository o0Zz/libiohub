#pragma once

#include "platform/iohub_uart.h"
#include "heatpump/iohub_heatpump_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heatpump_toshiba_s
{
    uart_ctx						*mUartCtx;
	BOOL							mfConnected;
} heatpump_toshiba;

/* -------------------------------------------------------------- */

ret_code_t  		iohub_heatpump_toshiba_init(heatpump_toshiba *aCtx, uart_ctx *anUART);
void    			iohub_heatpump_toshiba_uninit(heatpump_toshiba *aCtx);

ret_code_t			iohub_heatpump_toshiba_set_state(heatpump_toshiba *aCtx, const IoHubHeatpumpSettings *aSettings);
ret_code_t 			iohub_heatpump_toshiba_get_state(heatpump_toshiba *aCtx, IoHubHeatpumpSettings *aSettings);

ret_code_t 			iohub_heatpump_toshiba_get_room_temperature(heatpump_toshiba *aCtx, float *aTemperature);

#ifdef __cplusplus
}
#endif