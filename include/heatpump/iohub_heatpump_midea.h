#pragma once

#include "utils/iohub_types.h"
#include "utils/iohub_digital_async_receiver.h"
#include "heatpump/iohub_heatpump_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECEIVER_HEATPUMP_MIDEA_ID				0x1DEA

/* -------------------------------------------------------------- */

typedef struct heatpump_midea_s
{
    u32					mDigitalPinTx;
	
	u16					mTimings[2 + 6*8*2 + 2];
	u16					mTimingCount;
	u16					mTimingReadIdx;
}heatpump_midea;

/* -------------------------------------------------------------- */

int     								iohub_heatpump_midea_init(heatpump_midea *ctx, digital_async_receiver *receiver, u32 txPin);
void    								iohub_heatpump_midea_uninit(heatpump_midea *ctx);

int										iohub_heatpump_midea_set_state(heatpump_midea *ctx, const IoHubHeatpumpSettings *settings);
BOOL    								iohub_heatpump_midea_get_state(heatpump_midea *ctx, IoHubHeatpumpSettings *settings);

const digital_async_receiver_interface 	*iohub_heatpump_midea_get_interface(void);

void 									iohub_heatpump_midea_dump_timings(heatpump_midea *ctx);

#ifdef __cplusplus
}
#endif
