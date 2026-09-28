#include "bsp_sd.h"
#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "sd_bench";

esp_err_t bsp_sd_bench(size_t total, size_t blk)
{
    uint8_t *buf = heap_caps_malloc(blk, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf) return ESP_ERR_NO_MEM;
    memset(buf, 0xA5, blk);

    const char *path = BSP_SD_MOUNT_POINT "/bench.bin";

    FILE *f = fopen(path, "wb");
    if (!f) { free(buf); return ESP_FAIL; }
    int64_t t0 = esp_timer_get_time();
    for (size_t n = 0; n < total; n += blk) fwrite(buf, 1, blk, f);
    fclose(f);
    int64_t t1 = esp_timer_get_time();
    ESP_LOGI(TAG, "WRITE: %.2f MB/s", total / (double)(t1 - t0));   // bytes/us = MB/s

    f = fopen(path, "rb");
    if (!f) { free(buf); return ESP_FAIL; }
    t0 = esp_timer_get_time();
    while (fread(buf, 1, blk, f) == blk) {}
    fclose(f);
    t1 = esp_timer_get_time();
    ESP_LOGI(TAG, "READ:  %.2f MB/s", total / (double)(t1 - t0));

    remove(path);
    free(buf);
    return ESP_OK;
}