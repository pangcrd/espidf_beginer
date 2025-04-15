/** 
 * @file AHT20 example
 */
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <aht.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>

#define I2C_MASTER_SDA 5 
#define I2C_MASTER_SCL 6
#define LED_PIN 8  

#define ADDR AHT_I2C_ADDRESS_GND

#define AHT_TYPE AHT_TYPE_AHT20

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

TaskHandle_t AHTTaskHandle = NULL;
TaskHandle_t BlinkTaskHandle = NULL;

static const char *TAG = "aht20-blink-example";

void AHTtask(void *pvParams)
{
    aht_t dev = { 0 };
    dev.mode = AHT_MODE_NORMAL;
    dev.type = AHT_TYPE;

    ESP_ERROR_CHECK(aht_init_desc(&dev, ADDR, 0, I2C_MASTER_SDA, I2C_MASTER_SCL));
    ESP_ERROR_CHECK(aht_init(&dev));

    bool calibrated;
    ESP_ERROR_CHECK(aht_get_status(&dev, NULL, &calibrated));
    if (calibrated)
        ESP_LOGI(TAG, "Sensor calibrated");
    else
        ESP_LOGW(TAG, "Sensor not calibrated!");

    float temperature, humidity;

    while (1)
    {
        esp_err_t res = aht_get_data(&dev, &temperature, &humidity);
        if (res == ESP_OK)
            ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.2f%%", temperature, humidity);
        else
            ESP_LOGE(TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void blink_task(void *pvParams){
     /** GPIO configuration */
     gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT); /** GPIO as OUTPUT */
    while(1){
        gpio_set_level(LED_PIN, 1);      
        ESP_LOGI(TAG, "LED ON!");
        vTaskDelay(pdMS_TO_TICKS(1000));    
        gpio_set_level(LED_PIN, 0);        
        ESP_LOGI(TAG, "LED OFF!");
        vTaskDelay(pdMS_TO_TICKS(1000));  
    }
}

void app_main()
{
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreatePinnedToCore(AHTtask, TAG, configMINIMAL_STACK_SIZE * 4, NULL, 3, &AHTTaskHandle, APP_CPU_NUM);
    xTaskCreatePinnedToCore(blink_task, TAG, configMINIMAL_STACK_SIZE * 2, NULL, 2, &BlinkTaskHandle, APP_CPU_NUM);
}
