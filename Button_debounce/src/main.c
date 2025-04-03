/**
 * ESP-IDF example button debouce
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_PIN 35
#define DEBOUNCE_TIME_MS 50
#define LED_PIN 4

TimerHandle_t debounce_timer;

uint8_t led_state = 0;

static const char *tag = "MAIN";

/** ISR: Handle interrupt when button is pressed */
static void IRAM_ATTR button_isr_handler(void* arg)
{
    xTimerResetFromISR( debounce_timer, NULL);
}
/** Debounce timer callback */

void debounce_timer_callback(TimerHandle_t myTimer){
    if(gpio_get_level(BUTTON_PIN) == 0){ /** pull-up resistor  value is 0 when button pressed */
        ESP_LOGI(tag,"Button pressed! LED :%s", led_state ? "OFF": "ON");
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state);
    }
}

void app_main(){

    /** button configuration */
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_PIN,GPIO_INTR_NEGEDGE); /** For pullup resistor. If use pulldown "GPIO_INTR_POSEDGE" */

    /** LED configuration GPIO */
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
   

    /** Create timer debounce */
    debounce_timer = xTimerCreate("debounce_timer",
                    pdMS_TO_TICKS(DEBOUNCE_TIME_MS), /** 50ms */
                    pdFALSE,
                    NULL,
                    debounce_timer_callback);

    /** Register separate interrupts for button */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN,button_isr_handler,NULL);

    ESP_LOGI(tag,"Button ISR with debounce Installed");
}
