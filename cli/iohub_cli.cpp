#include "power/iohub_chacon_dio.h"
#include "lcd/iohub_hd44780_lcd.h"
#include "sensor/iohub_rs8706w_weatherlink.h"
#include "heater/iohub_cc1101.h"

#define CC1101_SAUTER
#include "iohub_cc1101_default_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------------------------
                                        LCD
   -------------------------------------------------------------------------------------------- */

static void lcd_write(const char *text, int backlight)
{
    hd44780_lcd_ctx lcd;
    iohub_hd44780_lcd_init(&lcd, 0x27);
    iohub_hd44780_lcd_set_backlight(&lcd, backlight);

    // split lines by "\n"
    const char *sep = strstr(text, "\\n");
    if (sep) {
        size_t len1 = sep - text;
        char line1[64] = {0}, line2[64] = {0};
        strncpy(line1, text, len1);
        strncpy(line2, sep + 2, sizeof(line2) - 1);

        iohub_hd44780_lcd_set_cursor(&lcd, 0, 1);
        iohub_hd44780_lcd_write_string(&lcd, line1);

        iohub_hd44780_lcd_set_cursor(&lcd, 0, 2);
        iohub_hd44780_lcd_write_string(&lcd, line2);
    } else {
        iohub_hd44780_lcd_set_cursor(&lcd, 0, 1);
        iohub_hd44780_lcd_write_string(&lcd, text);
    }
}

/* --------------------------------------------------------------------------------------------
                                        CC1101
   -------------------------------------------------------------------------------------------- */

static void cc1101_send(void)
{
    cc1101_ctx ctx;
    cc1101_packet_ctx packet;
    unsigned char data[] = {
        0x00, 0x20, 0x5a, 0xb5, 0x07, 0x52, 0x00, 0x00, 0x00,
        0x54, 0x00, 0x00, 0x00, 0x62, 0x00, 0x66, 0x02, 0x70,
        0x00, 0x00, 0x00, 0x08, 0x0b, 0xbb, 0x77
    };

    if (iohub_cc1101_init(&ctx, 0, 1, sCC1101_Default_Config) != SUCCESS) {
        fprintf(stderr, "CC1101 init failed\n");
        return;
    }

    for (;;) {
        memcpy(packet.mData, data, sizeof(data));
        packet.mLength = sizeof(data);
        int ret = iohub_cc1101_send(&ctx, &packet);
        if (ret != SUCCESS)
            fprintf(stderr, "Error while sending packet: %d\n", ret);
        sleep(2);
    }
}

static void cc1101_listen(void)
{
    cc1101_ctx ctx;
    cc1101_packet_ctx packet;

    if (iohub_cc1101_init(&ctx, 0, 1, sCC1101_Default_Config) != SUCCESS) {
        fprintf(stderr, "CC1101 init failed\n");
        return;
    }

    for (;;) {
        if (iohub_cc1101_receive(&ctx, &packet) > 0) {
            printf("Packet received (%d bytes): ", packet.mLength);
            for (int i = 0; i < packet.mLength; ++i)
                printf("%02X ", packet.mData[i]);
            printf("\n");
        }
    }
}

/* --------------------------------------------------------------------------------------------
                                   INOVALLEY WEATHER STATION
   -------------------------------------------------------------------------------------------- */

static void inovalley_listen(void)
{
    rs8706w_weatherlink_data data;
    rs8706w_weatherlink_ctx ctx;

    if (iohub_rs8706w_weatherlink_init(&ctx, 0) != SUCCESS) {
        fprintf(stderr, "Weatherlink init failed\n");
        return;
    }

    for (;;) {
        int ret = iohub_rs8706w_weatherlink_read(&ctx, 60 * 1000, &data);
        if (ret == SUCCESS) {
            printf("WeatherStation: %.1f°C %lu mm\n",
                   data.mTemperatureCelsius / 10.0f,
                   data.mRainfallMillimeters);
        } else {
            fprintf(stderr, "WeatherStation error: %d\n", ret);
        }
    }
}

/* --------------------------------------------------------------------------------------------
                                        DIO
   -------------------------------------------------------------------------------------------- */

static void dio_send(unsigned long sender, unsigned short interruptor)
{
    chacon_dio_ctx ctx;
    iohub_chacon_dio_init(&ctx, 0, ChaconDioMode_Send);
    iohub_chacon_dio_send_command(&ctx, 1, sender, interruptor & 0xFF);
    iohub_chacon_dio_uninit(&ctx);
}

static void dio_listen(void)
{
    chacon_dio_ctx ctx;
    if (iohub_chacon_dio_init(&ctx, 0, ChaconDioMode_Recv) != SUCCESS) {
        fprintf(stderr, "DIO init failed\n");
        return;
    }

    for (;;) {
        unsigned long sender;
        unsigned char interruptor;
        unsigned char on;
        if (iohub_chacon_dio_recv_command(&ctx, &sender, &interruptor, &on)) {
            printf("Received from %lu to %u -> %s\n",
                   sender, interruptor, on ? "ON" : "OFF");
        }
    }
}

/* --------------------------------------------------------------------------------------------
                                        MAIN
   -------------------------------------------------------------------------------------------- */

static void print_help(const char *prog)
{
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -on <pin>              Set pin HIGH\n");
    printf("  -off <pin>             Set pin LOW\n");
    printf("  -lcd <line1\\nline2>    Write to LCD (2 lines)\n");
    printf("  -lcdbacklight <0|1>    Enable/disable LCD backlight\n");
    printf("  -cc1101send            Send CC1101 test packet\n");
    printf("  -cc1101listen          Listen for CC1101 packets\n");
    printf("  -dio <on|off>          Control DIO output\n");
    printf("  -diosender <id>        Specify sender ID\n");
    printf("  -diointerruptor <id>   Specify interruptor ID\n");
    printf("  -diolisten             Listen to DIO commands\n");
    printf("  -inovalley             Listen to weather station data\n");
    printf("  -h                     Show this help\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_help(argv[0]);
        return 0;
    }

    int lcd_backlight = 1;
    unsigned long dio_sender = 0xFF;
    unsigned short dio_interruptor = 0x01;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-on") == 0 && i + 1 < argc) {
            unsigned pin = (unsigned)strtoul(argv[++i], NULL, 0);
            digital_ctx ctx;
            iohub_digital_init(&ctx, pin, PinMode_Output);
            iohub_digital_write(&ctx, PinLevel_High);
        } else if (strcmp(argv[i], "-off") == 0 && i + 1 < argc) {
            unsigned pin = (unsigned)strtoul(argv[++i], NULL, 0);
            digital_ctx ctx;
            iohub_digital_init(&ctx, pin, PinMode_Output);
            iohub_digital_write(&ctx, PinLevel_Low);
        } else if (strcmp(argv[i], "-lcd") == 0 && i + 1 < argc) {
            const char *text = argv[++i];
            if (i + 1 < argc && strcmp(argv[i + 1], "-lcdbacklight") == 0)
                lcd_backlight = atoi(argv[i + 2]);
            lcd_write(text, lcd_backlight);
        } else if (strcmp(argv[i], "-cc1101send") == 0) {
            cc1101_send();
        } else if (strcmp(argv[i], "-cc1101listen") == 0) {
            cc1101_listen();
        } else if (strcmp(argv[i], "-dio") == 0 && i + 1 < argc) {
            const char *state = argv[++i];
            int on = (strcmp(state, "on") == 0);
            dio_send(dio_sender, dio_interruptor);
            printf("Sent DIO %s\n", on ? "ON" : "OFF");
        } else if (strcmp(argv[i], "-diosender") == 0 && i + 1 < argc) {
            dio_sender = strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-diointerruptor") == 0 && i + 1 < argc) {
            dio_interruptor = (unsigned short)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-diolisten") == 0) {
            dio_listen();
        } else if (strcmp(argv[i], "-inovalley") == 0) {
            inovalley_listen();
        }
    }

    return 0;
}