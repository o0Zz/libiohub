#pragma once

#include "utils/iohub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void digital_async_receiver_interface_ctx;

typedef struct digital_async_receiver_interface_s
{
	u16		Id;
	BOOL 	(*detectPacket)	(digital_async_receiver_interface_ctx *ctx, u16 durationUs);
	void 	(*packetHandled)(digital_async_receiver_interface_ctx *ctx);
}digital_async_receiver_interface;

#ifdef __cplusplus
}
#endif
