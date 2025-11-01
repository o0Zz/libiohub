#pragma once

#include "utils/iohub_digital_async_receiver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_CHACON_DIO_ID				0xCA10

/* -------------------------------------------------------------- */

typedef struct chacon_dio_s
{
    u8					mDigitalPinTx;
	
	u16					mTimings[132 + 1];
	u16					mTimingCount;
	u16					mTimingReadIdx;
}chacon_dio;

/* -------------------------------------------------------------- */

int     								iohub_chacon_dio_init(chacon_dio *aCtx, u8 aGPIOTx);
void    								iohub_chacon_dio_uninit(chacon_dio *aCtx);

void   				 					iohub_chacon_dio_send(chacon_dio *aCtx, BOOL afON, u32 aSenderID, u8 aReceiverID);
BOOL    								iohub_chacon_dio_read(chacon_dio *aCtx, u32 *aSenderID, u8 *aReceiverID, BOOL *afON);

const digital_async_receiver_interface 	*iohub_chacon_dio_get_interface(void);

void 									iohub_chacon_dio_dump_timings(chacon_dio *aCtx);
#ifdef __cplusplus
}
#endif
