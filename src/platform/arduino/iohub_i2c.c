#include "board/drv_i2c.h"
#include <Wire.h>

/* --------------------------------------------------------- */

int drv_i2c_init(i2c_ctx *aCtx, u8 aI2CDeviceAddr)
{
    memset(aCtx, 0x00, sizeof(i2c_ctx));
  
    Wire.begin();
    
    aCtx->mI2CDeviceAddr = aI2CDeviceAddr;
    return SUCCESS;
}

/* --------------------------------------------------------- */

void drv_i2c_uninit(i2c_ctx *aCtx)
{
}

/* --------------------------------------------------------- */

ret_code_t drv_i2c_request_read(i2c_ctx *aCtx, BOOL afIssueStop)
{
    Wire.requestFrom(aCtx->mI2CDeviceAddr, (u8)1);
    return SUCCESS;
}

/* --------------------------------------------------------- */

ret_code_t drv_i2c_read(i2c_ctx *aCtx, unsigned char *aBuffer, const unsigned int aLen)
{
    u8 i=0;
    
    for (i=0; i<aLen; i++)
        aBuffer[i] = Wire.read();
    
    return SUCCESS;
}

/* --------------------------------------------------------- */

ret_code_t drv_i2c_write(i2c_ctx *aCtx, const unsigned char *aBuffer, const unsigned int aLen, BOOL afIssueStop)
{
    u8 i=0;

    Wire.beginTransmission(aCtx->mI2CDeviceAddr);

    for (unsigned int i=0; i<aLen; i++)
        Wire.write(aBuffer[i]);

    Wire.endTransmission(afIssueStop);
    return SUCCESS;
}
