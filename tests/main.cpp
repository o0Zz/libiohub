#include "ozTools/ozConsole.h"
#include "ozTools/ozCommandLineParser.h"
#include "ozTools/ozThread.h"
#include "drv_chacon_dio.h"
#include "drv_button_matrix.h"
#include "drv_hd44780_lcd.h"
#include "drv_button_matrix.h"
#include "drv_rs8706w_weatherlink.h"
#include "drv_cc1101.h"

#define CC1101_SAUTER
#include "drv_cc1101_default_value.h"

#define kozLogTag "HARDIO"

#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------------------------- 
                                        Main
   -------------------------------------------------------------------------------------------- */

static ozCommandLineParser_Options sOptionsList[] =
{
      //GPIO
    {   "-on",                  "Set pin ID HIGH",                                  ozCommandLineParserType_Option_Optional},
    {   "-off",                 "Set pin ID LOW",                                   ozCommandLineParserType_Option_Optional},

        //LCD
    {   "-lcd line1\\\\nline2", "Write on LCD 2 lines",                             ozCommandLineParserType_Option_Optional},
    {   "-lcdbacklight 1/0",    "Turn on/off lcd backlight (Default: 1)",           ozCommandLineParserType_Option_Optional},
    
        //Buttons
    {   "-buttonmatrix",        "Listening and display button matrix input",        ozCommandLineParserType_Switch},
   
        //CC1101
    {   "-cc1101send",          "Use CC1101 to send a test packet",                 ozCommandLineParserType_Option_Optional},
    {   "-cc1101listen",        "Use CC1101 to receive packets and print them",     ozCommandLineParserType_Switch},

        //Weather station inovalley
    {   "-inovalley",           "Listen and display inovalley weather station data",ozCommandLineParserType_Switch},

        //DIO
    {   "-dio",                 "on or off. Set interruptor to ON or OFF",          ozCommandLineParserType_Option_Optional},
    {   "-diosender",           "26 bits representing sender id (i.e: 12794898)",   ozCommandLineParserType_Option_Optional},
    {   "-diointerruptor",      "4 bits representing the interruptor id (i.e: 1)",  ozCommandLineParserType_Option_Optional},
    {   "-diolisten",           "Listen DIO commands and print them",               ozCommandLineParserType_Switch},

    {   "-h",                   "Help, this screen",                                ozCommandLineParserType_Switch},
    {   NULL,                   NULL,                                               ozCommandLineParserType_Unknown}
};

/* ------------------------------------------------------------------------------------ */

void lcd(const ozString &aString, int afBacklight)
{
    hd44780_lcd_ctx   theLCD;

    drv_hd44780_lcd_init(&theLCD, 0x27);
    drv_hd44780_lcd_set_backlight(&theLCD, afBacklight);

    drv_hd44780_lcd_set_cursor(&theLCD, 0, 1);
    drv_hd44780_lcd_write_string(&theLCD, aString.beforeFirst("\\n"));

    drv_hd44780_lcd_set_cursor(&theLCD, 0, 2);
    drv_hd44780_lcd_write_string(&theLCD, aString.afterFirst("\\n"));    
}

/* ------------------------------------------------------------------------------------ */

void matrixbutton()
{
    button_matrix_ctx theButtonMatrix;

    u8 theKeysMap[4][4] =
    {
      { 13, 9, 5, 1},
      { 14,10, 6, 2},
      { 15,11, 7, 3},
      { 16,12, 8, 4}
    };

    u32 theGPIOCols[] = {4,5,6,7};
    u32 theGPIORows[] = {0,1,2,3};

    drv_button_matrix_init(&theButtonMatrix, theKeysMap, theGPIOCols, theGPIORows);

    for(;;)
    {
        char theKey = drv_button_matrix_get_key_pressed(&theButtonMatrix);
        if (theKey != 0)
        {
            ozConsole_Printf("Key pressed: %d\n", (int)theKey);
        }
        else
            ozThread_Sleep(20);
    }

    drv_button_matrix_uninit(&theButtonMatrix);
    
}

/* ------------------------------------------------------------------------------------ */

void cc1101Send(const ozString &aPacket)
{
    cc1101_ctx          theCtx;
    cc1101_packet_ctx   thePacket;
    unsigned char       theData[] = { 0x0, 0x20, 0x5a, 0xb5, 0x7, 0x52, 0x0, 0x0, 0x0, 0x54, 0x0, 0x0, 0x0, 0x62, 0x0, 0x66, 0x2, 0x70, 0x0, 0x0, 0x0, 0x8, 0xb, 0xbb, 0x77 };

    if ( drv_cc1101_init(&theCtx, 0, 1, sCC1101_Default_Config) != SUCCESS )
        return;
    
    for(;;)
    {   
        thePacket.mLength = sizeof(theData);
        memcpy(thePacket.mData, theData, thePacket.mLength);

        int theRet = drv_cc1101_send(&theCtx, &thePacket);
        if ( theRet != SUCCESS )
            ozLog_Error("Error while sending packet: %d", theRet);

        ozThread_Sleep(2000);
    }
}

/* ------------------------------------------------------------------------------------ */

void cc1101Listen()
{
    cc1101_ctx          theCtx;
    cc1101_packet_ctx   thePacket;

    drv_cc1101_init(&theCtx, 0, 1, sCC1101_Default_Config);

    for(;;)
    {   
        if ( drv_cc1101_receive(&theCtx, &thePacket) > 0 )
        {
            ozLog_Debug("Packet received: ");
            ozLog_Buffer(thePacket.mData, thePacket.mLength);
        }
    }
}

/* ------------------------------------------------------------------------------------ */

void inovalleyListen()
{
    rs8706w_weatherlink_data theData;
    rs8706w_weatherlink_ctx theCtx;

    drv_rs8706w_weatherlink_init(&theCtx, 0);

    while (true)
    {
        int theRet = drv_rs8706w_weatherlink_read(&theCtx, 60*1000, &theData);
        if (theRet == SUCCESS)
            ozLog_Debug("WeatherStation: %f degree %lu mm", (float)theData.mTemperatureCelsius/10, theData.mRainfallMillimeters);
        else
            ozLog_Error("WeatherStation error: %d", theRet);
    }
    
    drv_rs8706w_weatherlink_uninit(&theCtx);
}

/* ------------------------------------------------------------------------------------ */

int main(int argc, char* argv[])
{
    ozCommandLineParser     theCommandLineParam(sOptionsList);

    ozLog_SetLogInConsole(1);

    if ( !theCommandLineParam.parse(argc, argv) || theCommandLineParam.hasSwitch("-h") )
    {
        theCommandLineParam.printUsage(argv[0]);
        ozConsole_GetInput();
        return 0;
    }

    if ( theCommandLineParam.hasOption("-dio") )
    {
        uint32 theDIOSender = 0xFF;
        uint16 theDIOInterruptor = 0x01;

        if ( theCommandLineParam.hasOption("-diosender") )
        {
            ozString theSender = theCommandLineParam.getOption("-diosender");
            theSender.toUInt32(&theDIOSender);
        }
        if ( theCommandLineParam.hasOption("-diointerruptor") )
        {
            ozString theInterruptor = theCommandLineParam.getOption("-diointerruptor");
            theInterruptor.toUInt16(&theDIOInterruptor);
        }

        chacon_dio_ctx theCtx;

        drv_chacon_dio_init(&theCtx, 0, ChaconDioMode_Send);
        drv_chacon_dio_send_command(&theCtx, TRUE, theDIOSender, theDIOInterruptor & 0xFF);
        drv_chacon_dio_uninit(&theCtx);
    }
    else if ( theCommandLineParam.hasSwitch("-diolisten") )
    {
        u32             theDIOSender;
        u8              theDIOInterruptor;
        u8              thefON;
        chacon_dio_ctx  theCtx;

        if (drv_chacon_dio_init(&theCtx, 0, ChaconDioMode_Recv) != SUCCESS)
            return -1;

        for (;;)
        {
            if ( drv_chacon_dio_recv_command(&theCtx, &theDIOSender, &theDIOInterruptor, &thefON) )
                ozLog_Debug("Received from %lu to %u -> %s", theDIOSender, theDIOInterruptor, thefON ? "ON" : "OFF");
        }

        drv_chacon_dio_uninit(&theCtx);
    }
    else if ( theCommandLineParam.hasOption("-on") )
    {
        uint32 thePinID;
        ozString thePinStr = theCommandLineParam.getOption("-on");
        thePinStr.toUInt32(&thePinID);

        digital_ctx theDigitalCtx;

        drv_digital_init(&theDigitalCtx, thePinID, PinMode_Output);
        drv_digital_write(&theDigitalCtx, PinLevel_High);
    }
    else if ( theCommandLineParam.hasSwitch("-off") )
    {
        uint32 thePinID;
        ozString thePinStr = theCommandLineParam.getOption("-off");
        thePinStr.toUInt32(&thePinID);

        digital_ctx theDigitalCtx;

        drv_digital_init(&theDigitalCtx, thePinID, PinMode_Output);
        drv_digital_write(&theDigitalCtx, PinLevel_Low);
    }
    else if ( theCommandLineParam.hasSwitch("-buttonmatrix") )
    {
        matrixbutton();
    }
    else if ( theCommandLineParam.hasOption("-lcd") )
    {
        int    thefBacklight = 1;
        ozString theLCDStr = theCommandLineParam.getOption("-lcd");
         if ( theCommandLineParam.hasOption("-lcdbacklight") )
            thefBacklight = theCommandLineParam.getOption("-lcdbacklight") != "0";
            
        lcd(theLCDStr, thefBacklight);
    }
    else if ( theCommandLineParam.hasOption("-cc1101send") )
    {
        ozString theCC1101PacketStr = theCommandLineParam.getOption("-rfsend");
        cc1101Send(theCC1101PacketStr);
    }
    else if ( theCommandLineParam.hasSwitch("-cc1101listen") )
    {
        cc1101Listen();
    }
    else if ( theCommandLineParam.hasSwitch("-inovalley") )
    {
        inovalleyListen();
    }

    return 0;
}
