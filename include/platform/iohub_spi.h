#ifndef _DRV_SPI_H_
#define _DRV_SPI_H_

#include "drivers/drv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	SPIMode_None = 0,
	SPIMode_WaitForMisoLowAfterSelect = 1 << 0,
}SPIMode;

/* -------------------------------------------------------------- */

typedef struct spi_ctx_s
{
    u32             mCSnPin;
    u8              mSelectedCount;
	SPIMode 		mMode;
}spi_ctx;

/* -------------------------------------------------------------- */

ret_code_t 		drv_spi_init(spi_ctx *aCtx, u32 aCSnPin, SPIMode aMode);

void    		drv_spi_uninit(spi_ctx *aCtx);

void    		drv_spi_select(spi_ctx *aCtx);

void    		drv_spi_deselect(spi_ctx *aCtx);

ret_code_t		drv_spi_write(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen);

ret_code_t		drv_spi_write_byte(spi_ctx *aCtx, u8 aByte);

ret_code_t 		drv_spi_read(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen);

u8      		drv_spi_read_byte(spi_ctx *aCtx);

ret_code_t		drv_spi_transfer(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen);

u8      		drv_spi_transfer_byte(spi_ctx *aCtx, u8 aByte);

#ifdef __cplusplus
}
#endif

#endif