#ifndef _DRV_LIGHT_SEAMAID_H_
#define _DRV_LIGHT_SEAMAID_H_

#include "utils/iohub_types.h"
#include "components/digital_async_receiver/digital_async_receiver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_LIGHT_SEAMAID_ID				0x5EA0

typedef enum
{
	LightSeamaidCmd_ON					= 0x80,
	LightSeamaidCmd_OFF					= 0x8F,
			
	LightSeamaidCmd_White				= 0x20,
	LightSeamaidCmd_Blue				= 0x21,
	LightSeamaidCmd_Cyan				= 0x22,
	LightSeamaidCmd_Green				= 0x23,
	LightSeamaidCmd_Yellow				= 0x24,
	LightSeamaidCmd_Orange				= 0x25,
	LightSeamaidCmd_Purple				= 0x26,
	
	LightSeamaidCmd_ChangeSmoothSlow	= 0x27,
	LightSeamaidCmd_ChangeSmooth		= 0x28,
	LightSeamaidCmd_ChangeBlink			= 0x29
}LightSeamaidCmd;


/* -------------------------------------------------------------- */

typedef struct light_seamaid_s
{	
    u8					mDigitalPinTx;
	u16					mTimings[(24+1) * 2];
	u16					mTimingCount;
	u16					mTimingReadIdx;
}light_seamaid;

/* -------------------------------------------------------------- */

//anAddr = 0x800E

int     								iohub_light_seamaid_init(light_seamaid *aCtx, u8 aGPIOTx);
void    								iohub_light_seamaid_uninit(light_seamaid *aCtx);

void 									iohub_light_seamaid_send(light_seamaid *aCtx, u16 anAddr, u8 aCmd);

BOOL    								iohub_light_seamaid_read(light_seamaid *aCtx, u16 *anAddr, u8 *aCmd);

const digital_async_receiver_interface 	*iohub_light_seamaid_get_interface(void);

void 									iohub_light_seamaid_dump_timings(light_seamaid *aCtx);

#ifdef __cplusplus
}
#endif

#endif