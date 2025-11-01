#include "platform/iohub_spi.h"
#include "mpsse.h"
#include <memory.h>
#include <stdlib.h>

/*
Hardware setup
--------------
D0 - SCLK
D1 - MOSI
D2 - MISO
D3 - CS

CSn - D4 - D7
*/

extern struct mpsse_context *sMPSSECtx;

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_init(spi_ctx *aCtx, u32 aCSnPin, IOHubSPIMode aMode)
{
    ret_code_t theRet;

    memset(aCtx, 0x00, sizeof(spi_ctx));

	aCtx->mCSnPin = aCSnPin;
	aCtx->mMode = aMode;
	
    if (sMPSSECtx == NULL || !sMPSSECtx->open)
        sMPSSECtx = MPSSE(SPI0, ONE_HUNDRED_KHZ, MSB);

    if (sMPSSECtx == NULL || !sMPSSECtx->open)
    {
        log_err("Device FTDI not found");
        return E_DEVICE_NOT_FOUND;
    }

    IOHUB_ASSERT(sMPSSECtx->mode == SPI0);

    theRet = iohub_digital_set_pin_mode(aCtx->mCSnPin, PinMode_Output);;
    if (theRet == SUCCESS)
        iohub_digital_write(aCtx->mCSnPin, PinLevel_High); //Deselect

    return theRet;
}

/* ------------------------------------------------------------- */

void iohub_spi_uninit(spi_ctx *aCtx)
{
    Close(sMPSSECtx);
}

/* ------------------------------------------------------------- */

void iohub_spi_select(spi_ctx *aCtx)
{
    IOHUB_ASSERT(aCtx->mSelectedCount < 255);

    if ( aCtx->mSelectedCount++ == 0 )
	{
        iohub_digital_write(aCtx->mCSnPin, PinLevel_Low);
		
		if (aCtx->mMode & SPIMode_WaitForMisoLowAfterSelect)
		{
			while ( iohub_digital_read(MISO_PIN) != PinLevel_Low );
		}
	}
}

/* ------------------------------------------------------------- */

void iohub_spi_deselect(spi_ctx *aCtx)
{
    if ( aCtx->mSelectedCount == 0 )
		return;
	
    if ( --aCtx->mSelectedCount == 0 )
        iohub_digital_write(aCtx->mCSnPin, PinLevel_High);
}

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_transfer(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen)
{
    ret_code_t      theRet = SUCCESS;
    u8      		*theRcvBuffer;

    iohub_spi_select(aCtx);

    if (Start(sMPSSECtx) == MPSSE_OK)
    {
        if (Write(sMPSSECtx, aBuffer, aBufferLen) != MPSSE_OK)
        {
            log_err("Write failed");
            theRet = E_WRITE_ERROR;
        }

        theRcvBuffer = Read(sMPSSECtx, aBufferLen);
        if (theRcvBuffer != NULL)
        {
	        memcpy(aBuffer, theRcvBuffer, aBufferLen);
            free(theRcvBuffer);
        }

        Stop(sMPSSECtx);
    }
    else
    {
        log_err("Start failed");
        theRet = E_WRITE_ERROR;
    }

    iohub_spi_deselect(aCtx);

    return theRet;
}

/* ------------------------------------------------------------- */

u8 iohub_spi_transfer_byte(spi_ctx *aCtx, u8 aByte)
{
    u8 theByte = aByte;
    if (iohub_spi_transfer(aCtx, &theByte, sizeof(theByte)) == SUCCESS)
        return theByte;

    return 0x00;
}

