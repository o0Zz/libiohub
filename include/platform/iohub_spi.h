#pragma once

#include "utils/iohub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	SPIMode_None = 0,
	SPIMode_WaitForMisoLowAfterSelect = 1 << 0,
}IOHubSPIMode;

/* -------------------------------------------------------------- */

typedef struct spi_ctx_s
{
    u32             mCSnPin;
    u8              mSelectedCount;
	IOHubSPIMode 	mMode;
	void			*mCtx;
}spi_ctx;

/* -------------------------------------------------------------- */

ret_code_t 		iohub_spi_init(spi_ctx *aCtx, u32 aCSnPin, IOHubSPIMode aMode);

void    		iohub_spi_uninit(spi_ctx *aCtx);

void    		iohub_spi_select(spi_ctx *aCtx);

void    		iohub_spi_deselect(spi_ctx *aCtx);

ret_code_t		iohub_spi_write(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen);

ret_code_t		iohub_spi_write_byte(spi_ctx *aCtx, u8 aByte);

ret_code_t 		iohub_spi_read(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen);

u8      		iohub_spi_read_byte(spi_ctx *aCtx);

ret_code_t		iohub_spi_transfer(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen);

u8      		iohub_spi_transfer_byte(spi_ctx *aCtx, u8 aByte);

#ifdef __cplusplus
}
#endif
