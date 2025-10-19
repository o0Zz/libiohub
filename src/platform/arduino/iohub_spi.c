#include "board/drv_spi.h"
#include <SPI.h>

#define MOSI_PIN				11
#define MISO_PIN				12
#define SCK_PIN					13

/* ------------------------------------------------------------- */

ret_code_t drv_spi_init(spi_ctx *aCtx, u32 aCSnPin, SPIMode aMode)
{
    ret_code_t theRet;

    memset(aCtx, 0x00, sizeof(spi_ctx));

	aCtx->mCSnPin = aCSnPin;
	aCtx->mMode = aMode;
	
	//SPI.setFrequency(1000000);
	SPI.begin();

    drv_digital_set_pin_mode(aCtx->mCSnPin, PinMode_Output);
	
	drv_digital_write(aCtx->mCSnPin, PinLevel_High); //Deselect
	
    return theRet;
}

/* ------------------------------------------------------------- */

void drv_spi_uninit(spi_ctx *aCtx)
{
	SPI.end();
}

/* ------------------------------------------------------------- */

void drv_spi_select(spi_ctx *aCtx)
{
    ASSERT(aCtx->mSelectedCount < 255);

    if ( aCtx->mSelectedCount++ == 0 )
	{
        drv_digital_write(aCtx->mCSnPin, PinLevel_Low);
		
		if (aCtx->mMode & SPIMode_WaitForMisoLowAfterSelect)
		{
			while ( drv_digital_read(MISO_PIN) != PinLevel_Low );
		}
	}
}

/* ------------------------------------------------------------- */

void drv_spi_deselect(spi_ctx *aCtx)
{
    if ( aCtx->mSelectedCount == 0 )
		return;
	
    if ( --aCtx->mSelectedCount == 0 )
        drv_digital_write(aCtx->mCSnPin, PinLevel_High);
}

/* ------------------------------------------------------------- */

ret_code_t drv_spi_transfer(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen)
{
    ret_code_t theRet;

    drv_spi_select(aCtx);
	
	LOG_DEBUG("SPI Tx:");
	LOG_BUFFER(aBuffer, aBufferLen);
	
	for (u16 i=0; i<aBufferLen; i++)
		aBuffer[i] = SPI.transfer(aBuffer[i]);
    
	LOG_DEBUG("SPI Rx:");
    LOG_BUFFER(aBuffer, aBufferLen);
	
	drv_spi_deselect(aCtx);

    return theRet;
}

/* ------------------------------------------------------------- */

u8 drv_spi_transfer_byte(spi_ctx *aCtx, u8 aByte)
{
    u8 theByte = aByte;
	
    if (drv_spi_transfer(aCtx, &theByte, sizeof(theByte)) == SUCCESS)
        return theByte;

    return 0x00;
}
