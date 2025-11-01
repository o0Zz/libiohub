
#pragma once

#include <Arduino.h>

#define INLINE												inline

INLINE void	iohub_platform_init()
{
	//Nothing to do
}

	//https://www.arduino.cc/en/Reference/PortManipulation
#define iohub_digital_read_0									(PIND & 0x01)
#define iohub_digital_read_1									(PIND & 0x02)
#define iohub_digital_read_2									(PIND & 0x04)
#define iohub_digital_read_3									(PIND & 0x08)
#define iohub_digital_read_4									(PIND & 0x10)
#define iohub_digital_read_5									(PIND & 0x20)
#define iohub_digital_read_6									(PIND & 0x40)
#define iohub_digital_read_7									(PIND & 0x80)
#define iohub_digital_read_8									(PINB & 0x01)
#define iohub_digital_read_9									(PINB & 0x02)
#define iohub_digital_read_10									(PINB & 0x04)
#define iohub_digital_read_11									(PINB & 0x08)
#define iohub_digital_read_12									(PINB & 0x10)
#define iohub_digital_read_13									(PINB & 0x20)

//Will be optimized by compiler when aValue is a constant
#define iohub_digital_write_0(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 0); else BIT_CLEAR(PORTD, 0);} while(0)
#define iohub_digital_write_1(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 1); else BIT_CLEAR(PORTD, 1);} while(0)
#define iohub_digital_write_2(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 2); else BIT_CLEAR(PORTD, 2);} while(0)
#define iohub_digital_write_3(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 3); else BIT_CLEAR(PORTD, 3);} while(0)
#define iohub_digital_write_4(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 4); else BIT_CLEAR(PORTD, 4);} while(0)
#define iohub_digital_write_5(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 5); else BIT_CLEAR(PORTD, 5);} while(0)

#define iohub_digital_set_pin_mode(aPin, aPinMode)				pinMode(aPin, (aPinMode == PinMode_Input) ? INPUT : OUTPUT)
#define iohub_digital_write(aPin, aPinLevel)					digitalWrite(aPin, (aPinLevel == PinLevel_High) ? HIGH : LOW)
#define iohub_digital_read(aPin)								(digitalRead(aPin) ? PinLevel_High : PinLevel_Low)

#define iohub_time_delay_ms(anMSDelay)							delay(anMSDelay)
#define iohub_time_delay_us(anUSDelay)							delayMicroseconds(anUSDelay)
#define iohub_time_now_us()										micros() //Rollover in 70 minutes
#define iohub_time_now_ms()										millis() //Rollover in 50 days

#define iohub_interrupts_disable()								noInterrupts()
#define iohub_interrupts_enable()								interrupts()
