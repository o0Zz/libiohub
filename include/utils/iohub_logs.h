#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "platform/iohub_platform.h"
#include "utils/iohub_types.h"

#ifndef IOHUB_LOG_DEBUG
#	define IOHUB_LOG_DEBUG(...)				iohub_log_debug(__VA_ARGS__)
#endif

#ifndef IOHUB_LOG_INFO
#	define IOHUB_LOG_INFO(...)				iohub_log_info(__VA_ARGS__)
#endif

#ifndef IOHUB_LOG_WARNING
#	define IOHUB_LOG_WARNING(...)			iohub_log_warning(__VA_ARGS__)
#endif

#ifndef IOHUB_LOG_ERROR
#	define IOHUB_LOG_ERROR(...)			    iohub_log_error(__VA_ARGS__)
#endif

#ifndef IOHUB_LOG_BUFFER
#	define IOHUB_LOG_BUFFER(buffer, size)	iohub_log_buffer(IOHUB_LOGLVL_DEBUG, buffer, size)
#endif

#define IOHUB_ASSERT(xx)          	    do{ if (!(xx)) iohub_log_assert(__FUNCTION__, __LINE__, #xx); }while(0)

typedef enum {
    IOHUB_LOGLVL_DEBUG,
    IOHUB_LOGLVL_INFO,
    IOHUB_LOGLVL_WARNING,
    IOHUB_LOGLVL_ERROR
} iohub_log_level_t;

typedef void (*iohub_log_callback_t)(iohub_log_level_t level, const char *msg, void *userdata);

/**
 * @brief Initialize the logger with a callback.
 * 
 * @param callback Function to be called for each log message.
 * @param userdata Pointer passed to the callback on each call.
 */
void iohub_logger_init(iohub_log_callback_t callback, void *userdata);

/**
 * @brief Generic log function with printf-style formatting.
 * 
 * @param level Log level.
 * @param fmt Format string (printf-style).
 * @param ... Additional arguments.
 */
void iohub_log(iohub_log_level_t level, const char *fmt, ...);

/**
 * @brief Log a buffer in hexadecimal format.
 * 
 * @param level Log level.
 * @param buffer Pointer to the buffer.
 * @param size Size of the buffer in bytes.
 */
void iohub_log_buffer(iohub_log_level_t level, const u8 *buffer, u16 size);

/**
 * @brief Log an assertion failure.
 * 
 * @param func Function name where the assertion failed.
 * @param line Line number where the assertion failed.
 * @param expr The expression that failed.
 */
void iohub_log_assert(const char *func, int line, const char *expr);

/* Convenience macros for each log level */
#define iohub_log_debug(fmt, ...)   iohub_log(IOHUB_LOGLVL_DEBUG, fmt, ##__VA_ARGS__)
#define iohub_log_info(fmt, ...)    iohub_log(IOHUB_LOGLVL_INFO, fmt, ##__VA_ARGS__)
#define iohub_log_warning(fmt, ...) iohub_log(IOHUB_LOGLVL_WARNING, fmt, ##__VA_ARGS__)
#define iohub_log_error(fmt, ...)   iohub_log(IOHUB_LOGLVL_ERROR, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif