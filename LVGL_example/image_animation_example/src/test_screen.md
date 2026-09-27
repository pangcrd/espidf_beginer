```cpp

#include "esp_display_panel.hpp"
#include "esp_lib_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char *TAG = "tft_test";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "display init");

    Board *board = new Board();
    assert(board);

    ESP_UTILS_CHECK_FALSE_EXIT(board->init(), "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");

    auto lcd = board->getLCD();

    ESP_LOGI(TAG, "Test color bar");
    lcd->colorBarTest();
    vTaskDelay(pdMS_TO_TICKS(3000));

    const int w = 172;   // ESP_PANEL_BOARD_WIDTH
    const int h = 320;   // ESP_PANEL_BOARD_HEIGHT
    uint16_t *buf = (uint16_t *)heap_caps_malloc((size_t)w * h * sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(buf);

    uint16_t test_colors[3] = {0xF800, 0x07E0, 0x001F}; // Red, Green, blue (RGB565)
    const char *color_names[3] = {"do", "xanh la", "xanh duong"};

    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "Test 2.%d: to man hinh mau %s", i + 1, color_names[i]);
        for (int p = 0; p < w * h; p++) {
            buf[p] = test_colors[i];
        }
        lcd->drawBitmap(0, 0, w, h, (uint8_t *)buf);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    free(buf);

    ESP_LOGI(TAG, "Hoan tat test man hinh");
}

```