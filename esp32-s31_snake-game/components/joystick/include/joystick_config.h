/* Joystick configuration. Edit this file only; overrides via -D are allowed. */

#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/adc_channel.h"

#ifndef JOYSTICK_ADC_UNIT
#define JOYSTICK_ADC_UNIT       ADC_UNIT_1
#endif

/* X = GPIO42, Y = GPIO44 (axes swapped on 2026-10-22). */
#ifndef JOYSTICK_X_GPIO
#define JOYSTICK_X_GPIO          42
#endif
#ifndef JOYSTICK_Y_GPIO
#define JOYSTICK_Y_GPIO          44
#endif

/* Active-low button with internal pull-up; -1 = no button. */
#ifndef JOYSTICK_BUTTON_GPIO
#define JOYSTICK_BUTTON_GPIO     38
#endif

/* ESP32-S31 accepts only DB_0 (SOC_ADC_ATTEN_NUM == 1). */
#ifndef JOYSTICK_ATTEN
#define JOYSTICK_ATTEN           ADC_ATTEN_DB_0
#endif

/* Resting center on S31 N-side pins; used only before calibrate(). */
#ifndef JOYSTICK_FALLBACK_CENTER
#define JOYSTICK_FALLBACK_CENTER 530
#endif

#ifndef JOYSTICK_DEADZONE
#define JOYSTICK_DEADZONE        150
#endif

#ifndef JOYSTICK_CAL_SAMPLES
#define JOYSTICK_CAL_SAMPLES     50
#endif

#ifndef JOYSTICK_DEBOUNCE_MS
#define JOYSTICK_DEBOUNCE_MS     20
#endif

/* Per-axis polarity, 1 = invert. */
#ifndef JOYSTICK_INVERT_X
#define JOYSTICK_INVERT_X        1
#endif
#ifndef JOYSTICK_INVERT_Y
#define JOYSTICK_INVERT_Y        0
#endif

#define _JOYSTICK_ADC1_CHANNEL(gpio)  ADC1_GPIO##gpio##_CHANNEL
#define JOYSTICK_ADC1_CHANNEL(gpio)   _JOYSTICK_ADC1_CHANNEL(gpio)

#define JOYSTICK_X_CHANNEL  JOYSTICK_ADC1_CHANNEL(JOYSTICK_X_GPIO)
#define JOYSTICK_Y_CHANNEL  JOYSTICK_ADC1_CHANNEL(JOYSTICK_Y_GPIO)

#define JOYSTICK_DEFAULT_CONFIG  { \
    .adc_unit    = JOYSTICK_ADC_UNIT, \
    .x_channel   = JOYSTICK_X_CHANNEL, \
    .y_channel   = JOYSTICK_Y_CHANNEL, \
    .atten       = JOYSTICK_ATTEN, \
    .button_gpio = (gpio_num_t)JOYSTICK_BUTTON_GPIO, \
    .deadzone    = JOYSTICK_DEADZONE, \
}
