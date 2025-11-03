#include <platform/iohub_uart.h>
#include "platform/iohub_platform.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

/* UART Configuration for ESP32-C6 */
#define ESP_UART_NUM				UART_NUM_1    /*!< UART port number to use */
#define ESP_UART_BUF_SIZE			1024          /*!< UART buffer size */
#define ESP_UART_QUEUE_SIZE			10            /*!< UART event queue size */

#define LOG_UART_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)

QueueHandle_t uart_queue;

ret_code_t iohub_uart_init(uart_ctx *aCtx, u8 txPin, u8 rxPin)
{
    memset(aCtx, 0x00, sizeof(uart_ctx));

    // Set UART pins
    gpio_num_t tx_io = (txPin == UART_NOT_USED_PIN) ? UART_PIN_NO_CHANGE : (gpio_num_t)txPin;
    gpio_num_t rx_io = (rxPin == UART_NOT_USED_PIN) ? UART_PIN_NO_CHANGE : (gpio_num_t)rxPin;

    esp_err_t ret = uart_set_pin(ESP_UART_NUM, tx_io, rx_io, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to set UART pins: %s", esp_err_to_name(ret));
        return E_FAIL_SET_PIN;
    }

    // Install UART driver
    ret = uart_driver_install(ESP_UART_NUM, ESP_UART_BUF_SIZE, ESP_UART_BUF_SIZE, 
                             ESP_UART_QUEUE_SIZE, &uart_queue, 0);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to install UART driver: %s", esp_err_to_name(ret));
        return E_DEVICE_INIT_FAILED;
    }

    LOG_UART_DEBUG("UART initialized (TX=%d RX=%d)", txPin, rxPin);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_open(uart_ctx *ctx, u32 baudrate, IOHubUartParity parity, u8 stopBits)
{
    uart_config_t uart_config;   
    uart_config.baud_rate = baudrate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;  // Set to default value
    uart_config.source_clk = UART_SCLK_DEFAULT;

    if (stopBits == 1) {
        uart_config.stop_bits = UART_STOP_BITS_1;
    } else if (stopBits == 2) {
        uart_config.stop_bits = UART_STOP_BITS_2;
    } else {
        IOHUB_LOG_ERROR("Unsupported stop bits: %d", stopBits);
        return E_INVALID_PARAMETERS;
    }

    if (parity == IOHubUartParity_Even) {
        uart_config.parity = UART_PARITY_EVEN;
    } else if (parity == IOHubUartParity_Odd) {
        uart_config.parity = UART_PARITY_ODD;
    }

    esp_err_t ret = uart_param_config(ESP_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to reconfigure UART parameters: %s", esp_err_to_name(ret));
        return E_DEVICE_INIT_FAILED;
    }

    LOG_UART_DEBUG("UART opened (Baud=%d Parity=%d StopBits=%d)", baudrate, parity, stopBits);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

u16 iohub_uart_data_available(uart_ctx *ctx)
{
    size_t buffered_size = 0;
    esp_err_t ret = uart_get_buffered_data_len(ESP_UART_NUM, &buffered_size);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to get buffered data length: %s", esp_err_to_name(ret));
        return 0;
    }

    return (u16)buffered_size;
}

/* ------------------------------------------------------------- */

u8 iohub_uart_read_byte(uart_ctx *ctx)
{
    u8 data = 0;
    int len = uart_read_bytes(ESP_UART_NUM, &data, 1, 0); // Non-blocking read
    if (len <= 0) {
        return 0;
    }

    return data;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_read(uart_ctx *ctx, u8 *buffer, u16 *size)
{
    if (!buffer || !size) {
        if (size) *size = 0;
        return E_INVALID_PARAMETERS;
    }

    u16 theIdx = 0;
    u16 max_size = *size;

    // Check available data
    size_t available = 0;
    esp_err_t ret = uart_get_buffered_data_len(ESP_UART_NUM, &available);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to get buffered data length: %s", esp_err_to_name(ret));
        *size = 0;
        return E_READ_ERROR;
    }

    if (available > 0) {
        // Read available data, but not more than requested
        u16 to_read = (available > max_size) ? max_size : (u16)available;
        int len = uart_read_bytes(ESP_UART_NUM, buffer, to_read, 0);
        if (len > 0) {
            theIdx = (u16)len;
        }
    }

    *size = theIdx;
    return SUCCESS;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_write(uart_ctx *ctx, u8 *buffer, u16 size)
{
    if (!buffer || size == 0) {
        return E_INVALID_PARAMETERS;
    }

    int len = uart_write_bytes(ESP_UART_NUM, (const char*)buffer, size);
    if (len != size) {
        IOHUB_LOG_ERROR("Failed to write all bytes: wrote %d of %d", len, size);
        return E_WRITE_ERROR;
    }

    // Wait for transmission to complete
    esp_err_t ret = uart_wait_tx_done(ESP_UART_NUM, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        IOHUB_LOG_WARNING("UART Write timeout: %s", esp_err_to_name(ret));
        // Don't return error for timeout, data was queued successfully
    }

    return SUCCESS;
}

/* ------------------------------------------------------------- */

void iohub_uart_close(uart_ctx *aCtx)
{
    uart_driver_delete(ESP_UART_NUM);
    LOG_UART_DEBUG("UART closed");
}