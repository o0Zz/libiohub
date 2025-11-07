#pragma once

#include "platform/iohub_uart.h"
#include "heatpump/iohub_heatpump_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	toshiba_state_disconnected = 0,
	toshiba_state_handshake_syn,
	toshiba_state_handshake_ack,
	toshiba_state_connected
} toshiba_connection_state;

typedef struct heatpump_toshiba_ctx_s
{
    uart_ctx						*mUartCtx;
	toshiba_connection_state		mConnectionState;
	u32								mLastReceiveTime;
	BOOL							mConnected;
	BOOL							mHandshakeReceived;
	BOOL							mInitialized;
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