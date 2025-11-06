#include "utils/iohub_digital_async_receiver.h"
#include "platform/iohub_platform.h"

static digital_async_receiver		*gDigitalReceiver = NULL;

#ifdef DEBUG
volatile static u32 				gDurationUs = 0;
#endif

/* --------------------------------------------------- */

	//This function is timing dependant
	//Do not add any log in this function or it will break the timing
	static void IRAM_ATTR digital_async_receiver_handle_interrupt(void* arg)
	{
		u32 theTimeUs = iohub_time_now_us();
		u32 theDurationUs = theTimeUs - gDigitalReceiver->mLastTimeUs;
		gDigitalReceiver->mLastTimeUs = theTimeUs;
		
#ifdef DEBUG
		gDurationUs = theDurationUs;
#endif

		for(u8 i=0; i<gDigitalReceiver->mPluginCount; i++)
		{
			if (!IOHUB_GPIO_IS_BIT_SET(gDigitalReceiver->mPacketReady, i))
			{
				if (gDigitalReceiver->mInterfaceList[i]->detectPacket(gDigitalReceiver->mInterfaceCtx[i], (u16)theDurationUs))
				{
					IOHUB_GPIO_BIT_SET(gDigitalReceiver->mPacketReady, i);
				}
			}
		}
	}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_init(digital_async_receiver *ctx, u8 aPin)
{
	memset(ctx, 0x00, sizeof(digital_async_receiver));

	gDigitalReceiver = ctx;
	ctx->mPin = aPin;
	ctx->mLastTimeUs = iohub_time_now_us();
	
	iohub_digital_set_pin_mode(aPin, PinMode_Input);
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_uninit(digital_async_receiver *ctx)
{
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_start(digital_async_receiver *ctx)
{
	IOHUB_LOG_INFO("Attaching interrupt to pin %d", ctx->mPin);
	
	ret_code_t ret = iohub_attach_interrupt((gpio_num_t)ctx->mPin,
										   digital_async_receiver_handle_interrupt, 
										   IOHUB_GPIO_INT_TYPE_CHANGE, 
										   NULL);
	if (ret != SUCCESS) {
		IOHUB_LOG_ERROR("Failed to attach interrupt to pin %d: %s", ctx->mPin, esp_err_to_name(ret));
	}
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_stop(digital_async_receiver *ctx)
{
	ret_code_t ret = iohub_detach_interrupt(ctx->mPin);
	if (ret != SUCCESS) {
		IOHUB_LOG_ERROR("Failed to detach interrupt from pin %d: %s", ctx->mPin, esp_err_to_name(ret));
	}
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_register_plugin(digital_async_receiver *ctx, const digital_async_receiver_interface *anInterface, digital_async_receiver_interface_ctx *anInterfaceCtx)
{
	ctx->mInterfaceList[ctx->mPluginCount] = anInterface;
	ctx->mInterfaceCtx[ctx->mPluginCount] = anInterfaceCtx;
	ctx->mPluginCount++;
}

/* --------------------------------------------------- */

u16 iohub_digital_async_receiver_wait_for_packet(digital_async_receiver *ctx)
{
	u16 receiverId = 0;

	while(!iohub_digital_async_receiver_has_packet_available(ctx, &receiverId));

	return receiverId;
}

/* --------------------------------------------------- */

BOOL iohub_digital_async_receiver_has_packet_available(digital_async_receiver *ctx, u16 *receiverId)
{
	if (ctx->mPacketReady)
	{	
		for (u8 i=0; i<(sizeof(ctx->mPacketReady) * 8); i++)
		{
			if (IOHUB_GPIO_IS_BIT_SET(ctx->mPacketReady, i))
			{
				*receiverId = ctx->mInterfaceList[i]->Id;
				IOHUB_LOG_ERROR("Packet available: %X", *receiverId);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_packet_handled(digital_async_receiver *ctx, u16 receiverId)
{
	for(u8 i=0; i<gDigitalReceiver->mPluginCount; i++)
	{
		if (ctx->mInterfaceList[i]->Id == receiverId)
		{
			ctx->mInterfaceList[i]->packetHandled(ctx->mInterfaceCtx[i]);
			IOHUB_GPIO_BIT_CLEAR(ctx->mPacketReady, i);
			return;
		}
	}
	
	IOHUB_LOG_ERROR("ReceiverId not found: %d", receiverId);
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_real_time_dump(digital_async_receiver *ctx)
{
#ifdef DEBUG
	for(;;)
		IOHUB_LOG_DEBUG("%lu", gDurationUs);
#endif
}
