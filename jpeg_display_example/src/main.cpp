#include "esp_display_panel.hpp"
#include "esp_lib_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "bsp_sd.h"
#include "jpeg_dec.h"

#define PANEL_SWAP_BYTES true

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char *TAG = "tft_sdcard_test";

extern "C" void app_main(void)
{
    // --- SD ---
    bsp_sd_config_t sd = {
        .clk = 38, .cmd = 40,
        .d0 = 39, .d1 = 41, .d2 = 48, .d3 = 47,
        .width = 4,
        .max_freq_khz = SDMMC_FREQ_HIGHSPEED,
        .internal_pullup = true,
    };
    if (bsp_sd_mount(&sd) != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
        return;
    }

    // --- Display ---
    Board *board = new Board();
    assert(board);
    ESP_UTILS_CHECK_FALSE_EXIT(board->init(), "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");

    auto lcd = board->getLCD();
    lcd->swapXY(true);
    lcd->mirrorX(true);
    lcd->mirrorY(true);
    const int w = 320, h = 240;
    ESP_LOGI(TAG, "panel %dx%d", w, h);

    // --- JPEG test ---
    jpeg_image_t img;
    if (jpeg_dec_file("/sdcard/camellya.jpg", PANEL_SWAP_BYTES, &img) != ESP_OK) {
        ESP_LOGE(TAG, "decode failed");
        return;
    }
    if (img.width <= w && img.height <= h) {
        int x = (w - img.width) / 2;
        int y = (h - img.height) / 2;
        lcd->drawBitmap(x, y, img.width, img.height, (uint8_t *)img.pixels);
    } else {
        ESP_LOGW(TAG, "anh %ux%u lon hon man hinh %dx%d", img.width, img.height, w, h);
    }
    jpeg_image_free(&img);
}