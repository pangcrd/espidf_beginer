#include "bsp_fs.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "bsp_fs";

bool bsp_fs_exists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0;
}

bool bsp_fs_is_dir(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

esp_err_t bsp_fs_file_size(const char *path, size_t *out_size)
{
    struct stat st;
    if (!path || !out_size) return ESP_ERR_INVALID_ARG;
    if (stat(path, &st) != 0) return ESP_ERR_NOT_FOUND;
    *out_size = st.st_size;
    return ESP_OK;
}

static esp_err_t write_file(const char *path, const void *data, size_t len, const char *mode)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    FILE *f = fopen(path, mode);
    if (!f) {
        ESP_LOGE(TAG, "open %s failed: errno %d", path, errno);
        return ESP_FAIL;
    }
    esp_err_t ret = ESP_OK;
    if (len && fwrite(data, 1, len, f) != len) ret = ESP_FAIL;
    if (fclose(f) != 0) ret = ESP_FAIL;
    return ret;
}

esp_err_t bsp_fs_create_file(const char *path, const void *data, size_t len)
{
    return write_file(path, data, len, "wb");
}

esp_err_t bsp_fs_append_file(const char *path, const void *data, size_t len)
{
    return write_file(path, data, len, "ab");
}

esp_err_t bsp_fs_delete_file(const char *path)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    if (unlink(path) != 0) {
        return errno == ENOENT ? ESP_ERR_NOT_FOUND : ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t bsp_fs_create_dir(const char *path)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    if (mkdir(path, 0775) == 0) return ESP_OK;
    if (errno == EEXIST && bsp_fs_is_dir(path)) return ESP_OK;
    ESP_LOGE(TAG, "mkdir %s failed: errno %d", path, errno);
    return ESP_FAIL;
}

esp_err_t bsp_fs_delete_dir(const char *path)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    if (rmdir(path) != 0) {
        return errno == ENOENT ? ESP_ERR_NOT_FOUND : ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t bsp_fs_delete_recursive(const char *path)
{
    if (!path) return ESP_ERR_INVALID_ARG;
    if (!bsp_fs_is_dir(path)) return bsp_fs_delete_file(path);

    DIR *d = opendir(path);
    if (!d) return ESP_FAIL;

    // Cấp phát path trên heap, tránh tràn stack khi đệ quy sâu
    size_t plen = strlen(path);
    char *child = malloc(plen + 1 + 256 + 1);
    if (!child) { closedir(d); return ESP_ERR_NO_MEM; }

    esp_err_t ret = ESP_OK;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(child, plen + 1 + 256 + 1, "%s/%s", path, e->d_name);
        ret = bsp_fs_delete_recursive(child);
        if (ret != ESP_OK) break;
    }
    free(child);
    closedir(d);

    return ret == ESP_OK ? bsp_fs_delete_dir(path) : ret;
}

esp_err_t bsp_fs_list_dir(const char *path, bsp_fs_list_cb_t cb, void *ctx)
{
    if (!path || !cb) return ESP_ERR_INVALID_ARG;
    DIR *d = opendir(path);
    if (!d) return ESP_ERR_NOT_FOUND;

    size_t plen = strlen(path);
    char *full = malloc(plen + 1 + 256 + 1);
    if (!full) { closedir(d); return ESP_ERR_NO_MEM; }

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(full, plen + 1 + 256 + 1, "%s/%s", path, e->d_name);
        struct stat st;
        bool is_dir = false;
        size_t size = 0;
        if (stat(full, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
            size = st.st_size;
        }
        if (!cb(e->d_name, is_dir, size, ctx)) break;
    }
    free(full);
    closedir(d);
    return ESP_OK;
}