
#pragma once

#include <Arduino.h>

static inline void iohub_platform_init()
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
#define iohub_digital_write_0(aValue)							do {if (aValue == PinLevel_High) IOHUB_GPIO_BIT_SET(PORTD, 0); else IOHUB_GPIO_BIT_CLEAR(PORTD, 0);} while(0)
#define iohub_digital_write_1(aValue)							do {if (aValue == PinLevel_High) IOHUB_GPIO_BIT_SET(PORTD, 1); else IOHUB_GPIO_BIT_CLEAR(PORTD, 1);} while(0)
#define iohub_digital_write_2(aValue)							do {if (aValue == PinLevel_High) IOHUB_GPIO_BIT_SET(PORTD, 2); else IOHUB_GPIO_BIT_CLEAR(PORTD, 2);} while(0)
#define iohub_digital_write_3(aValue)							do {if (aValue == PinLevel_High) IOHUB_GPIO_BIT_SET(PORTD, 3); else IOHUB_GPIO_BIT_CLEAR(PORTD, 3);} while(0)
#define iohub_digital_write_4(aValue)							do {if (aValue == PinLevel_High) IOHUB_GPIO_BIT_SET(PORTD, 4); else IOHUB_GPIO_BIT_CLEAR(PORTD, 4);} while(0)
#define iohub_digital_write_5(aValue)							do {if (aValue == PinLevel_High) IOHUB_GPIO_BIT_SET(PORTD, 5); else IOHUB_GPIO_BIT_CLEAR(PORTD, 5);} while(0)

#define iohub_digital_set_pin_mode(aPin, aPinMode)				pinMode(aPin, (aPinMode == PinMode_Input) ? INPUT : OUTPUT)
#define iohub_digital_write(aPin, aPinLevel)					digitalWrite(aPin, (aPinLevel == PinLevel_High) ? HIGH : LOW)
#define iohub_digital_read(aPin)								(digitalRead(aPin) ? PinLevel_High : PinLevel_Low)

#define iohub_time_delay_ms(anMSDelay)							delay(anMSDelay)
#define iohub_time_delay_us(anUSDelay)							delayMicroseconds(anUSDelay)
#define iohub_time_now_us()										micros() //Rollover in 70 minutes
#define iohub_time_now_ms()										millis() //Rollover in 50 days

#define iohub_interrupts_disable()								noInterrupts()
#define iohub_interrupts_enable()								interrupts()

#define IOHUB_INT_TYPE_CHANGE CHANGE
#define IOHUB_INT_TYPE_FALLING FALLING
#define IOHUB_INT_TYPE_RISING RISING

static inline ret_code_t iohub_detach_interrupt(u8 pin)
{
    detachInterrupt(digitalPinToInterrupt(pin));
    return SUCCESS;
}

ret_code_t iohub_attach_interrupt(u8 pin, gpio_isr_t isr_handler, gpio_int_type_t intr_type, void* arg)
{
	if (pin != 2 && pin != 3)
		IOHUB_LOG_ERROR("PIN: %d, not valid for interrupt !", pin);

	attachInterrupt(digitalPinToInterrupt(pin), isr_handler, intr_type);
	return SUCCESS;
}