#ifndef _DRV_HEATPUMP_MITSUBISHI_H_
#define _DRV_HEATPUMP_MITSUBISHI_H_

#include "platform/iohub_uart.h"
#include "heatpump/iohub_heatpump_common.h"

/*
CN105 Pin	Signal
	1		12V (Not used)
	2		GND
	3		5V
	4		TX, data from heat pump
	5		RX, data from thermostat
	
IMPORTANT:
	UART is baud 2400 with parity EVEN !
	DO not forget parity or it will not works !
*/

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

#endif