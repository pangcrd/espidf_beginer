#include "led.h"
#include "esp_log.h" 

static const char *TAG = "blink-OOP";   

LED::LED(uint8_t gpio) : _gpio((gpio_num_t)gpio) {
    gpio_set_direction(_gpio, GPIO_MODE_OUTPUT);
}

void LED::on() {
    gpio_set_level(_gpio, 0);
}

void LED::off() {
    gpio_set_level(_gpio, 1);
}

void LED::blink(int delay_ms) {
    on();
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    ESP_LOGI(TAG, "LED ON!");
    off();
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    ESP_LOGI(TAG, "LED OFF!");
}
