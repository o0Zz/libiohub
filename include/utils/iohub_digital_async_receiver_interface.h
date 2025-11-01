#pragma once

#include "utils/iohub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void digital_async_receiver_interface_ctx;

typedef struct digital_async_receiver_interface_s
{
	u16		ID;
	BOOL 	(*detectPacket)	(digital_async_receiver_interface_ctx *aCtx, u16 aDurationUs);
	void 	(*packetHandled)(digital_async_receiver_interface_ctx *aCtx);
}digital_async_receiver_interface;

#ifdef __cplusplus
}
#endif
