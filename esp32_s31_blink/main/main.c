
/** Blink Example
* RGB LED ESP32-S31
* DHT20 Test
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#define LED_GPIO    60
#define LED_COUNT   1

void app_main(void)
{
    led_strip_handle_t strip;

    led_strip_config_t strip_cfg = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,  // 10 MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));

    while (1) {
        led_strip_set_pixel(strip, 0, 100, 10, 20);   // R, G, B (0-255)
        led_strip_refresh(strip);
        vTaskDelay(pdMS_TO_TICKS(500));

        led_strip_clear(strip);                   
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}