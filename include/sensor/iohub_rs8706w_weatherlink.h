#ifndef _DRV_RS8706W_WEATHERLINK_H_
#define _DRV_RS8706W_WEATHERLINK_H_

#include "drivers/drv_common.h"
#include "components/digital_async_receiver/digital_async_receiver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_RS8706W_ID				0x8706

typedef struct rs8706w_weatherlink_data_s
{
    float           mTemperatureCelsius;
    u8              mRainfallMillimeters;
    u8              mStationID;
}rs8706w_weatherlink_data;

/* -------------------------------------------------------------- */

typedef struct rs8706w_weatherlink_s
{
	u16					mTimings[(37*2) + 1];
   	u16					mTimingCount;
	u16					mTimingReadIdx;
}rs8706w_weatherlink;

/* -------------------------------------------------------------- */

int 									drv_rs8706w_weatherlink_init(rs8706w_weatherlink *aCtx);

void 									drv_rs8706w_weatherlink_uninit(rs8706w_weatherlink *aCtx);

BOOL 									drv_rs8706w_weatherlink_read(rs8706w_weatherlink *aCtx, rs8706w_weatherlink_data *anOutputData);

const digital_async_receiver_interface 	*drv_rs8706w_weatherlink_get_interface(void);

void 									drv_rs8706w_weatherlink_dump_timings(rs8706w_weatherlink *aCtx);

#ifdef __cplusplus
}
#endif

#endif