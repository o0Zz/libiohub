#include "platform/iohub_i2c.h"
#include "platform/iohub_platform.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"

/* I2C Configuration for ESP32-C6 */
#define I2C_MASTER_SCL_IO           6    /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           5    /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0    /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000     /*!< I2C master clock frequency */

static const char *TAG = "iohub_i2c";

typedef struct {
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    u8 device_addr;
    BOOL initialized;
} esp32_i2c_ctx_t;

/* --------------------------------------------------------- */

int iohub_i2c_init(i2c_ctx *aCtx, u8 aI2CDeviceAddr)
{
    memset(aCtx, 0x00, sizeof(i2c_ctx));
    
    esp32_i2c_ctx_t *esp_ctx = (esp32_i2c_ctx_t*)malloc(sizeof(esp32_i2c_ctx_t));
    if (!esp_ctx) {
        ESP_LOGE(TAG, "Failed to allocate memory for I2C context");
        return E_DEVICE_INIT_FAILED;
    }
    
    memset(esp_ctx, 0, sizeof(esp32_i2c_ctx_t));
    esp_ctx->device_addr = aI2CDeviceAddr;
    
    // Configure I2C master bus
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &esp_ctx->bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        free(esp_ctx);
        return E_DEVICE_INIT_FAILED;
    }
    
    // Configure I2C device
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = aI2CDeviceAddr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    ret = i2c_master_bus_add_device(esp_ctx->bus_handle, &dev_cfg, &esp_ctx->dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        i2c_del_master_bus(esp_ctx->bus_handle);
        free(esp_ctx);
        return E_DEVICE_INIT_FAILED;
    }
    
    esp_ctx->initialized = TRUE;
    aCtx->mCtx = esp_ctx;
    aCtx->mI2CDeviceAddr = aI2CDeviceAddr;
    
    ESP_LOGI(TAG, "I2C initialized successfully for device 0x%02X", aI2CDeviceAddr);
    return SUCCESS;
}

/* --------------------------------------------------------- */

void iohub_i2c_uninit(i2c_ctx *aCtx)
{
    if (!aCtx || !aCtx->mCtx) {
        return;
    }
    
    esp32_i2c_ctx_t *esp_ctx = (esp32_i2c_ctx_t*)aCtx->mCtx;
    
    if (esp_ctx->initialized) {
        if (esp_ctx->dev_handle) {
            i2c_master_bus_rm_device(esp_ctx->dev_handle);
        }
        if (esp_ctx->bus_handle) {
            i2c_del_master_bus(esp_ctx->bus_handle);
        }
        esp_ctx->initialized = FALSE;
    }
    
    free(esp_ctx);
    aCtx->mCtx = NULL;
    
    ESP_LOGI(TAG, "I2C uninitialized");
}

/* --------------------------------------------------------- */

ret_code_t iohub_i2c_request_read(i2c_ctx *aCtx, BOOL afIssueStop)
{
    // This function is used by some platforms to prepare for reading
    // For ESP-IDF, the read operation is atomic, so this is a no-op
    if (!aCtx || !aCtx->mCtx) {
        return E_INVALID_PARAMETERS;
    }
    
    esp32_i2c_ctx_t *esp_ctx = (esp32_i2c_ctx_t*)aCtx->mCtx;
    if (!esp_ctx->initialized) {
        return E_INVALID_STATE;
    }
    
    return SUCCESS;
}

/* --------------------------------------------------------- */

ret_code_t iohub_i2c_read(i2c_ctx *aCtx, u8 *aBuffer, const u16 aLen)
{
    if (!aCtx || !aCtx->mCtx || !aBuffer || aLen == 0) {
        return E_INVALID_PARAMETERS;
    }
    
    esp32_i2c_ctx_t *esp_ctx = (esp32_i2c_ctx_t*)aCtx->mCtx;
    if (!esp_ctx->initialized) {
        return E_INVALID_STATE;
    }
    
    esp_err_t ret = i2c_master_receive(esp_ctx->dev_handle, aBuffer, aLen, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from I2C device: %s", esp_err_to_name(ret));
        if (ret == ESP_ERR_TIMEOUT) {
            return E_TIMEOUT;
        } else if (ret == ESP_ERR_INVALID_RESPONSE) {
            return E_FAIL_ACK;
        }
        return E_READ_ERROR;
    }
    
    ESP_LOGD(TAG, "Successfully read %d bytes from I2C device 0x%02X", aLen, esp_ctx->device_addr);
    return SUCCESS;
}

/* --------------------------------------------------------- */

ret_code_t iohub_i2c_write(i2c_ctx *aCtx, const u8 *aBuffer, const u16 aLen, BOOL afIssueStop)
{
    if (!aCtx || !aCtx->mCtx || !aBuffer || aLen == 0) {
        return E_INVALID_PARAMETERS;
    }
    
    esp32_i2c_ctx_t *esp_ctx = (esp32_i2c_ctx_t*)aCtx->mCtx;
    if (!esp_ctx->initialized) {
        return E_INVALID_STATE;
    }
    
    esp_err_t ret = i2c_master_transmit(esp_ctx->dev_handle, aBuffer, aLen, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to I2C device: %s", esp_err_to_name(ret));
        if (ret == ESP_ERR_TIMEOUT) {
            return E_TIMEOUT;
        } else if (ret == ESP_ERR_INVALID_RESPONSE) {
            return E_FAIL_ACK;
        }
        return E_WRITE_ERROR;
    }
    
    ESP_LOGD(TAG, "Successfully wrote %d bytes to I2C device 0x%02X", aLen, esp_ctx->device_addr);
    return SUCCESS;
}
