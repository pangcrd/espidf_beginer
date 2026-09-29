```cpp
#include "esp_display_panel.hpp"
#include "esp_lib_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include "bsp_sd.h"
#include "bsp_fs.h"
#include "jpeg_dec.h"

#define PANEL_SWAP_BYTES true

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char *TAG = "tft_sdcard_test";

// Compare FAT read sizes on the same file and its actual allocated location.
static void fat_file_read_bench(const char *path)
{
    static const size_t chunks[] = {512, 4096, 8192, 32768, 65536};
    uint8_t *buf = (uint8_t *)heap_caps_malloc(chunks[sizeof(chunks) / sizeof(chunks[0]) - 1],
                                               MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf) {
        ESP_LOGE(TAG, "FAT benchmark buffer allocation failed");
        return;
    }

    for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]); ++i) {
        FILE *f = fopen(path, "rb");
        if (!f) {
            ESP_LOGE(TAG, "FAT benchmark open failed: %s", path);
            break;
        }
        if (fseek(f, 0, SEEK_END) != 0) {
            fclose(f);
            ESP_LOGE(TAG, "FAT benchmark seek failed: %s", path);
            break;
        }
        long expected = ftell(f);
        if (expected <= 0 || fseek(f, 0, SEEK_SET) != 0) {
            fclose(f);
            ESP_LOGE(TAG, "FAT benchmark size/rewind failed: %s", path);
            break;
        }
        int64_t t0 = esp_timer_get_time();
        size_t total = 0;
        while (1) {
            size_t n = fread(buf, 1, chunks[i], f);
            total += n;
            if (n < chunks[i]) break;
        }
        bool failed = ferror(f);
        if (fclose(f) != 0) failed = true;
        int64_t elapsed = esp_timer_get_time() - t0;
        if (failed || total != (size_t)expected || elapsed <= 0) {
            ESP_LOGE(TAG, "FAT benchmark failed: chunk=%u bytes=%u", (unsigned)chunks[i], (unsigned)total);
            break;
        }
        ESP_LOGI(TAG, "FAT read: chunk=%u bytes=%u time=%lld ms rate=%.2f MB/s",
                 (unsigned)chunks[i], (unsigned)total, elapsed / 1000,
                 total / ((double)elapsed));
    }
    heap_caps_free(buf);
}

extern "C" void app_main(void)
{
    // --- SD ---
    bsp_sd_config_t sd = {
        .clk = 38, .cmd = 40,
        .d0 = 39, .d1 = 41, .d2 = 48, .d3 = 47,
        .width = 4,
        .max_freq_khz = SDMMC_FREQ_HIGHSPEED,   // lỗi thì hạ SDMMC_FREQ_DEFAULT
        .internal_pullup = true,
    };
    if (bsp_sd_mount(&sd) != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
        return;
    }
    // Compare FAT file reads against raw SDMMC reads after mount when profiling.
    // raw_sector_bench(bsp_sd_get_card());
    // bsp_sd_bench(4 * 1024 * 1024, 64 * 1024);
    // fat_file_read_bench("/sdcard/camellya.jpg");

    // --- Display ---
    ESP_LOGI(TAG, "display init");
    Board *board = new Board();
    assert(board);
    ESP_UTILS_CHECK_FALSE_EXIT(board->init(), "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");

    auto lcd = board->getLCD();

    lcd->swapXY(true);
    lcd->mirrorX(true);    
    lcd->mirrorY(true);

    const int w = 320;
    const int h = 240;
    ESP_LOGI(TAG, "panel %dx%d", w, h);

    // JPEG Images camellya.jpg
    uint8_t *jpg = nullptr; size_t sz = 0;
    int64_t t0 = esp_timer_get_time();
    esp_err_t read_ret = bsp_fs_read_file_psram("/sdcard/camellya.jpg", &jpg, &sz);
    int64_t t_read = esp_timer_get_time() - t0;
    if (read_ret != ESP_OK) {
        ESP_LOGE(TAG, "image read failed: %s", esp_err_to_name(read_ret));
        return;
    }

    uint16_t iw, ih;
    if (jpeg_dec_get_info(jpg, sz, &iw, &ih) != ESP_OK) {
        ESP_LOGE(TAG, "invalid/unsupported JPEG");
        heap_caps_free(jpg);
        return;
    }
    uint8_t *pix = (uint8_t *)heap_caps_aligned_alloc(16, (size_t)iw * ih * 2, MALLOC_CAP_DMA);
    if (!pix) {
        ESP_LOGE(TAG, "pixel buffer allocation failed");
        heap_caps_free(jpg);
        return;
    }
    int64_t sum_dec = 0, sum_draw = 0;
    const int N = 20;
    for (int i = 0; i < N; i++) {
        t0 = esp_timer_get_time();
        esp_err_t dec_ret = jpeg_dec_buffer(jpg, sz, PANEL_SWAP_BYTES, pix,
                                            (size_t)iw * ih * 2, &iw, &ih);
        if (dec_ret != ESP_OK) {
            ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(dec_ret));
            break;
        }
        sum_dec += esp_timer_get_time() - t0;

        t0 = esp_timer_get_time();
        lcd->drawBitmap(0, 0, iw, ih, pix);
        sum_draw += esp_timer_get_time() - t0;
        vTaskDelay(3);
    }
    ESP_LOGI(TAG, "file %u KB | read %lld ms | decode %lld ms | draw %lld ms",
            (unsigned)(sz / 1024), t_read / 1000, sum_dec / N / 1000, sum_draw / N / 1000); 
    heap_caps_free(pix);
    heap_caps_free(jpg);
    
}

```