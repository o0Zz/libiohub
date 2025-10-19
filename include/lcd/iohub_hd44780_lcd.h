#ifndef _DRV_HD44780_LCD_H_
#define _DRV_HD44780_LCD_H_

#include "drivers/drv_common.h"
#include "board/drv_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------- */

typedef struct hd44780_lcd_ctx_s
{
    i2c_ctx             mI2CCtx;
    u8                  mBacklightValue;
}hd44780_lcd_ctx;

/* -------------------------------------------------------------- */

    ///aI2CDevice should be: 0x27
int drv_hd44780_lcd_init(hd44780_lcd_ctx *aCtx, u8 aI2CDevice);

void drv_hd44780_lcd_uninit(hd44780_lcd_ctx *aCtx);

int drv_hd44780_lcd_set_backlight(hd44780_lcd_ctx *aCtx, BOOL afEnable);

int drv_hd44780_lcd_set_cursor(hd44780_lcd_ctx *aCtx, u8 aCol, u8 aRow);

int drv_hd44780_lcd_home(hd44780_lcd_ctx *aCtx);

int drv_hd44780_lcd_clear(hd44780_lcd_ctx *aCtx);

int drv_hd44780_lcd_write_string(hd44780_lcd_ctx *aCtx, const char *aStr);

int drv_hd44780_lcd_write_char(hd44780_lcd_ctx *aCtx, char aChar);

#ifdef __cplusplus
}
#endif

#endif