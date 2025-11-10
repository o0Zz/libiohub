#include "platform/iohub_spi.h"
#include "platform/iohub_platform.h"
#include "utils/iohub_logs.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <string.h>

/* SPI Configuration for ESP32-C6 */
#define MOSI_PIN				23    /*!< GPIO number for SPI MOSI */
#define MISO_PIN				19    /*!< GPIO number for SPI MISO */
#define SCK_PIN					18    /*!< GPIO number for SPI CLK */
#define SPI_HOST_ID				SPI2_HOST
#define SPI_CLOCK_SPEED_HZ		1000000  /*!< 1 MHz default clock speed */

typedef struct {
    spi_device_handle_t spi_handle;
    spi_bus_config_t bus_config;
    spi_device_interface_config_t dev_config;
    BOOL bus_initialized;
    BOOL device_added;
    u32 cs_pin;
    IOHubSPIMode mode;
} esp32_spi_ctx_t;

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_init(spi_ctx *aCtx, u32 aCSnPin, IOHubSPIMode aMode)
{
    memset(aCtx, 0x00, sizeof(spi_ctx));

    esp32_spi_ctx_t *esp_ctx = (esp32_spi_ctx_t*)malloc(sizeof(esp32_spi_ctx_t));
    if (!esp_ctx) {
        IOHUB_LOG_ERROR("Failed to allocate memory for SPI context");
        return E_DEVICE_INIT_FAILED;
    }
    
    memset(esp_ctx, 0, sizeof(esp32_spi_ctx_t));
    esp_ctx->cs_pin = aCSnPin;
    esp_ctx->mode = aMode;

    // Configure SPI bus
    esp_ctx->bus_config.mosi_io_num = MOSI_PIN;
    esp_ctx->bus_config.miso_io_num = MISO_PIN;
    esp_ctx->bus_config.sclk_io_num = SCK_PIN;
    esp_ctx->bus_config.quadwp_io_num = -1;
    esp_ctx->bus_config.quadhd_io_num = -1;
    esp_ctx->bus_config.max_transfer_sz = 4092;

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &esp_ctx->bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        free(esp_ctx);
        return E_DEVICE_INIT_FAILED;
    }
    esp_ctx->bus_initialized = TRUE;

    // Configure SPI device
    esp_ctx->dev_config.clock_speed_hz = SPI_CLOCK_SPEED_HZ;
    esp_ctx->dev_config.mode = 0;  // SPI mode 0
    esp_ctx->dev_config.spics_io_num = -1; // We'll handle CS manually
    esp_ctx->dev_config.queue_size = 7;
    esp_ctx->dev_config.pre_cb = NULL;
    esp_ctx->dev_config.post_cb = NULL;

    // Add device to SPI bus
    ret = spi_bus_add_device(SPI_HOST_ID, &esp_ctx->dev_config, &esp_ctx->spi_handle);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(SPI_HOST_ID);
        free(esp_ctx);
        return E_DEVICE_INIT_FAILED;
    }
    esp_ctx->device_added = TRUE;

    // Configure CS pin as output
    iohub_digital_set_pin_mode(aCSnPin, PinMode_Output);
    iohub_digital_write(aCSnPin, PinLevel_High); // Deselect

    aCtx->mCSnPin = aCSnPin;
    aCtx->mMode = aMode;
    aCtx->mCtx = esp_ctx;

    IOHUB_LOG_INFO("SPI initialized successfully (CS: %d)", aCSnPin);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

void iohub_spi_uninit(spi_ctx *aCtx)
{
    if (!aCtx || !aCtx->mCtx) {
        return;
    }

    esp32_spi_ctx_t *esp_ctx = (esp32_spi_ctx_t*)aCtx->mCtx;

    if (esp_ctx->device_added) {
        spi_bus_remove_device(esp_ctx->spi_handle);
        esp_ctx->device_added = FALSE;
    }

    if (esp_ctx->bus_initialized) {
        spi_bus_free(SPI_HOST_ID);
        esp_ctx->bus_initialized = FALSE;
    }

    free(esp_ctx);
    aCtx->mCtx = NULL;

    IOHUB_LOG_INFO("SPI uninitialized");
}

/* ------------------------------------------------------------- */

void iohub_spi_select(spi_ctx *aCtx)
{
    IOHUB_ASSERT(aCtx->mSelectedCount < 255);

    if (aCtx->mSelectedCount++ == 0) {
        iohub_digital_write(aCtx->mCSnPin, PinLevel_Low);
        
        if (aCtx->mMode & SPIMode_WaitForMisoLowAfterSelect) {
            while (iohub_digital_read(MISO_PIN) != PinLevel_Low);
        }
    }
}

/* ------------------------------------------------------------- */

void iohub_spi_deselect(spi_ctx *aCtx)
{
    if (aCtx->mSelectedCount == 0)
        return;
    
    if (--aCtx->mSelectedCount == 0)
        iohub_digital_write(aCtx->mCSnPin, PinLevel_High);
}

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_transfer(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen)
{
    if (!aCtx || !aCtx->mCtx || !aBuffer || aBufferLen == 0) {
        return E_INVALID_PARAMETERS;
    }

    esp32_spi_ctx_t *esp_ctx = (esp32_spi_ctx_t*)aCtx->mCtx;

    iohub_spi_select(aCtx);
    
    IOHUB_LOG_DEBUG("SPI Tx:");
    IOHUB_LOG_BUFFER(aBuffer, aBufferLen);
    
    // Prepare SPI transaction
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = aBufferLen * 8; // Length in bits
    trans.tx_buffer = aBuffer;
    trans.rx_buffer = aBuffer; // Full duplex - receive into same buffer
    
    esp_err_t ret = spi_device_transmit(esp_ctx->spi_handle, &trans);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("SPI transfer failed: %s", esp_err_to_name(ret));
        iohub_spi_deselect(aCtx);
        return E_WRITE_ERROR;
    }

    IOHUB_LOG_DEBUG("SPI Rx:");
    IOHUB_LOG_BUFFER(aBuffer, aBufferLen);
    
    iohub_spi_deselect(aCtx);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

u8 iohub_spi_transfer_byte(spi_ctx *aCtx, u8 aByte)
{
    u8 theByte = aByte;
    
    if (iohub_spi_transfer(aCtx, &theByte, sizeof(theByte)) == SUCCESS)
        return theByte;

    return 0x00;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_write(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen)
{
    if (!aCtx || !aCtx->mCtx || !aBuffer || aBufferLen == 0) {
        return E_INVALID_PARAMETERS;
    }

    esp32_spi_ctx_t *esp_ctx = (esp32_spi_ctx_t*)aCtx->mCtx;

    iohub_spi_select(aCtx);

    IOHUB_LOG_DEBUG("SPI Write:");
    IOHUB_LOG_BUFFER(aBuffer, aBufferLen);
    
    // Prepare SPI transaction for write-only
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = aBufferLen * 8; // Length in bits
    trans.tx_buffer = aBuffer;
    trans.rx_buffer = NULL; // Write only
    
    esp_err_t ret = spi_device_transmit(esp_ctx->spi_handle, &trans);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("SPI write failed: %s", esp_err_to_name(ret));
        iohub_spi_deselect(aCtx);
        return E_WRITE_ERROR;
    }
    
    iohub_spi_deselect(aCtx);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_write_byte(spi_ctx *aCtx, u8 aByte)
{
    return iohub_spi_write(aCtx, &aByte, 1);
}

/* ------------------------------------------------------------- */

ret_code_t iohub_spi_read(spi_ctx *aCtx, u8 *aBuffer, u16 aBufferLen)
{
    if (!aCtx || !aCtx->mCtx || !aBuffer || aBufferLen == 0) {
        return E_INVALID_PARAMETERS;
    }

    esp32_spi_ctx_t *esp_ctx = (esp32_spi_ctx_t*)aCtx->mCtx;

    iohub_spi_select(aCtx);
    
    // Prepare SPI transaction for read-only
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = aBufferLen * 8; // Length in bits
    trans.tx_buffer = NULL; // Read only
    trans.rx_buffer = aBuffer;
    
    esp_err_t ret = spi_device_transmit(esp_ctx->spi_handle, &trans);
    if (ret != ESP_OK) {
        IOHUB_LOG_ERROR("SPI read failed: %s", esp_err_to_name(ret));
        iohub_spi_deselect(aCtx);
        return E_READ_ERROR;
    }
    
    IOHUB_LOG_DEBUG("SPI Read:");
    IOHUB_LOG_BUFFER(aBuffer, aBufferLen);
    
    iohub_spi_deselect(aCtx);
    return SUCCESS;
}

/* ------------------------------------------------------------- */

u8 iohub_spi_read_byte(spi_ctx *aCtx)
{
    u8 byte = 0;
    
    if (iohub_spi_read(aCtx, &byte, 1) == SUCCESS)
        return byte;

    return 0x00;
}
