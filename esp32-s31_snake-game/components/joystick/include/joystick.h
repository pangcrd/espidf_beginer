/* Joystick driver (ESP-IDF, adc_oneshot). See joystick.c for notes. */

#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "joystick_config.h"

typedef struct {
    adc_unit_t    adc_unit;
    adc_channel_t x_channel;
    adc_channel_t y_channel;
    adc_atten_t   atten;
    gpio_num_t    button_gpio;
    int           deadzone;
} joystick_config_t;

typedef struct joystick_dev *joystick_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t joystick_init(const joystick_config_t *config,
                        joystick_handle_t *out_handle);
esp_err_t joystick_init_default(joystick_handle_t *out_handle);
esp_err_t joystick_read(joystick_handle_t handle,
                        int *out_x_raw, int *out_y_raw,
                        bool *out_button_down);
esp_err_t joystick_calibrate(joystick_handle_t handle, int num_samples);
esp_err_t joystick_read_normalized(joystick_handle_t handle,
                                   int *out_x, int *out_y,
                                   bool *out_button_down);
esp_err_t joystick_deinit(joystick_handle_t handle);

#ifdef __cplusplus
}
#endif
