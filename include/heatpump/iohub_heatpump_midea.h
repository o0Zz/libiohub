#pragma once

#include "utils/iohub_types.h"
#include "utils/iohub_digital_async_receiver.h"
#include "heatpump/iohub_heatpump_common.h"

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

int     								iohub_heatpump_midea_init(heatpump_midea *aCtx, u32 aGPIOTx);
void    								iohub_heatpump_midea_uninit(heatpump_midea *aCtx);

int										iohub_heatpump_midea_set_state(heatpump_midea *aCtx, const IoHubHeatpumpSettings *aSettings);
BOOL    								iohub_heatpump_midea_get_state(heatpump_midea *aCtx, IoHubHeatpumpSettings *aSettings);

const digital_async_receiver_interface 	*iohub_heatpump_midea_get_interface(void);

void 									iohub_heatpump_midea_dump_timings(heatpump_midea *aCtx);

#ifdef __cplusplus
}
#endif
