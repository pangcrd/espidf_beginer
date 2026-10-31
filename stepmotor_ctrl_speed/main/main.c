/* Joystick -> stepper motor, step/dir driver (A4988 / DRV8825 / TMC2209).
 * Joystick X sets direction (sign) and target speed (magnitude).
 * STEP is a LEDC PWM train, so pulses are evenly spaced; a burst
 * of pulses followed by silence makes the motor buzz and lose steps.
 * main/CMakeLists.txt: SRCS main_stepper.c,
 * REQUIRES joystick esp_driver_ledc (IDF v6; on v5 use driver). */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "joystick.h"

static const char *TAG = "stepper";

#define STEP_GPIO  GPIO_NUM_3
#define DIR_GPIO   GPIO_NUM_0
#define EN_GPIO    GPIO_NUM_2

#define TICK_MS         10    /* ramp loop period, >= 1 tick period */
#define JOY_READ_MS     20    /* joystick read period */

#define STEP_MAX_SPS    10000.0f
#define STEP_ACCEL_SPS2 10000.0f 

#define JOY_POS_MAX     530
#define JOY_NEG_MAX     1670

/* Set STEP PWM frequency. Duty 0 stops stepping (output low). */
static void set_step_freq(float sps, bool *running)
{
    if (sps <= 0.0f) {
        if (*running) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            *running = false;
        }
        return;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, (uint32_t)sps);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096); /* 50% */
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    *running = true;
}

void app_main(void)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << DIR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
    gpio_set_level(DIR_GPIO, 0);
    gpio_config_t en_cfg = {
        .pin_bit_mask = (1ULL << EN_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&en_cfg));
    gpio_set_level(EN_GPIO, 1); /* driver disabled until STEP is ready */

    joystick_handle_t joystick = NULL;
    ESP_ERROR_CHECK(joystick_init_default(&joystick));
    ESP_LOGI(TAG, "joystick_init OK, hold the joystick centered to calibrate...");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_ERROR_CHECK(joystick_calibrate(joystick, JOYSTICK_CAL_SAMPLES));

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .gpio_num = STEP_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
    /* EN stays high (disabled) here on purpose: the first button
     * press enables the driver, so a power-up spin is impossible. */
    ESP_LOGI(TAG, "press the joystick button to enable the driver");

    float current_sps = 0.0f;
    float last_sps = -1.0f;
    bool pwm_running = false;
    int moving_dir = 0; /* 0 = stopped, 1 / -1 = current direction */
    int target_dir = 0;
    float target_sps = 0.0f;
    bool driver_enabled = false; /* button toggle, starts disabled */
    bool prev_button_down = false;
    int tick = 0;

    while (1) {
        /* Read the joystick every 20 ms; ramp every 10 ms. */
        if (tick % (JOY_READ_MS / TICK_MS) == 0) {
            int x_norm, y_norm;
            bool button_down = false;
            if (joystick_read_normalized(joystick, &x_norm, &y_norm,
                                         &button_down) == ESP_OK) {
                /* Toggle EN on the press edge only. The library
                 * debounces the button (20 ms), and prev_button
                 * stops a held press from toggling repeatedly. */
                if (button_down && !prev_button_down) {
                    driver_enabled = !driver_enabled;
                    gpio_set_level(EN_GPIO, driver_enabled ? 0 : 1);
                    ESP_LOGI(TAG, "driver %s",
                             driver_enabled ? "ENABLED" : "DISABLED");
                    if (!driver_enabled) {
                        current_sps = 0.0f;
                        moving_dir = 0;
                    }
                }
                prev_button_down = button_down;
                if (x_norm == 0) {
                    target_dir = 0;
                    target_sps = 0.0f;
                } else {
                    target_dir = (x_norm > 0) ? 1 : -1;
                    int max_codes = (x_norm > 0) ? JOY_POS_MAX : JOY_NEG_MAX;
                    float frac = (float)abs(x_norm) / (float)max_codes;
                    if (frac > 1.0f) {
                        frac = 1.0f;
                    }
                    target_sps = frac * STEP_MAX_SPS;
                }
            }
        }
        tick++;

        /* Direction change must wait for a full stop first,
         * otherwise the motor stalls and loses steps. While the
         * driver is disabled, no direction change is accepted. */
        float desired_sps = target_sps;
        if (!driver_enabled) {
            desired_sps = 0.0f;
        } else if (target_dir == 0) {
            desired_sps = 0.0f;
        } else if (moving_dir == 0) {
            moving_dir = target_dir;
            gpio_set_level(DIR_GPIO, moving_dir > 0 ? 1 : 0);
        } else if (target_dir != moving_dir) {
            desired_sps = 0.0f;
            if (current_sps <= 0.0f) {
                moving_dir = target_dir;
                gpio_set_level(DIR_GPIO, moving_dir > 0 ? 1 : 0);
                desired_sps = target_sps;
            }
        }

        /* Ramp current speed toward the desired speed. */
        const float dt = TICK_MS / 1000.0f;
        const float step = STEP_ACCEL_SPS2 * dt;
        if (current_sps < desired_sps) {
            current_sps += step;
            if (current_sps > desired_sps) {
                current_sps = desired_sps;
            }
        } else if (current_sps > desired_sps) {
            current_sps -= step;
            if (current_sps < desired_sps) {
                current_sps = desired_sps;
            }
        }
        if (current_sps <= 0.0f) {
            current_sps = 0.0f;
            if (target_dir == 0) {
                moving_dir = 0;
            }
        }

        /* Update the PWM train only when the speed changed. */
        if (current_sps != last_sps) {
            set_step_freq(current_sps, &pwm_running);
            last_sps = current_sps;
        }

        vTaskDelay(pdMS_TO_TICKS(TICK_MS));
    }
}
