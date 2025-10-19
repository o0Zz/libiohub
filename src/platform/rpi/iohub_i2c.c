#include "board/drv_i2c.h"
#include "wiringPiI2C.h"
#include <memory.h>
#include <stdlib.h>

#define I2C_DEV_ID              0

/* --------------------------------------------------------- */

int drv_i2c_init(i2c_ctx *aCtx, u8 aI2CDeviceAddr)
{
    memset(aCtx, 0x00, sizeof(i2c_ctx));

    aCtx->mCtx = wiringPiI2CSetup(aI2CDeviceAddr);
    aCtx->mI2CDeviceAddr = aI2CDeviceAddr;
    
    return SUCCESS;
}

/* --------------------------------------------------------- */

void drv_i2c_uninit(i2c_ctx *aCtx)
{   
}

/* --------------------------------------------------------- */

int drv_i2c_request_read(i2c_ctx *aCtx, BOOL afIssueStop)
{
    return SUCCESS;
}

/* --------------------------------------------------------- */

int drv_i2c_read(i2c_ctx *aCtx, u8 *aBuffer, const u16 aLen)
{
    int theRet;
    u16 i;

    for (i=0; i<aLen; i++)
    {
        theRet = wiringPiI2CRead (aCtx->mCtx);
        if (theRet < 0)
            return E_READ_ERROR;
        aBuffer[i] = theRet & 0xFF;
    }
    
    return SUCCESS;
}

/* --------------------------------------------------------- */

int drv_i2c_write(i2c_ctx *aCtx, const u8 *aBuffer, const u16 aLen, BOOL afIssueStop)
{
    int theRet;
    u16 i;
    
    for (i=0; i<aLen; i++)
    {
        theRet = wiringPiI2CWrite(aCtx->mCtx, aBuffer[i]);
        if (theRet < 0)
            return E_WRITE_ERROR;
    }
    
    return SUCCESS;
}
