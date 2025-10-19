#ifndef _DIGITAL_ASYNC_RECEIVER_H_
#define _DIGITAL_ASYNC_RECEIVER_H_

#include "components/digital_async_receiver/digital_async_receiver_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DIGITAL_ASYNC_RECEIVER_INTERFACE_MAX
#	define DIGITAL_ASYNC_RECEIVER_INTERFACE_MAX			2
#endif

/* -------------------------------------------------------------- */

typedef struct digital_async_receiver_s
{	
	u8											mPin;
	u32 										mLastTimeUs;
	const digital_async_receiver_interface		*mInterfaceList[DIGITAL_ASYNC_RECEIVER_INTERFACE_MAX];
	digital_async_receiver_interface_ctx		*mInterfaceCtx[DIGITAL_ASYNC_RECEIVER_INTERFACE_MAX];
	u8											mPluginCount;
	volatile u16								mPacketReady; //Avoid compiler optimization because it's used from ISR and main
}digital_async_receiver;

/* -------------------------------------------------------------- */

	///\note on Arduino aPin must be 2 or 3
void    	digital_async_receiver_init(digital_async_receiver *aCtx, u8 aPin);
void    	digital_async_receiver_uninit(digital_async_receiver *aCtx);
	
void    	digital_async_receiver_start(digital_async_receiver *aCtx);
void    	digital_async_receiver_stop(digital_async_receiver *aCtx);

u16     	digital_async_receiver_wait_for_packet(digital_async_receiver *aCtx);
BOOL    	digital_async_receiver_has_packet_available(digital_async_receiver *aCtx, u16 *aPluginID);
void    	digital_async_receiver_packet_handled(digital_async_receiver *aCtx, u16 aPluginID);

void    	digital_async_receiver_register_plugin(digital_async_receiver *aCtx, const digital_async_receiver_interface *anInterface, digital_async_receiver_interface_ctx *anInterfaceCtx);

void    	digital_async_receiver_real_time_dump(digital_async_receiver *aCtx);

#ifdef __cplusplus
}
#endif

#endif
