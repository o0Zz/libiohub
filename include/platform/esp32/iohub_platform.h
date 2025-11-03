#pragma once

#include "utils/iohub_errors.h"
#include "utils/iohub_types.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

static inline void iohub_platform_init()
{
	//Nothing to do
}

#define IOHUB_LOG_DEBUG(...)			ESP_LOGD("iohub", __VA_ARGS__)
#define IOHUB_LOG_INFO(...)				ESP_LOGI("iohub", __VA_ARGS__)
#define IOHUB_LOG_WARNING(...)			ESP_LOGW("iohub", __VA_ARGS__)
#define IOHUB_LOG_ERROR(...)		    ESP_LOGE("iohub", __VA_ARGS__)
#define IOHUB_LOG_BUFFER(buf, size)	    ESP_LOG_BUFFER_CHAR_LEVEL("iohub", buf, size, ESP_LOG_DEBUG)

/* GPIO Digital I/O Macros for ESP32 */
#define iohub_digital_set_pin_mode(aPin, aPinMode) \
    gpio_set_direction((gpio_num_t)(aPin), (aPinMode == PinMode_Output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT)

#define iohub_digital_write(aPin, aPinLevel) \
    gpio_set_level((gpio_num_t)(aPin), (aPinLevel == PinLevel_High) ? 1 : 0)

#define iohub_digital_read(aPin) \
    (gpio_get_level((gpio_num_t)(aPin)) ? PinLevel_High : PinLevel_Low)


#define iohub_time_delay_ms(anMSDelay)							vTaskDelay(pdMS_TO_TICKS(anMSDelay))
#define iohub_time_delay_us(anUSDelay)							esp_rom_delay_us(anUSDelay)
#define iohub_time_now_us()										esp_timer_get_time()
#define iohub_time_now_ms()										(esp_timer_get_time() / 1000)

/* Interrupt control for ESP32 */
#define iohub_interrupts_disable()                                 portENTER_CRITICAL(NULL)
#define iohub_interrupts_enable()                                  portEXIT_CRITICAL(NULL)

#define IOHUB_GPIO_INT_TYPE_CHANGE       GPIO_INTR_ANYEDGE
#define IOHUB_GPIO_INT_TYPE_FALLING      GPIO_INTR_NEGEDGE
#define IOHUB_GPIO_INT_TYPE_RISING       GPIO_INTR_POSEDGE

/* GPIO Interrupt functions for ESP32 */
static inline ret_code_t iohub_attach_interrupt(gpio_num_t pin, gpio_isr_t isr_handler, gpio_int_type_t intr_type, void* arg)
{
    static bool isr_service_installed = false;
    
    // Install ISR service if not already done
    if (!isr_service_installed) {
        esp_err_t ret = gpio_install_isr_service(0);
        if (ret != ESP_OK) {
            return (ret_code_t)ret;
        }
        isr_service_installed = true;
    }
    
    // Configure pin as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = intr_type
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        return (ret_code_t)ret;
    }
    
    // Add ISR handler
    ret = gpio_isr_handler_add(pin, isr_handler, arg);
    return (ret_code_t)ret;
}

static inline ret_code_t iohub_detach_interrupt(u8 pin)
{
    esp_err_t ret = gpio_isr_handler_remove((gpio_num_t) pin);
    return (ret_code_t)ret;
}

