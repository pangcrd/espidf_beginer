#ifndef LED_H
#define LED_H

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class LED {
    public:
        LED(uint8_t gpio);  
        void on();
        void off();
        void blink(int delay_ms);
    private:
        gpio_num_t _gpio;  
    };
#endif // LED_H