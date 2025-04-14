/** 
 * @file ledc pwm example
 * @brief This example demonstrates how to use the LEDC peripheral to generate PWM signals.
 */

 #include <stdio.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/queue.h"
 #include "sdkconfig.h"
 #include "driver/ledc.h"

 #define LEDC_CHANNEL     LEDC_CHANNEL_0
 #define LEDC_GPIO        8
 #define LEDC_TIMER       LEDC_TIMER_0
 #define LEDC_MODE        LEDC_LOW_SPEED_MODE /** Only use for ESP32-C3 */
 #define LEDC_DUTY_RES    LEDC_TIMER_8_BIT  // 0 - 255
 #define LEDC_FREQUENCY   5000              // 5kHz
 
 void app_main(void)
 {
     /** Config timer for ledc */
     ledc_timer_config_t ledc_timer = {
         .speed_mode       = LEDC_MODE,
         .timer_num        = LEDC_TIMER,
         .duty_resolution  = LEDC_DUTY_RES,
         .freq_hz          = LEDC_FREQUENCY,
         .clk_cfg          = LEDC_AUTO_CLK
     };
     ledc_timer_config(&ledc_timer);
 
     /** Config timer */
     ledc_channel_config_t ledc_channel = {
         .channel    = LEDC_CHANNEL,
         .duty       = 0,
         .gpio_num   = LEDC_GPIO,
         .speed_mode = LEDC_MODE,
         .hpoint     = 0,
         .timer_sel  = LEDC_TIMER
     };
     ledc_channel_config(&ledc_channel);
 
     /** Auto pwm */
     while (1) {
         for (int duty = 0; duty <= 255; duty += 1) {
             ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
             ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
             vTaskDelay(pdMS_TO_TICKS(30));
         }
     }
 }