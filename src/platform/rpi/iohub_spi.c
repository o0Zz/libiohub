#include "platform/iohub_spi.h"
#include <memory.h>
#include <stdlib.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

/*
 *  Enable SPI on raspberry pi:
 *      1)Remove SPI from the Blacklist
 *          vi /etc/modprobe.d/raspi-blacklist.conf
 *          #blacklist spi-bcm2708
 *
 *      2)Reboot
 *
*/

#define SPI_DEV             "/dev/spidev0.0"
#define SPI_SPEED           100000 //100 KHZ

static int              sSPIFD = 0;
const static u8         spiMode  = 0 ;
const static u8         spiBPW   = 8 ;
const static u16        spiDelay = 0 ;

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_init(spi_ctx *aCtx, u32 aCSnPin, IOHubSPIMode aMode)
{
    ret_code_t theRet;
    int theSpeed = SPI_SPEED;

    memset(aCtx, 0x00, sizeof(spi_ctx));
	aCtx->mCSnPin = aCSnPin;
	aCtx->mMode = aMode;
	
    if ((sSPIFD = open (SPI_DEV, O_RDWR)) < 0)
      return E_DEVICE_INIT_FAILED;

    if (ioctl (sSPIFD, SPI_IOC_WR_MODE, &spiMode) < 0)
        return E_DEVICE_INIT_FAILED;

    if (ioctl (sSPIFD, SPI_IOC_WR_BITS_PER_WORD, &spiBPW) < 0)
        return E_DEVICE_INIT_FAILED;

    if (ioctl (sSPIFD, SPI_IOC_WR_MAX_SPEED_HZ, &theSpeed) < 0)
        return E_DEVICE_INIT_FAILED;

    theRet = iohub_digital_set_pin_mode(aCtx->mCSnPin, PinMode_Output);
    if (theRet == SUCCESS)
		iohub_digital_write(aCtx->mCSnPin, PinLevel_High); //Deselect

    return theRet;
}

/* ------------------------------------------------------------- */

void iohub_spi_uninit(spi_ctx *aCtx)
{
    close(sSPIFD);
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
    int                     theRet = SUCCESS;
    struct spi_ioc_transfer theSPI;

    iohub_spi_select(aCtx);
	
    theSPI.tx_buf        = (unsigned long)aBuffer;
    theSPI.rx_buf        = (unsigned long)aBuffer;
    theSPI.len           = aBufferLen;
    theSPI.delay_usecs   = spiDelay;
    theSPI.speed_hz      = SPI_SPEED;
    theSPI.bits_per_word = spiBPW;

    if (ioctl (sSPIFD, SPI_IOC_MESSAGE(1), &theSPI) < 0)
        theRet = E_READ_ERROR;
        
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
