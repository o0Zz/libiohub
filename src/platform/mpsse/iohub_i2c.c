#include "board/drv_i2c.h"
#include "mpsse.h"
#include <memory.h>
#include <stdlib.h>

/*
Hardware setup:
---------------
    Pull up resistor 4.7K from D0 to VCC
    Pull up resistor 4.7K rom D1 to VCC
    D1 and D2 must be linked

    SCL = D0
    SDA = D1/D2
*/

extern struct mpsse_context *sMPSSECtx;

/* --------------------------------------------------------- */

int drv_i2c_init(i2c_ctx *aCtx, u8 aI2CDeviceAddr)
{
    memset(aCtx, 0x00, sizeof(i2c_ctx));
    
    if (sMPSSECtx == NULL || !sMPSSECtx->open)
        sMPSSECtx = MPSSE(I2C, ONE_HUNDRED_KHZ, MSB);

    if (sMPSSECtx == NULL || !sMPSSECtx->open)
    {
        log_err("Device FTDI not found");
        return E_DEVICE_NOT_FOUND;
    }

    ASSERT(sMPSSECtx->mode == I2C);

    /*
    theRet = Tristate(sMPSSECtx);
    if (theRet != MPSSE_OK)
        return E_FAIL_INIT_DEVICE;
    */

    aCtx->mI2CDeviceAddr = aI2CDeviceAddr;
    return SUCCESS;
}

/* --------------------------------------------------------- */

void drv_i2c_uninit(i2c_ctx *aCtx)
{
    Close(sMPSSECtx);
}

/* --------------------------------------------------------- */

int drv_i2c_request_read(i2c_ctx *aCtx, BOOL afIssueStop)
{
    u8          theFirstByte;
   
    if (Start(sMPSSECtx) != MPSSE_OK)
    {
        log_err("Start bit failed");
        return E_WRITE_ERROR;
    }
    
    theFirstByte = aCtx->mI2CDeviceAddr << 1;
    theFirstByte |= 0x01; //Read

    if(Write(sMPSSECtx, &theFirstByte, 1) != MPSSE_OK)
    {
        log_err("Request read failed");
        return E_WRITE_ERROR;
    }

    if(GetAck(sMPSSECtx) != ACK)
    {
        log_err("Got NACK for slave address");
        return E_FAIL_ACK;
    }

    if (afIssueStop)
        Stop(sMPSSECtx);

    return SUCCESS;
}

/* --------------------------------------------------------- */

int drv_i2c_read(i2c_ctx *aCtx, u8 *aBuffer, const u16 aLen)
{
    u8 *theBuffer = Read(sMPSSECtx, aLen);
    if(theBuffer == NULL)
    {
        log_err("Read failed");
        return E_READ_ERROR;
    }

    SendAcks(sMPSSECtx);

    memcpy(aBuffer, theBuffer, aLen);
    free(theBuffer);

    return SUCCESS;
}

/* --------------------------------------------------------- */

int drv_i2c_write(i2c_ctx *aCtx, const u8 *aBuffer, const u16 aLen, BOOL afIssueStop)
{
    u8 theFirstByte;
    int theRet;

        //According to http://i2c.info/i2c-bus-specification
    if(Start(sMPSSECtx) != MPSSE_OK)
    {
        log_err("Start failed");
        return E_WRITE_ERROR;
    }
    
    theFirstByte = aCtx->mI2CDeviceAddr << 1;
    theFirstByte |= 0x00; //Write

    if(Write(sMPSSECtx, &theFirstByte, 1) != MPSSE_OK)
    {
        log_err("Request write failed");
        return E_WRITE_ERROR;
    }

    if(GetAck(sMPSSECtx) != ACK)
    {
        log_err("Got NACK for slave address");
        return E_FAIL_ACK;
    }

    theRet = Write(sMPSSECtx, aBuffer, aLen);
    if (theRet != MPSSE_OK)
        return E_WRITE_ERROR;

    if(GetAck(sMPSSECtx) != ACK)
    {
        log_err("Got NACK for data");
        return E_FAIL_ACK;
    }

    if (afIssueStop)
        Stop(sMPSSECtx);

    return SUCCESS;
}
