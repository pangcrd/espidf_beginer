
#include "bsp_sd.h"
#include "bsp_fs.h"

bool print_entry(const char *name, bool is_dir, size_t size, void *ctx)
{
    printf("%s %s (%u B)\n", is_dir ? "[D]" : "[F]", name, (unsigned)size);
    return true;
}
extern "C" void app_main(void)
{
    bsp_sd_config_t sd = {
        .clk = 38, .cmd = 40,
        .d0 = 39, .d1 = 41, .d2 = 48, .d3 = 47,
        .width = 4,
        .max_freq_khz = SDMMC_FREQ_HIGHSPEED,   // lỗi thì hạ SDMMC_FREQ_DEFAULT
        .internal_pullup = true,
    };
    if (bsp_sd_mount(&sd) == ESP_OK) {
        bsp_sd_bench(8 * 1024 * 1024, 32 * 1024);
    }

    //bsp_fs_create_dir("/sdcard/images");
    bsp_fs_create_file("/sdcard/images/hello.txt", "hi", 2);
    //bsp_fs_create_dir_recursive("/sdcard/a/b/c");
    //bsp_fs_list_dir("/sdcard", print_entry, NULL);
    //bsp_fs_delete_recursive("/sdcard/images");
}
