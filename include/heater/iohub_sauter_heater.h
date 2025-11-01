#pragma once

#include "utils/iohub_types.h"
#include "heater/iohub_cc1101.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------- */

typedef struct sauter_heater_s
{
	cc1101_ctx			*mCC1101;
	
}sauter_heater;

/* -------------------------------------------------------------- */

ret_code_t								iohub_sauter_heater_init(sauter_heater *aCtx, cc1101_ctx *aCC1101);
void    								iohub_sauter_heater_uninit(sauter_heater *aCtx);

ret_code_t			 					iohub_sauter_heater_send(sauter_heater *aCtx, BOOL afOn, float aTemperature, float aBoostTimeMinute);
ret_code_t 								iohub_sauter_heater_read(sauter_heater *aCtx, BOOL *afOn, float *aTemperature, float *aBoostTimeMinute);
u8 										iohub_sauter_heater_crc(u8 *aBuffer, u16 aBufferSize);

#ifdef __cplusplus
}
#endif