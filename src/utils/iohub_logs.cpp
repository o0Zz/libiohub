#include "utils/iohub_logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static iohub_log_callback_t g_callback = NULL;
static void *g_user_data = NULL;

void iohub_logger_init(iohub_log_callback_t callback, void *userdata)
{
    g_callback = callback;
    g_user_data = userdata;
}

#ifdef HAVE_VSNPRINTF

void iohub_log(iohub_log_level_t level, const char *fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (g_callback) {
        g_callback(level, buffer, g_user_data);
    } else {
        const char *lvl_str = "UNKNOWN";
        switch (level) {
            case IOHUB_LOG_DEBUG:   lvl_str = "DEBUG"; break;
            case IOHUB_LOG_INFO:    lvl_str = "INFO"; break;
            case IOHUB_LOG_WARNING: lvl_str = "WARNING"; break;
            case IOHUB_LOG_ERROR:   lvl_str = "ERROR"; break;
        }
        fprintf(stdout, "[%s] %s\n", lvl_str, buffer);
    }
}

void iohub_log_buffer(iohub_log_level_t level, const u8 *buffer, u16 size)
{
    char line[128];
    for (u16 i = 0; i < size; i += 16) {
        int len = snprintf(line, sizeof(line), "%04X: ", i);
        for (u16 j = 0; j < 16 && (i + j) < size; ++j) {
            len += snprintf(line + len, sizeof(line) - len, "%02X ", buffer[i + j]);
        }
        iohub_log(level, "%s", line);
    }
}

#else

void iohub_log(iohub_log_level_t level, const char *fmt, ...)
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
    IOHUB_LOG_DEBUG("\n"); 
}

#endif

void iohub_log_assert(const char *func, int line, const char *expr)
{
	iohub_log(IOHUB_LOG_ERROR, "Assertion failed in %s at line %d: %s", func, line, expr);
	// Optionally, you could add code here to halt execution or trigger a breakpoint.
	abort(); // Terminate the program
}