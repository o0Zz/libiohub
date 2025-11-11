#pragma once

#include "platform/iohub_uart.h"
#include "heatpump/iohub_heatpump_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------- */

typedef struct heatpump_mitsubishi_s
{
    uart_ctx            *mUartCtx;
	BOOL				mfConnected;
}heatpump_mitsubishi;

/* -------------------------------------------------------------- */

ret_code_t  		iohub_heatpump_mitsubishi_init(heatpump_mitsubishi *aCtx, uart_ctx *anUART);
void    			iohub_heatpump_mitsubishi_uninit(heatpump_mitsubishi *aCtx);

ret_code_t			iohub_heatpump_mitsubishi_set_state(heatpump_mitsubishi *aCtx, const IoHubHeatpumpSettings *aSettings);
ret_code_t 			iohub_heatpump_mitsubishi_get_state(heatpump_mitsubishi *aCtx, IoHubHeatpumpSettings *aSettings);

ret_code_t 			iohub_heatpump_mitsubishi_get_room_temperature(heatpump_mitsubishi *aCtx, float *aTemperature);

#ifdef __cplusplus
}
#endif
