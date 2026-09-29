#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t *pixels;
    uint16_t width;
    uint16_t height;
} jpeg_image_t;

esp_err_t jpeg_dec_get_info(const uint8_t *data, size_t len, uint16_t *w, uint16_t *h);

// out_size phải >= giá trị lấy từ jpeg_dec_get_info + jpeg_dec_get_outbuf_len (dùng jpeg_dec_file để tự lo phần này)
esp_err_t jpeg_dec_buffer(const uint8_t *data, size_t len, bool swap_bytes,
                          uint8_t *out, size_t out_size, uint16_t *w, uint16_t *h);

esp_err_t jpeg_dec_file(const char *path, bool swap_bytes, jpeg_image_t *out);

void jpeg_image_free(jpeg_image_t *img);

#ifdef __cplusplus
}
#endif