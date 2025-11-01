#include <stdarg.h>
#include <Arduino.h>

extern "C" void log_debug(const char *str, ...)
{
	int i, theCount=0;
	va_list argv;
	
	for(i=0; str[i] != '\0'; i++)
		if(str[i] == '%')
			theCount++;

	va_start(argv, str);

	for(i=0; str[i] != '\0'; i++)
	{
		if(str[i]=='%')
		{
			switch(str[++i])
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
						char theChar = str[++i];
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
					Serial.print(str[i]);
					break;
			};
		}
		else 
		{
			Serial.print(str[i]);
		}
	}
	
	va_end(argv);
	//Serial.println(); //Print trailing newline
}