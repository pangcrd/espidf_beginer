/** 
 * ESP-IDF Example: Control LED with 2 button
 * Button Interrupt 
 * LED Blink
 * Stop task
 * Resume task
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 26     
#define BUTTON1 35  /** LED stop button (STOP) */
#define BUTTON2 36   /** LED resume button (RESUME) */

static const char *TAG = "MAIN";

TaskHandle_t ledTaskHandle = NULL; /** Handle of Task LED */
QueueHandle_t buttonQueue;  /** Queue contains button ID (used for communication between ISR and main Task) */

void IRAM_ATTR button_isr_handler(void *arg) { /** ISR: Handles interrupt when button is pressed, sends button ID to Queue */
    int btn_id = (int)arg;  /** Get the pressed button ID */
    xQueueSendFromISR(buttonQueue, &btn_id, NULL);
}
/** Task LED: LED blinks */
void led_task(void *pvParams) {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT); /** GPIO as OUTPUT */
    ESP_LOGI(TAG, "LED Task Started!");

    while (1) {

        gpio_set_level(LED_PIN, 1); 
        ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(500));

        gpio_set_level(LED_PIN, 0);
        ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
/** Button handling task */
void button_task(void *pvParams) {
    int btn_id;
    while (1) {
        if (xQueueReceive(buttonQueue, &btn_id, portMAX_DELAY)) {
            if (btn_id == BUTTON1) {  /** If you press the STOP button */
                if (ledTaskHandle != NULL) {
                    vTaskSuspend(ledTaskHandle); /** Pause Task LED */
                    ESP_LOGW(TAG, "Task LED Stopped!");
                }
            } else if (btn_id == BUTTON2) {  /** If you press the START button */
                if (ledTaskHandle != NULL) {
                    vTaskResume(ledTaskHandle); /** Resume Task LED  */
                    ESP_LOGW(TAG, "Task LED Resumed!");
                }
            }
        }
    }
}

void app_main() {
    /** Create Queue containing button ID (for communication between ISR and main Task) */
    buttonQueue = xQueueCreate(10, sizeof(int));

    /** Push button configuration */
    gpio_set_direction(BUTTON1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON1, GPIO_PULLUP_ONLY); /** Pull-up resistor */
    gpio_set_intr_type(BUTTON1, GPIO_INTR_NEGEDGE);

    gpio_set_direction(BUTTON2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON2, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON2, GPIO_INTR_NEGEDGE);
    
    /** Register separate interrupts for each button */
    gpio_install_isr_service(0);

    gpio_isr_handler_add(BUTTON1, button_isr_handler, (void*)BUTTON1);
    gpio_isr_handler_add(BUTTON2, button_isr_handler, (void*)BUTTON2);
    ESP_LOGI(TAG, "Button ISR Installed!");

    /** Create LED Task and Button Handling Task */
    xTaskCreate(led_task, "LED Task", 2048, NULL, 5, &ledTaskHandle);
    xTaskCreate(button_task, "Button Task", 2048, NULL, 5, NULL);
}
