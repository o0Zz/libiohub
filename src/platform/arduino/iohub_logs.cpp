#include <stdarg.h>
#include <Arduino.h>

extern "C" void iohub_log(iohub_log_level_t level, const char *fmt, ...)
{
	int i, theCount=0;
	va_list argv;

	for(i=0; fmt[i] != '\0'; i++)
		if(fmt[i] == '%')
			theCount++;

	va_start(argv, fmt);

	for(i=0; fmt[i] != '\0'; i++)
	{
		if(fmt[i]=='%')
		{
			switch(fmt[++i])
			{
				case 'u': 
					Serial.print(va_arg(argv, unsigned int));
					break;
					
				case 'd': 
					Serial.print(va_arg(argv, int));
					break;
					
				case 'X':
				case 'x': 
					Serial.print(va_arg(argv, int), HEX);
					break;
					
				case 'l': 
					{
						char theChar = fmt[++i];
						if (theChar == 'u')
						{
							Serial.print(va_arg(argv, unsigned long));
						}
						else if (theChar == 'x' || theChar == 'X' )
						{
							Serial.print(va_arg(argv, unsigned long), HEX);
						}
						else if (theChar == 'f')
						{
							Serial.print(va_arg(argv, double));
						}
						else
						{
							i--;
							Serial.print(va_arg(argv, long));
						}
					}
					break;
					
				case 'f': 
					Serial.print(va_arg(argv, double));
					break;
					
				case 'c': 
					Serial.print((char)va_arg(argv, int));
					break;
					
				case 's':
					Serial.print(va_arg(argv, char *));
					break;
					
				default:
					Serial.print('%');
					Serial.print(fmt[i]);
					break;
			};
		}
		else 
		{
			Serial.print(fmt[i]);
		}
	}
	
	va_end(argv);
}

void iohub_log_buffer(iohub_log_level_t level, const u8 *buffer, u16 size)
{
    for (int i=0; i<size; i++) 
        IOHUB_LOG_DEBUG("0x%x ", ((u8)buffer[i])); 
    IOHUB_LOG_DEBUG(""); 
}