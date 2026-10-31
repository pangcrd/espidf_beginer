/* Joystick driver: ADC oneshot -> center subtraction -> polarity -> deadzone. */

#include "joystick.h"

#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

static const char *TAG = "joystick";

/* Debounce window. */
#define JOYSTICK_DEBOUNCE_US  (JOYSTICK_DEBOUNCE_MS * 1000)
struct joystick_dev {
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t             x_channel;
    adc_channel_t             y_channel;
    gpio_num_t                button_gpio;
    bool                      has_button;
    int                       deadzone;
    bool                      calibrated;
    int                       center_x;
    int                       center_y;
    bool                      button_stable_state;
    bool                      button_last_raw_state;
    int64_t                   button_last_change_us;
};

/* Debounced active-low button read. */
static bool read_button_debounced(struct joystick_dev *dev)
{
    bool raw_pressed = (gpio_get_level(dev->button_gpio) == 0);
    int64_t now = esp_timer_get_time();

    if (raw_pressed != dev->button_last_raw_state) {
        dev->button_last_raw_state = raw_pressed;
        dev->button_last_change_us = now;
    } else if ((now - dev->button_last_change_us) >= JOYSTICK_DEBOUNCE_US) {
        dev->button_stable_state = raw_pressed;
    }

    return dev->button_stable_state;
}

/* Init ADC unit, both channels, and the button GPIO. */
esp_err_t joystick_init(const joystick_config_t *config, joystick_handle_t *out_handle)
{
    if (config == NULL || out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct joystick_dev *dev = calloc(1, sizeof(struct joystick_dev));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    dev->x_channel   = config->x_channel;
    dev->y_channel   = config->y_channel;
    dev->button_gpio = config->button_gpio;
    dev->has_button  = (config->button_gpio >= 0);
    dev->deadzone    = config->deadzone;
    dev->calibrated  = false;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = config->adc_unit,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &dev->adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        free(dev);
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = config->atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
#if defined(SOC_ADC_ATTEN_NUM) && (SOC_ADC_ATTEN_NUM == 1)
    /* S31 only accepts DB_0; force it so a stale DB_12 config
     * does not fail at the first channel ("invalid attenuation"). */
    chan_cfg.atten = ADC_ATTEN_DB_0;
#endif

    err = adc_oneshot_config_channel(dev->adc_handle, dev->x_channel, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "config X channel failed: %s", esp_err_to_name(err));
        adc_oneshot_del_unit(dev->adc_handle);
        free(dev);
        return err;
    }

    err = adc_oneshot_config_channel(dev->adc_handle, dev->y_channel, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "config Y channel failed: %s", esp_err_to_name(err));
        adc_oneshot_del_unit(dev->adc_handle);
        free(dev);
        return err;
    }

    if (dev->has_button) {
        gpio_config_t io_cfg = {
            .pin_bit_mask = 1ULL << dev->button_gpio,
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        err = gpio_config(&io_cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "gpio_config for button failed: %s", esp_err_to_name(err));
            adc_oneshot_del_unit(dev->adc_handle);
            free(dev);
            return err;
        }

        dev->button_last_raw_state  = (gpio_get_level(dev->button_gpio) == 0);
        dev->button_stable_state    = dev->button_last_raw_state;
        dev->button_last_change_us  = esp_timer_get_time();
    }

    *out_handle = dev;
    return ESP_OK;
}

/* Init from JOYSTICK_DEFAULT_CONFIG. */
esp_err_t joystick_init_default(joystick_handle_t *out_handle)
{
    static const joystick_config_t default_cfg = JOYSTICK_DEFAULT_CONFIG;
    return joystick_init(&default_cfg, out_handle);
}

/* Read raw X/Y and the debounced button. Any out pointer may be NULL. */
esp_err_t joystick_read(joystick_handle_t handle, int *out_x_raw, int *out_y_raw, bool *out_button_down)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int x_raw = 0, y_raw = 0;

    esp_err_t err = adc_oneshot_read(handle->adc_handle, handle->x_channel, &x_raw);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read X failed: %s", esp_err_to_name(err));
        return err;
    }

    err = adc_oneshot_read(handle->adc_handle, handle->y_channel, &y_raw);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read Y failed: %s", esp_err_to_name(err));
        return err;
    }

    if (out_x_raw) {
        *out_x_raw = x_raw;
    }
    if (out_y_raw) {
        *out_y_raw = y_raw;
    }
    if (out_button_down) {
        *out_button_down = handle->has_button ? read_button_debounced(handle) : false;
    }

    return ESP_OK;
}

/* Average samples at rest to find the center. Call once after init. */
esp_err_t joystick_calibrate(joystick_handle_t handle, int num_samples)
{
    if (handle == NULL || num_samples <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    long sum_x = 0, sum_y = 0;

    for (int i = 0; i < num_samples; i++) {
        int x_raw, y_raw;
        esp_err_t err = joystick_read(handle, &x_raw, &y_raw, NULL);
        if (err != ESP_OK) {
            return err;
        }
        sum_x += x_raw;
        sum_y += y_raw;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    handle->center_x  = (int)(sum_x / num_samples);
    handle->center_y  = (int)(sum_y / num_samples);
    handle->calibrated = true;

    ESP_LOGI(TAG, "calibrated: center_x=%d center_y=%d", handle->center_x, handle->center_y);
    return ESP_OK;
}

/* Center-subtracted, polarity-corrected, deadzone-applied values.
 * Polarity is applied here, so the app must not invert again.
 * Falls back to JOYSTICK_FALLBACK_CENTER if not calibrated. */
esp_err_t joystick_read_normalized(joystick_handle_t handle, int *out_x, int *out_y, bool *out_button_down)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int x_raw, y_raw;
    bool button_down = false;

    esp_err_t err = joystick_read(handle, &x_raw, &y_raw, &button_down);
    if (err != ESP_OK) {
        return err;
    }

    int center_x = handle->calibrated ? handle->center_x : JOYSTICK_FALLBACK_CENTER;
    int center_y = handle->calibrated ? handle->center_y : JOYSTICK_FALLBACK_CENTER;

    int dx = x_raw - center_x;
    int dy = y_raw - center_y;

#if JOYSTICK_INVERT_X
    dx = -dx;
#endif
#if JOYSTICK_INVERT_Y
    dy = -dy;
#endif

    if (abs(dx) < handle->deadzone) {
        dx = 0;
    }
    if (abs(dy) < handle->deadzone) {
        dy = 0;
    }

    if (out_x) {
        *out_x = dx;
    }
    if (out_y) {
        *out_y = dy;
    }
    if (out_button_down) {
        *out_button_down = button_down;
    }

    return ESP_OK;
}

/* Free the ADC unit and the handle. */
esp_err_t joystick_deinit(joystick_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = adc_oneshot_del_unit(handle->adc_handle);
    free(handle);
    return err;
}