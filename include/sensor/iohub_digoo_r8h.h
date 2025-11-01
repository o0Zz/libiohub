#ifndef _DRV_DIGOO_R8H_H_
#define _DRV_DIGOO_R8H_H_

#include "utils/iohub_types.h"
#include "utils/iohub_digital_async_receiver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_DIGOO_R8H_ID				0xD680

/* -------------------------------------------------------------- */

typedef struct digoo_r8h_s
{	
	u16					mTimings[72 + 1];
	u16					mTimingCount;
	u16					mTimingReadIdx;
}digoo_r8h;

/* -------------------------------------------------------------- */

int     								iohub_digoo_r8h_init(digoo_r8h *aCtx);
void    								iohub_digoo_r8h_uninit(digoo_r8h *aCtx);

BOOL    								iohub_digoo_r8h_read(digoo_r8h *aCtx, u8 *aSensorID, u8 *aChannelID, float *aTemperature, u8 *anHumidity);

const digital_async_receiver_interface 	*iohub_digoo_r8h_get_interface(void);

void 									iohub_digoo_r8h_dump_timings(digoo_r8h *aCtx);

#ifdef __cplusplus
}
#endif

#endif