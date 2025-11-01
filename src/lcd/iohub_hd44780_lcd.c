#include "lcd/iohub_hd44780_lcd.h"

#ifndef WIN32
//#include <unistd.h>
#endif

/*

http://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller

Pins
======================================
Ground
VCC (+3.3 to +5V)
Contrast adjustment (VO)
Register Select (RS).   RS=0: Command, 
            RS=1: Data
Read/Write (R/W).   R/W=0: Write, 
            R/W=1: Read (This pin is optional due to the fact that most of the time you will only want to write to it and not read. Therefore, in general use, this pin will be permanently connected directly to ground.)

Clock (Enable). Falling edge triggered
Bit 0 (Not used in 4-bit operation)
Bit 1 (Not used in 4-bit operation)
Bit 2 (Not used in 4-bit operation)
Bit 3 (Not used in 4-bit operation)
Bit 4
Bit 5
Bit 6
Bit 7
Backlight Anode (+) (If applicable)
Backlight Cathode (-) (If applicable)

Commands
======================================
                RS      R/W  |   B7      B6      B5      B4      B3      B2      B1      B0
Clear display   0       0    |   0       0       0       0       0       0       0       1       Clears display and returns cursor to the home position (address 0).
Cursor home     0       0    |   0       0       0       0       0       0       1       *       Returns cursor to home position. Also returns display being shifted to the original position. DDRAM content remains unchanged
Entry mode set  0       0    |   0       0       0       0       0       1       I/D     S       "Sets cursor move direction (I/D); specifies to shift the display (S). These operations are performed during data read/write
Display control 0       0    |   0       0       0       0       1       D       C       B       Sets on/off of all display (D), cursor on/off (C), and blink of cursor position character (B)
Cursor shift    0       0    |   0       0       0       1       S/C     R/L     *       *       Sets cursor-move or display-shift (S/C), shift direction (R/L). DDRAM content remains unchanged
Function set    0       0    |   0       0       1       DL      N       F       *       *       Sets interface data length (DL), number of display line (N), and character font (F)
Set CGRAM addr  0       0    |   0       1       CGRAM address                                   Sets the CGRAM address. CGRAM data are sent and received after this setting
Set DDRAM addr  0       0    |   1       DDRAM address                                           Sets the DDRAM address. DDRAM data are sent and received after this setting
Read flag/addr  0       1    |   BF      CGRAM/DDRAM address                                     Reads busy flag (BF) indicating internal operation being performed and reads CGRAM or DDRAM address counter contents (depending on previous instruction)
Write CG/DDRAM  1       0    |   Write Data                                                      Write data to CGRAM or DDRAM
Read CG/DDRAM   1       1    |   Read Data                                                       Read data from CGRAM or DDRAM

Instruction bit names
======================
I/D - 0 = decrement cursor position
      1 = increment cursor position
S   - 0 = no display shift
      1 = display shift
D   - 0 = display off
      1 = display on
C   - 0 = cursor off
      1 = cursor on
B   - 0 = cursor blink off
      1 = cursor blink on
S/C - 0 = move cursor
      1 = shift display
R/L - 0 = shift left
      1 = shift right
DL  - 0 = 4-bit interface
      1 = 8-bit interface
N   - 0 = 1/8 or 1/11 duty (1 line)
      1 = 1/16 duty (2 lines)
F   - 0 = 5�8 dots
      1 = 5�10 dots
BF  - 0 = can accept instruction
      1 = internal operation in progress


https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library/blob/master/LiquidCrystal_I2C.h
https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library/blob/master/LiquidCrystal_I2C.cpp
*/

// Commands
#define LCD_CLEARDISPLAY                0x01
#define LCD_RETURNHOME                  0x02
#define LCD_ENTRYMODESET                0x04
#define LCD_DISPLAYCONTROL              0x08
#define LCD_CURSORSHIFT                 0x10
#define LCD_FUNCTIONSET                 0x20
#define LCD_SETCGRAMADDR                0x40
#define LCD_SETDDRAMADDR                0x80

// ENTRYMODESET
#define LCD_ENTRYMODESET_RIGHT          0x00
#define LCD_ENTRYMODESET_LEFT           0x02
#define LCD_ENTRYMODESET_SHIFTINCREMENT 0x01
#define LCD_ENTRYMODESET_SHIFTDECREMENT 0x00

// DISPLAYCONTROL
#define LCD_DISPLAYCONTROL_ON           0x04
#define LCD_DISPLAYCONTROL_OFF          0x00
#define LCD_DISPLAYCONTROL_CURSORON     0x02
#define LCD_DISPLAYCONTROL_CURSOROFF    0x00
#define LCD_DISPLAYCONTROL_BLINKON      0x01
#define LCD_DISPLAYCONTROL_BLINKOFF     0x00

// CURSORSHIFT
#define LCD_CURSORSHIFT_DISPLAYMOVE     0x08
#define LCD_CURSORSHIFT_CURSORMOVE      0x00
#define LCD_CURSORSHIFT_MOVERIGHT       0x04
#define LCD_CURSORSHIFT_MOVELEFT        0x00

// FUNCTIONSET
#define LCD_FUNCTIONSET_8BITMODE    0x10
#define LCD_FUNCTIONSET_4BITMODE    0x00
#define LCD_FUNCTIONSET_2LINE       0x08
#define LCD_FUNCTIONSET_1LINE       0x00
#define LCD_FUNCTIONSET_5x10DOTS    0x04
#define LCD_FUNCTIONSET_5x8DOTS     0x00

// BACKLIGHT
#define LCD_BACKLIGHT_ON            0x08
#define LCD_BACKLIGHT_OFF           0x00

#define BIT_EN                      0x04    // Enable bit
#define BIT_RW                      0x02    // Read/Write bit
#define BIT_RS                      0x01    // Register select bit

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_command(hd44780_lcd_ctx *aCtx, unsigned char aCommand);
int iohub_hd44780_lcd_write_byte(hd44780_lcd_ctx *aCtx, unsigned char aByte, unsigned char aMode);
int iohub_hd44780_lcd_write_4bits(hd44780_lcd_ctx *aCtx, u8 aByte);
int iohub_hd44780_lcd_write_hardware(hd44780_lcd_ctx *aCtx, u8 aByte);
int iohub_hd44780_lcd_pulse_enable(hd44780_lcd_ctx *aCtx, u8 aByte);

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_init(hd44780_lcd_ctx *aCtx, u8 aI2CDevice)
{
    int theRet;

    memset(aCtx, 0x00, sizeof(hd44780_lcd_ctx));
    aCtx->mBacklightValue = LCD_BACKLIGHT_ON;

    theRet = iohub_i2c_init(&aCtx->mI2CCtx,  aI2CDevice);
    if (theRet != SUCCESS)
        return theRet;

        //HD44780 Initialisation. Refer you to HD44780 specification, p46
    iohub_hd44780_lcd_write_4bits(aCtx, 0x03 << 4);
    time_delay_us(4500); // Wait min 4.1ms
    iohub_hd44780_lcd_write_4bits(aCtx, 0x03 << 4);
    time_delay_us(4500); // Wait min 4.1ms
    iohub_hd44780_lcd_write_4bits(aCtx, 0x03 << 4);
    time_delay_us(150); // Wait min 100us
    iohub_hd44780_lcd_write_4bits(aCtx, 0x02 << 4);

    iohub_hd44780_lcd_write_command(aCtx, LCD_FUNCTIONSET | LCD_FUNCTIONSET_4BITMODE | LCD_FUNCTIONSET_2LINE | LCD_FUNCTIONSET_5x8DOTS);
    iohub_hd44780_lcd_write_command(aCtx, LCD_DISPLAYCONTROL | LCD_DISPLAYCONTROL_ON | LCD_DISPLAYCONTROL_CURSOROFF | LCD_DISPLAYCONTROL_BLINKOFF);
    iohub_hd44780_lcd_write_command(aCtx, LCD_ENTRYMODESET | LCD_ENTRYMODESET_LEFT | LCD_ENTRYMODESET_SHIFTDECREMENT);

    iohub_hd44780_lcd_clear(aCtx);
    iohub_hd44780_lcd_home(aCtx);

    return SUCCESS;
}

/* ----------------------------------------------------- */

void iohub_hd44780_lcd_uninit(hd44780_lcd_ctx *aCtx)
{
    iohub_i2c_uninit(&aCtx->mI2CCtx);
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_set_backlight(hd44780_lcd_ctx *aCtx, BOOL afEnable)
{
    int theRet = SUCCESS;

    unsigned char theNewBacklightValue = afEnable ? LCD_BACKLIGHT_ON : LCD_BACKLIGHT_OFF;
    if (aCtx->mBacklightValue != theNewBacklightValue)
    {
        aCtx->mBacklightValue = theNewBacklightValue;
        theRet = iohub_hd44780_lcd_write_hardware(aCtx, 0x00);
    }

    return theRet;
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_set_cursor(hd44780_lcd_ctx *aCtx, u8 aCol, u8 aRow)
{
    static int sRowOffsets[] = { 0x00, 0x40, 0x14, 0x54 };
    
    if (aRow > 4)
        aRow = 4;

    if (aRow < 1)
        aRow = 1;

    return iohub_hd44780_lcd_write_command(aCtx, LCD_SETDDRAMADDR | (aCol + sRowOffsets[aRow - 1]));
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_home(hd44780_lcd_ctx *aCtx)
{
    int theRet = iohub_hd44780_lcd_write_command(aCtx, LCD_RETURNHOME);

    time_delay_us(2000); //This command takes a long time

    return theRet;
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_clear(hd44780_lcd_ctx *aCtx)
{
    int theRet = iohub_hd44780_lcd_write_command(aCtx, LCD_CLEARDISPLAY);

    time_delay_us(2000); //This command takes a long time

    return theRet;
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_string(hd44780_lcd_ctx *aCtx, const char *aStr)
{
    int i = 0;

    while(aStr[i] != '\0') 
    {
        int theRet = iohub_hd44780_lcd_write_char(aCtx, aStr[i]);
        if (theRet != SUCCESS)
            return theRet;

        i++;
    }
        
    return SUCCESS;
}       

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_char(hd44780_lcd_ctx *aCtx, char aChar)
{
    return iohub_hd44780_lcd_write_byte(aCtx, aChar, BIT_RS);
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_command(hd44780_lcd_ctx *aCtx, unsigned char aCommand)
{
    return iohub_hd44780_lcd_write_byte(aCtx, aCommand, 0x00);
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_byte(hd44780_lcd_ctx *aCtx, unsigned char aByte, unsigned char aMode)
{
    unsigned char theHigh = aByte & 0xF0;
    unsigned char theLow = (aByte<<4) & 0xF0;

    int theRet = iohub_hd44780_lcd_write_4bits(aCtx, theHigh | aMode);
    if (theRet == SUCCESS)
        theRet = iohub_hd44780_lcd_write_4bits(aCtx, theLow | aMode); 

    return theRet;
}
 
/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_4bits(hd44780_lcd_ctx *aCtx, u8 aByte)
{
    int theRet = iohub_hd44780_lcd_write_hardware(aCtx, aByte);
    if (theRet == SUCCESS)
        theRet = iohub_hd44780_lcd_pulse_enable(aCtx, aByte);

    return theRet;
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_write_hardware(hd44780_lcd_ctx *aCtx, u8 aByte)
{
    unsigned char theByte = aByte | aCtx->mBacklightValue;
    return iohub_i2c_write(&aCtx->mI2CCtx, &theByte, sizeof(theByte), TRUE);
}

/* ----------------------------------------------------- */

int iohub_hd44780_lcd_pulse_enable(hd44780_lcd_ctx *aCtx, u8 aByte)
{
        // En bit high
    int theRet = iohub_hd44780_lcd_write_hardware(aCtx, aByte | BIT_EN);
    if (theRet != SUCCESS)
        return theRet;

        // Enable pulse must be >450ns
    time_delay_us(1);

        // En bit low
    theRet = iohub_hd44780_lcd_write_hardware(aCtx, aByte & ~BIT_EN);
    if (theRet != SUCCESS)
        return theRet;

        // Commands need > 37us to settle
    time_delay_us(50);

    return SUCCESS;
}
