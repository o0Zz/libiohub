#include "utils/iohub_digital_async_receiver.h"
#include "platform/iohub_platform.h"

//#define DEBUG_ASYNC_RECEIVER 1
#ifdef DEBUG_ASYNC_RECEIVER
volatile static u32 				gDurationUs = 0;
#endif

/* --------------------------------------------------- */

	//This function is timing dependant
	//Do not add any log in this function or it will break the timing
	static void IRAM_ATTR digital_async_receiver_handle_interrupt(void* arg)
	{
		digital_async_receiver *ctx = (digital_async_receiver *)arg;

		//IOHUB_LOG_DEBUG("Interrupt on pin %d", ctx->mPin);
		u32 theTimeUs = iohub_time_now_us();
		u32 theDurationUs = theTimeUs - ctx->mLastTimeUs;
		ctx->mLastTimeUs = theTimeUs;

#ifdef DEBUG_ASYNC_RECEIVER
		gDurationUs = theDurationUs;
#endif

		for(u8 i=0; i<ctx->mPluginCount; i++)
		{
			if (!IOHUB_GPIO_IS_BIT_SET(ctx->mPacketReady, i))
			{
				if (ctx->mInterfaceList[i]->detectPacket(ctx->mInterfaceCtx[i], (u16)theDurationUs))
				{
					IOHUB_GPIO_BIT_SET(ctx->mPacketReady, i);
				}
			}
		}
	}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_init(digital_async_receiver *ctx, u8 aPin)
{
	memset(ctx, 0x00, sizeof(digital_async_receiver));

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
	
	ctx->mIsRunning = TRUE;
	
	ret_code_t ret = iohub_attach_interrupt((gpio_num_t)ctx->mPin,
										   digital_async_receiver_handle_interrupt, 
										   IOHUB_GPIO_INT_TYPE_CHANGE, 
										   ctx);
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

	ctx->mIsRunning = FALSE;
}

/* --------------------------------------------------- */

BOOL iohub_digital_async_receiver_is_running(digital_async_receiver *ctx)
{
	return ctx->mIsRunning;
}

/* --------------------------------------------------- */

BOOL iohub_digital_async_receiver_register(digital_async_receiver *ctx, const digital_async_receiver_interface *anInterface, digital_async_receiver_interface_ctx *anInterfaceCtx)
{
	if (ctx->mPluginCount >= DIGITAL_ASYNC_RECEIVER_INTERFACE_MAX)
	{
		IOHUB_LOG_ERROR("Cannot register plugin, max count reached (%d)", DIGITAL_ASYNC_RECEIVER_INTERFACE_MAX);
		return FALSE;
	}

	ctx->mInterfaceList[ctx->mPluginCount] = anInterface;
	ctx->mInterfaceCtx[ctx->mPluginCount] = anInterfaceCtx;
	ctx->mPluginCount++;

	return TRUE;
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
				IOHUB_LOG_DEBUG("Packet available: %X", *receiverId);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

/* --------------------------------------------------- */

void iohub_digital_async_receiver_packet_handled(digital_async_receiver *ctx, u16 receiverId)
{
	for(u8 i=0; i<ctx->mPluginCount; i++)
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
#ifdef DEBUG_ASYNC_RECEIVER
	for(;;)
	{
		IOHUB_LOG_DEBUG("%lu", gDurationUs);
		iohub_time_delay_ms(1);
	}
#endif
}
