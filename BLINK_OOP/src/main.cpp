#include "led.h"

#define LED_PIN 8 

TaskHandle_t ledBlinkHandle = NULL; 

void blink_task(void *pvParams) {
    LED myLed(LED_PIN);
    while (1) {
        myLed.blink(1000); // Blink every 1 second
    }
}

extern "C" void app_main() {
    xTaskCreate(&blink_task, "LED_BLINK", 2048, NULL, 5, &ledBlinkHandle ); 
}
