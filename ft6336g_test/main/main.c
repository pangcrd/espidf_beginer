/* Poll test, no LVGL. Copy this file to main/main.c for step 1.
 * Expected first lines if wiring + address are right:
 *   FT6336G v1.0.1 ready: vendor 0x11 cipher 64-26-01 ...
 *   CHIP vendor=0x11 cipher=64-26-01 lib=16.1 fw=0x00
 * Then touch the panel: one log line per change, not per poll.
 * If init fails, check the README step 1 (I2C scan, AHT20 clash). */
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ft6336g.h"

static const char *TAG = "FT6336G_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "FT6336G poll test, driver %s", ft6336g_version());

    ft6336g_handle_t touch = NULL;
    esp_err_t err = ft6336g_init_default(&touch);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %s - see README step 1", esp_err_to_name(err));
        return;
    }

    ft6336g_chip_id_t id;
    if (ft6336g_get_chip_id(touch, &id) == ESP_OK) {
        ESP_LOGI(TAG, "CHIP vendor=0x%02x cipher=%02x-%02x-%02x lib=%d.%d fw=0x%02x",
                 id.vendor_id, id.cipher_high, id.cipher_mid, id.cipher_low,
                 id.lib_ver_high, id.lib_ver_low, id.firmware_id);
    }

    ft6336g_touch_t prev = {0};
    bool first = true;
    while (1) {
        ft6336g_touch_t t = {0};
        if (ft6336g_read(touch, &t) == ESP_OK) {
            bool changed = first || t.count != prev.count ||
                           t.points[0].x != prev.points[0].x ||
                           t.points[0].y != prev.points[0].y ||
                           t.points[0].event != prev.points[0].event ||
                           t.points[1].x != prev.points[1].x ||
                           t.points[1].y != prev.points[1].y;
            if (changed) {
                if (t.count == 0) {
                    ESP_LOGI(TAG, "RELEASE");
                } else {
                    ESP_LOGI(TAG, "count=%d p0 id=%d x=%d y=%d ev=%d | p1 id=%d x=%d y=%d ev=%d",
                             t.count,
                             t.points[0].id, t.points[0].x, t.points[0].y, t.points[0].event,
                             t.points[1].id, t.points[1].x, t.points[1].y, t.points[1].event);
                }
                prev = t;
                first = false;
            }
        } else {
            ESP_LOGW(TAG, "read failed");
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}