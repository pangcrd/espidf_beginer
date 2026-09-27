/**
 * main.cpp
 * ESP-IDF ver 5.5.5
 * Test LVGL v9.5 tren man hinh 172x320 (ST7789),ESP32C3
 * Youtube:https://www.youtube.com/@pangcrd
 * Github:https://github.com/pangcrd
 */

#include "esp_display_panel.hpp"
#include "esp_lib_utils.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "lv_examples.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char *TAG = "lvgl_hello";

LV_IMG_DECLARE(espressif_img);
LV_IMG_DECLARE(lvgl_img);
LV_IMG_DECLARE(youtube_img);

lv_obj_t * screen1 = NULL;
lv_obj_t * screen2 = NULL;

lv_obj_t * espressif_logo = NULL;
lv_obj_t * lvgl_logo = NULL;
lv_obj_t * youtube_logo = NULL;
lv_obj_t * subs = NULL;
lv_obj_t * like = NULL;

void screen1_init(void){
    screen1 = lv_obj_create(NULL);
    lv_obj_remove_flag(screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(screen1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    espressif_logo = lv_image_create(screen1);
    lv_image_set_src(espressif_logo, &espressif_img);
    lv_obj_set_width(espressif_logo, LV_SIZE_CONTENT); 
    lv_obj_set_height(espressif_logo, LV_SIZE_CONTENT); 
    lv_obj_set_x(espressif_logo, 22);
    lv_obj_set_y(espressif_logo, 25);
 
    lvgl_logo = lv_image_create(screen1);
    lv_image_set_src(lvgl_logo, &lvgl_img);
    lv_obj_set_width(lvgl_logo, LV_SIZE_CONTENT);
    lv_obj_set_height(lvgl_logo, LV_SIZE_CONTENT);
    lv_obj_set_x(lvgl_logo, 25);
    lv_obj_set_y(lvgl_logo, 179);

    lv_obj_set_style_pad_row(screen1, 40, 0);
}
void screen2_init(void){
    screen2 = lv_obj_create(NULL);
    lv_obj_remove_flag(screen2, LV_OBJ_FLAG_SCROLLABLE);

    like = lv_label_create(screen2);
    lv_obj_set_width(like, LV_SIZE_CONTENT);
    lv_obj_set_height(like, LV_SIZE_CONTENT);
    lv_obj_set_x(like, 55);
    lv_obj_set_y(like, 40);
    lv_label_set_text(like, "LIKE");
    lv_obj_set_style_text_color(like, lv_color_hex(0x00000), LV_PART_MAIN);
    lv_obj_set_style_text_font(like, &lv_font_montserrat_30, LV_PART_MAIN);

    subs = lv_label_create(screen2);
    lv_obj_set_width(subs, LV_SIZE_CONTENT);
    lv_obj_set_height(subs, LV_SIZE_CONTENT);
    lv_obj_set_x(subs, 10);
    lv_obj_set_y(subs, 100);
    lv_label_set_text(subs, "SUBSCRIBE");
    lv_obj_set_style_text_color(subs, lv_color_hex(0xf3001d), LV_PART_MAIN);
    lv_obj_set_style_text_font(subs, &lv_font_montserrat_26, LV_PART_MAIN);

    youtube_logo = lv_image_create(screen2);
    lv_image_set_src(youtube_logo, &youtube_img);
    lv_obj_set_width(youtube_logo, LV_SIZE_CONTENT);
    lv_obj_set_height(youtube_logo, LV_SIZE_CONTENT);
    lv_obj_set_x(youtube_logo, 22);
    lv_obj_set_y(youtube_logo, 158);
}
static void switch_to_screen2_cb(lv_timer_t * timer){
    lv_screen_load(screen2);
    lv_timer_delete(timer);  
}
extern "C" void app_main(void)
{
    // Init display
    ESP_LOGI(TAG, "Khoi tao board");
    Board *board = new Board();
    assert(board);
    ESP_UTILS_CHECK_FALSE_EXIT(board->init(), "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");

    auto lcd = board->getLCD();
    auto backlight = board->getBacklight();
    backlight->setBrightness(10);   // 0-100 (%)

    // handle esp_lvgl_port
    esp_lcd_panel_io_handle_t io_handle = lcd->getBus()->getControlPanelHandle();
    esp_lcd_panel_handle_t panel_handle = lcd->getRefreshPanelHandle();

    // ==== Init LVGL port ====
    ESP_LOGI(TAG, "Khoi tao LVGL port");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = 172 * 20,        // buffer.<giá trị càng nhỏ càng đỡ tốn RAM, nên đặt mức tối thiểu là 20>
        .double_buffer = true,
        .hres = 172,                    // ESP_PANEL_BOARD_WIDTH
        .vres = 320,                    // ESP_PANEL_BOARD_HEIGHT
        .monochrome = false,
        .rotation = {
            .swap_xy = false,           
            .mirror_x = false,         
            .mirror_y = false,         
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = true,
            .full_refresh = false,
            .direct_mode = false,
        }
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    assert(disp);

    // Run ui demo
    lvgl_port_lock(0);
      //lv_example_image_3();   
      //lv_example_anim_2();    
      //lv_example_animimg_1();  
      //lv_example_bar_3();     
      //lv_example_image_4();
      screen1_init();
      screen2_init();
      lv_screen_load(screen1); 
      lv_timer_create(switch_to_screen2_cb, 5000, NULL);   
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Hello world LVGL v9");
}