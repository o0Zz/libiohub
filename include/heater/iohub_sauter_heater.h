#ifndef _DRV_SAUTER_H_
#define _DRV_SAUTER_H_

#include "drivers/drv_common.h"
#include "drivers/drv_cc1101.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------- */

typedef struct sauter_heater_s
{
	cc1101_ctx			*mCC1101;
	
}sauter_heater;

/* -------------------------------------------------------------- */

ret_code_t								drv_sauter_heater_init(sauter_heater *aCtx, cc1101_ctx *aCC1101);
void    								drv_sauter_heater_uninit(sauter_heater *aCtx);

ret_code_t			 					drv_sauter_heater_send(sauter_heater *aCtx, BOOL afOn, float aTemperature, float aBoostTimeMinute);
ret_code_t 								drv_sauter_heater_read(sauter_heater *aCtx, BOOL *afOn, float *aTemperature, float *aBoostTimeMinute);
u8 										drv_sauter_heater_crc(u8 *aBuffer, u16 aBufferSize);

#ifdef __cplusplus
}
#endif

#endif