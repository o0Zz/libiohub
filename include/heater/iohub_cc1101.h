#ifndef _DRV_CC1101_H_
#define _DRV_CC1101_H_

// -----------------------------------------------------------------
// http://www.ti.com/lit/an/swra112b/swra112b.pdf
#define CC1101_REGISTERS_COUNT		0x29 //The chips have 47 configuration registers (address 0 to address 0x2E). (29 to 2E is for testing purpose)

/*
Wiring
======================

PINOUT RF1100SE: https://www.aeq-web.com/media/93480DA07062018125800.png
PINOUT CC1101 868: https://quadmeup.com/wp-content/uploads/2017/12/CC1101-868mhz-radio-module-pinout.jpg

	Arduino | RF1100SE or TI CC1101
-------------------------
	D10		| 7 (CSn)
	D11		| 3 (Mosi)
	D12		| 5 (Miso)
	D13		| 4 (Sck)
	x		| 8 (GD0)
	D03		| 6 (GD2)
	3v3		| 1 or 2 (VCC)
	GND		| 9 or 10 (Gnd)
	
    Arduino | CC1101 868
-------------------------
	D10		| 8 (CSn)
	D11		| 3 (Mosi)
	D12		| 5 (Miso)
	D13		| 4 (Sck)
	x		| 7 (GD0)
	D03		| 6 (GD2)
	3v3		| 1 (VCC)
	GND		| 2 (Gnd)
*/

#include "drivers/drv_common.h"
#include "board/drv_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CC1101_BUFFER_LEN        64
#define CC1101_DATA_LEN          (CC1101_BUFFER_LEN - 3)

/* -------------------------------------------------------------- */

typedef struct cc1101_packet_ctx_s
{
    u8      mLength;
	u8		mDst;
    u8      mData[CC1101_DATA_LEN];
	
    BOOL    mfCrcOk;
    u8      mRSSI;
    u8      mLinkQualityIndex;
}cc1101_packet_ctx;

/* -------------------------------------------------------------- */

typedef struct cc1101_ctx_s
{
    spi_ctx             mSPICtx;
    u32        			mGDO0Pin;
	s8					mWakeupCount;
}cc1101_ctx;

/* -------------------------------------------------------------- */

ret_code_t     	drv_cc1101_init(cc1101_ctx *aCtx, u32 aCSnPin, u32 aGDO0Pin, uint8_t aDefaultConfig[], uint8_t aFirstBytePaTable);
void    		drv_cc1101_uninit(cc1101_ctx *aCtx);

ret_code_t 		drv_cc1101_wakeup(cc1101_ctx *aCtx);
ret_code_t 		drv_cc1101_standby(cc1101_ctx *aCtx);

ret_code_t     	drv_cc1101_send_data(cc1101_ctx *aCtx, cc1101_packet_ctx *aPacket);

ret_code_t		drv_cc1101_set_receive(cc1101_ctx *aCtx);
ret_code_t     	drv_cc1101_receive_data(cc1101_ctx *aCtx, cc1101_packet_ctx *aPacket);

BOOL 			drv_cc1101_is_data_available(cc1101_ctx *aCtx);

#ifdef __cplusplus
}
#endif

#endif