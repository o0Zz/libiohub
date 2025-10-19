#ifndef _DRV_I2C_H_
#define _DRV_I2C_H_

#include "drivers/drv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------- */

typedef struct i2c_ctx_s
{
    void      *mCtx;
    u8        mI2CDeviceAddr;
}i2c_ctx;

/* -------------------------------------------------------------- */

int     	drv_i2c_init(i2c_ctx *aCtx, u8 aI2CDeviceAddr);

void    	drv_i2c_uninit(i2c_ctx *aCtx);

ret_code_t  drv_i2c_write(i2c_ctx *aCtx, const u8 *aBuffer, const u16 aLen, BOOL afIssueStop);

ret_code_t  drv_i2c_request_read(i2c_ctx *aCtx, BOOL afIssueStop);

ret_code_t  drv_i2c_read(i2c_ctx *aCtx, u8 *aBuffer, const u16 aLen);

#ifdef __cplusplus
}
#endif

#endif