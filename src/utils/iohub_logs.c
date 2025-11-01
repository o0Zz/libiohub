#include "utils/iohub_logs.h"
#include "platform/iohub_platform.h"
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

#ifdef IOHUB_LOG_PRINTF

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

#endif

void iohub_log_assert(const char *func, int line, const char *expr)
{
	IOHUB_LOG_ERROR("Assertion failed in %s at line %d: %s", func, line, expr);
	// Optionally, you could add code here to halt execution or trigger a breakpoint.
	abort(); // Terminate the program
}