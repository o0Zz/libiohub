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

static const char *TAG = "iohub_uart";

typedef struct {
    uart_port_t uart_num;
    u8 tx_pin;
    u8 rx_pin;
    u32 baudrate;
    u16 mode;
    BOOL initialized;
    QueueHandle_t uart_queue;
} esp32_uart_ctx_t;

/* Static context - since uart_ctx doesn't have space for a pointer */
static esp32_uart_ctx_t s_uart_ctx = {0};

#define LOG_UART_DEBUG(...)				ESP_LOGD(TAG, __VA_ARGS__)

/* Helper function to convert iohub UART mode to ESP-IDF configuration */
static esp_err_t convert_uart_mode(u16 mode, uart_config_t *uart_config)
{
    // Default configuration
    uart_config->parity = UART_PARITY_DISABLE;
    uart_config->stop_bits = UART_STOP_BITS_1;
    uart_config->flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config->rx_flow_ctrl_thresh = 122;
    uart_config->source_clk = UART_SCLK_DEFAULT;

    // Parse mode (format: XYZ where X=data bits, Y=parity, Z=stop bits)
    u8 data_bits = mode / 100;
    u8 parity = (mode % 100) / 10;
    u8 stop_bits = mode % 10;

    // Set data bits
    switch (data_bits) {
        case 5: uart_config->data_bits = UART_DATA_5_BITS; break;
        case 6: uart_config->data_bits = UART_DATA_6_BITS; break;
        case 7: uart_config->data_bits = UART_DATA_7_BITS; break;
        case 8: uart_config->data_bits = UART_DATA_8_BITS; break;
        default:
            ESP_LOGE(TAG, "Unsupported data bits: %d", data_bits);
            return ESP_ERR_INVALID_ARG;
    }

    // Set parity
    switch (parity) {
        case 0: uart_config->parity = UART_PARITY_ODD; break;   // O = Odd
        case 1: uart_config->parity = UART_PARITY_DISABLE; break; // N = None
        case 2: uart_config->parity = UART_PARITY_EVEN; break;  // E = Even
        default:
            ESP_LOGE(TAG, "Unsupported parity: %d", parity);
            return ESP_ERR_INVALID_ARG;
    }

    // Set stop bits
    switch (stop_bits) {
        case 1: uart_config->stop_bits = UART_STOP_BITS_1; break;
        case 2: uart_config->stop_bits = UART_STOP_BITS_2; break;
        default:
            ESP_LOGE(TAG, "Unsupported stop bits: %d", stop_bits);
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

ret_code_t iohub_uart_init(uart_ctx *aCtx, u8 aTxPin, u8 aRxPin, u32 aBaudrate, u16 aMode)
{
    memset(aCtx, 0x00, sizeof(uart_ctx));
    memset(&s_uart_ctx, 0x00, sizeof(esp32_uart_ctx_t));

    s_uart_ctx.uart_num = ESP_UART_NUM;
    s_uart_ctx.tx_pin = aTxPin;
    s_uart_ctx.rx_pin = aRxPin;
    s_uart_ctx.baudrate = aBaudrate;
    s_uart_ctx.mode = aMode;

    // Configure UART parameters
    uart_config_t uart_config;
    uart_config.baud_rate = aBaudrate;
    
    esp_err_t ret = convert_uart_mode(aMode, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to convert UART mode: %d", aMode);
        return E_INVALID_PARAMETERS;
    }

    // Apply UART configuration
    ret = uart_param_config(s_uart_ctx.uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return E_DEVICE_INIT_FAILED;
    }

    // Set UART pins
    gpio_num_t tx_io = (aTxPin == UART_NOT_USED_PIN) ? UART_PIN_NO_CHANGE : (gpio_num_t)aTxPin;
    gpio_num_t rx_io = (aRxPin == UART_NOT_USED_PIN) ? UART_PIN_NO_CHANGE : (gpio_num_t)aRxPin;
    
    ret = uart_set_pin(s_uart_ctx.uart_num, tx_io, rx_io, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return E_FAIL_SET_PIN;
    }

    // Install UART driver
    ret = uart_driver_install(s_uart_ctx.uart_num, ESP_UART_BUF_SIZE, ESP_UART_BUF_SIZE, 
                             ESP_UART_QUEUE_SIZE, &s_uart_ctx.uart_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return E_DEVICE_INIT_FAILED;
    }

    s_uart_ctx.initialized = TRUE;

    LOG_UART_DEBUG("UART initialized (TX=%d RX=%d Baud=%d Mode=%d)", aTxPin, aRxPin, aBaudrate, aMode);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

u16 iohub_uart_data_available(uart_ctx *aCtx)
{
    if (!s_uart_ctx.initialized) {
        return 0;
    }

    size_t buffered_size = 0;
    esp_err_t ret = uart_get_buffered_data_len(s_uart_ctx.uart_num, &buffered_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get buffered data length: %s", esp_err_to_name(ret));
        return 0;
    }

    return (u16)buffered_size;
}

/* ------------------------------------------------------------- */

u8 iohub_uart_read_byte(uart_ctx *aCtx)
{
    if (!s_uart_ctx.initialized) {
        return 0;
    }

    u8 data = 0;
    int len = uart_read_bytes(s_uart_ctx.uart_num, &data, 1, 0); // Non-blocking read
    if (len <= 0) {
        return 0;
    }

    return data;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_read(uart_ctx *aCtx, u8 *aBuffer, u16 *aSize)
{
    if (!s_uart_ctx.initialized || !aBuffer || !aSize) {
        if (aSize) *aSize = 0;
        return E_INVALID_PARAMETERS;
    }

    u16 theIdx = 0;
    u16 max_size = *aSize;

    // Check available data
    size_t available = 0;
    esp_err_t ret = uart_get_buffered_data_len(s_uart_ctx.uart_num, &available);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get buffered data length: %s", esp_err_to_name(ret));
        *aSize = 0;
        return E_READ_ERROR;
    }

    if (available > 0) {
        // Read available data, but not more than requested
        u16 to_read = (available > max_size) ? max_size : (u16)available;
        int len = uart_read_bytes(s_uart_ctx.uart_num, aBuffer, to_read, 0);
        if (len > 0) {
            theIdx = (u16)len;
        }
    }

    *aSize = theIdx;
    return SUCCESS;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_uart_write(uart_ctx *aCtx, u8 *aBuffer, u16 aSize)
{
    if (!s_uart_ctx.initialized || !aBuffer || aSize == 0) {
        return E_INVALID_PARAMETERS;
    }

    int len = uart_write_bytes(s_uart_ctx.uart_num, (const char*)aBuffer, aSize);
    if (len != aSize) {
        ESP_LOGE(TAG, "Failed to write all bytes: wrote %d of %d", len, aSize);
        return E_WRITE_ERROR;
    }

    // Wait for transmission to complete
    esp_err_t ret = uart_wait_tx_done(s_uart_ctx.uart_num, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Write timeout: %s", esp_err_to_name(ret));
        // Don't return error for timeout, data was queued successfully
    }

    return SUCCESS;
}

/* ------------------------------------------------------------- */

void iohub_uart_close(uart_ctx *aCtx)
{
    if (!s_uart_ctx.initialized) {
        return;
    }

    uart_driver_delete(s_uart_ctx.uart_num);
    s_uart_ctx.initialized = FALSE;
    s_uart_ctx.uart_queue = NULL;

    LOG_UART_DEBUG("UART closed");
}