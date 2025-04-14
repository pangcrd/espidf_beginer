/**

ESP32-S3 ADC Channels
+-------------------+----------+              +-------------------+----------+
| Channel ADC1      |   GPIO   |              | Channel ADC2      |   GPIO   |
+-------------------+----------+              +-------------------+----------+
| ADC1_CHANNEL_0    | GPIO 1   |              | ADC2_CHANNEL_0    | GPIO 11  |
| ADC1_CHANNEL_1    | GPIO 2   |              | ADC2_CHANNEL_1    | GPIO 12  |
| ADC1_CHANNEL_2    | GPIO 3   |              | ADC2_CHANNEL_2    | GPIO 13  |
| ADC1_CHANNEL_3    | GPIO 4   |              | ADC2_CHANNEL_3    | GPIO 14  |
| ADC1_CHANNEL_4    | GPIO 5   |              | ADC2_CHANNEL_4    | GPIO 15  |
| ADC1_CHANNEL_5    | GPIO 6   |              | ADC2_CHANNEL_5    | GPIO 16  |
| ADC1_CHANNEL_6    | GPIO 7   |              | ADC2_CHANNEL_6    | GPIO 17  |
| ADC1_CHANNEL_7    | GPIO 8   |              | ADC2_CHANNEL_7    | GPIO 18  |
| ADC1_CHANNEL_8    | GPIO 9   |              | ADC2_CHANNEL_8    | GPIO 19  |
| ADC1_CHANNEL_9    | GPIO 10  |              | ADC2_CHANNEL_9    | GPIO 20  |
+-------------------+----------+              +-------------------+----------+
                       +----------------+----------+
https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32/api-reference/peripherals/adc_oneshot.html

Using ADC1_CHANNEL_2 to read the LDR.

 **/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    3300    
#define NO_OF_SAMPLES   64      

int adc_value = 0;
static esp_adc_cal_characteristics_t *adc_chars;

void app_main() {
    
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_12);
    adc1_config_width( ADC_ATTEN_DB_12);

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));

    if (esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12,  ADC_ATTEN_DB_12, DEFAULT_VREF, adc_chars) == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Using eFuse Vref\n");
    } else {
        printf("Using default Vref: %d mV\n", DEFAULT_VREF);
    }


    while (1) {

        uint32_t adc_reading = 0;

        // Read ADC multiple times and average to reduce noise
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw(ADC1_CHANNEL_2);
        }
        adc_reading /= NO_OF_SAMPLES;

        // Convert ADC value to mV (correction)
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

        printf("ADC Raw: %lu, Voltage: %lu mV\n", adc_reading, voltage);

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}