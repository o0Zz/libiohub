#	include <Arduino.h>

#	define INLINE												inline

	//https://www.arduino.cc/en/Reference/PortManipulation
#	define drv_digital_read_0									(PIND & 0x01)
#	define drv_digital_read_1									(PIND & 0x02)
#	define drv_digital_read_2									(PIND & 0x04)
#	define drv_digital_read_3									(PIND & 0x08)
#	define drv_digital_read_4									(PIND & 0x10)
#	define drv_digital_read_5									(PIND & 0x20)
#	define drv_digital_read_6									(PIND & 0x40)
#	define drv_digital_read_7									(PIND & 0x80)
#	define drv_digital_read_8									(PINB & 0x01)
#	define drv_digital_read_9									(PINB & 0x02)
#	define drv_digital_read_10									(PINB & 0x04)
#	define drv_digital_read_11									(PINB & 0x08)
#	define drv_digital_read_12									(PINB & 0x10)
#	define drv_digital_read_13									(PINB & 0x20)

	//Will be optimized by compiler when aValue is a constant
#	define drv_digital_write_0(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 0); else BIT_CLEAR(PORTD, 0);} while(0)
#	define drv_digital_write_1(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 1); else BIT_CLEAR(PORTD, 1);} while(0)
#	define drv_digital_write_2(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 2); else BIT_CLEAR(PORTD, 2);} while(0)
#	define drv_digital_write_3(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 3); else BIT_CLEAR(PORTD, 3);} while(0)
#	define drv_digital_write_4(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 4); else BIT_CLEAR(PORTD, 4);} while(0)
#	define drv_digital_write_5(aValue)							do {if (aValue == PinLevel_High) BIT_SET(PORTD, 5); else BIT_CLEAR(PORTD, 5);} while(0)

#	define drv_digital_set_pin_mode(aPin, aPinMode)				pinMode(aPin, (aPinMode == PinMode_Input) ? INPUT : OUTPUT)
#	define drv_digital_write(aPin, aPinLevel)					digitalWrite(aPin, (aPinLevel == PinLevel_High) ? HIGH : LOW)
#	define drv_digital_read(aPin)								(digitalRead(aPin) ? PinLevel_High : PinLevel_Low)

#ifdef __cplusplus
extern "C" {
#endif
	extern void log_debug(const char *str, ...);
#ifdef __cplusplus
}
#endif


#	define log_buffer(aBuffer, aLen)							do{ for (int i=0; i<aLen; i++) log_debug("0x%x ", ((u8)aBuffer[i])); log_debug("\n"); }while(0)
#	define log_assert(aFunction, aLine, anExpr)					do{ log_error("ASSERT: %s:%d\r\n", aFunction, aLine); while(1); }while(0)
#	define log_error											log_debug

#	define time_delay_ms(anMSDelay)								delay(anMSDelay)
#	define time_delay_us(anUSDelay)								delayMicroseconds(anUSDelay)
#	define time_now_us()										micros() //Rollover in 70 minutes
#	define time_now_ms()										millis() //Rollover in 50 days

#	define interrupts_disable()									noInterrupts()
#	define interrupts_enable()									interrupts()
