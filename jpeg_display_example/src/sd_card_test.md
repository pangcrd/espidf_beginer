```C
#include "bsp_sd.h"
#include "bsp_fs.h"
#include "esp_log.h"

static const char *TAG = "sdcard_test";

void app_main() {
    bsp_sd_config_t sd = {
        .clk = 38, .cmd = 40,
        .d0 = 39, .d1 = 41, .d2 = 48, .d3 = 47,
        .width = 4, //4 bit mode
        .max_freq_khz = SDMMC_FREQ_HIGHSPEED, // 40MHZ
        //.max_freq_khz = SDMMC_FREQ_DEFAULT,  //  20MHZ
        .internal_pullup = true,
    };
    
    if (bsp_sd_mount(&sd) != ESP_OK) {
        //bsp_sd_bench(8 * 1024 * 1024, 32 * 1024); // Test read/write sd speed
        ESP_LOGE (TAG, "SD mount failed!");
        return;
    }

    //bsp_fs_create_dir("/sdcard/images");
    bsp_fs_create_file_p("/sdcard/images/hello.txt", "hi", 2);
    //bsp_fs_create_dir_recursive("/sdcard/a/b/c");
    //bsp_fs_list_dir("/sdcard", print_entry, NULL);
    //bsp_fs_delete_recursive("/sdcard/images");
}

```