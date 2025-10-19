#ifndef _DRV_HEATPUMP_MIDEA_H_
#define _DRV_HEATPUMP_MIDEA_H_

#include "drivers/drv_common.h"
#include "components/digital_async_receiver/digital_async_receiver.h"
#include "drivers/drv_heatpump_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_HEATPUMP_MIDEA_ID				0xC511

/* -------------------------------------------------------------- */

typedef struct heatpump_midea_s
{
    u32					mDigitalPinTx;
	
	u16					mTimings[2 + 6*8*2 + 2];
	u16					mTimingCount;
	u16					mTimingReadIdx;
}heatpump_midea;

/* -------------------------------------------------------------- */

int     								drv_heatpump_midea_init(heatpump_midea *aCtx, u32 aGPIOTx);
void    								drv_heatpump_midea_uninit(heatpump_midea *aCtx);

int										drv_heatpump_midea_send(heatpump_midea *aCtx, HeatpumpAction anAction, int aTemperature, HeatpumpFanSpeed aFanSpeed, HeatpumpMode aMode);
BOOL    								drv_heatpump_midea_read(heatpump_midea *aCtx, HeatpumpAction *anAction, int *aTemperature, HeatpumpFanSpeed *aFanSpeed, HeatpumpMode *aMode);

const digital_async_receiver_interface 	*drv_heatpump_midea_get_interface(void);

void 									drv_heatpump_midea_dump_timings(heatpump_midea *aCtx);

#ifdef __cplusplus
}
#endif

#endif