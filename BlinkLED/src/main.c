/** Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>                 /** C standard library */
#include "freertos/FreeRTOS.h"      /** FreeRTOS libraries (tasks, delays, etc.) */
#include "freertos/task.h"
#include "driver/gpio.h"            /** GPIO control library */
#include "esp_log.h"                /** Library for debugging/logging */

static const char *TAG = "MAIN";     /** Define TAG to log */

#define LED_PIN 26  

TaskHandle_t ledBlinkHandle = NULL; /** Handle of Task LED */

/** Create task LED */
void led_blink(void *pvParams) {

    ESP_LOGI(TAG, "Start blink LED!");

    /** GPIO configuration */
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT); /** GPIO as OUTPUT */

    while (1) {
        gpio_set_level(LED_PIN, 0);          /**  LED ON */
        ESP_LOGI(TAG, "LED ON!");
        vTaskDelay(pdMS_TO_TICKS(1000));     /** Delay 1000ms */ 

        gpio_set_level(LED_PIN, 1);          /** LED OFF */
        ESP_LOGI(TAG, "LED OFF!");
        vTaskDelay(pdMS_TO_TICKS(1000));     /** Delay 1000ms */
    }
}

void app_main() {
    xTaskCreate(&led_blink, "LED_BLINK", 2048, NULL, 5, &ledBlinkHandle );  /** Create task blink LED */
    //xTaskCreatePinnedToCore(&led_blink, "LED Task", 1024, NULL, 5, &ledBlinkHandle, 1);
}
