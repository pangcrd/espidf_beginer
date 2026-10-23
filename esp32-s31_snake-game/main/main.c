/* Snake game: 8x8 WS2812B panel (DIN = GPIO4, progressive layout)
 * + joystick component. Walls wrap; self-bite loses (red X);
 * joystick button restarts. main/CMakeLists.txt: SRCS main_snake.c,
 * REQUIRES joystick led_strip. */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"  
#include "led_strip.h"
#include "joystick.h"

static const char *TAG = "snake";

#define MATRIX_W      8
#define MATRIX_H      8
#define MATRIX_LEDS   (MATRIX_W * MATRIX_H)
#define MATRIX_GPIO   4

#define LED_BRIGHTNESS  40
#define SNAKE_INVERT_X  0

#define POLL_MS       50      /* moi vong doc joystick 1 lan */
#define START_STEP_MS 280     /* toc do ban dau: 280ms / 1 o */
#define MIN_STEP_MS   120     /* an nhieu thi nhanh dan, toi da 120ms / 1 o */
#define STEP_PER_FOOD 10      /* moi mieng moi nhanh them 10ms */

typedef struct {
    int x;
    int y;
} point_t;

typedef enum {
    SNAKE_PLAYING,
    SNAKE_GAME_OVER,
} snake_state_t;

/* Ran: snake[0] la dau, snake[len-1] la duoi. */
static point_t snake[MATRIX_LEDS];
static int snake_len = 0;
static int dir_x = 1, dir_y = 0;             /* huong dang chay */
static int pending_x = 1, pending_y = 0;     /* huong cho doi ap o buoc toi */
static point_t food = { -1, -1 };
static int food_color_idx = -1;
static int foods_eaten = 0;
static int step_ms = START_STEP_MS;
static snake_state_t state = SNAKE_PLAYING;

/* Mau moi: khong co mau xanh la (de khoi lan voi than ran).
 * Moi mieng moi se boc ngau nhien 1 mau trong bang nay, khac
 * mau cua mieng truoc do. */
static const uint8_t food_colors[][3] = {
    {90, 0, 0},     /* do */
    {0, 0, 90},     /* lam */
    {90, 90, 0},    /* vang */
    {90, 0, 90},    /* tim */
    {0, 90, 90},    /* cyan */
    {90, 45, 0},    /* cam */
};
#define NUM_FOOD_COLORS  (sizeof(food_colors) / sizeof(food_colors[0]))

static int xy_to_index(int x, int y)
{
    return y * MATRIX_W + x;  /* kieu tien, da xac nhan 2026-10-22 */
}

static void set_pixel(led_strip_handle_t strip, int idx,
                      uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(strip, idx,
                        (uint8_t)(r * LED_BRIGHTNESS / 255),
                        (uint8_t)(g * LED_BRIGHTNESS / 255),
                        (uint8_t)(b * LED_BRIGHTNESS / 255));
}

static bool point_on_snake(int x, int y)
{
    for (int i = 0; i < snake_len; i++) {
        if (snake[i].x == x && snake[i].y == y) {
            return true;
        }
    }
    return false;
}

/* Sinh moi moi: vi tri ngau nhien, khong trung than ran,
 * mau ngau nhien, khac mau mieng truoc. */
static void spawn_food(void)
{
    if (snake_len >= MATRIX_LEDS) {
        food.x = -1;  /* ban kin, khong con cho dat moi */
        food.y = -1;
        return;
    }
    do {
        food.x = (int)(esp_random() % MATRIX_W);
        food.y = (int)(esp_random() % MATRIX_H);
    } while (point_on_snake(food.x, food.y));

    int idx;
    do {
        idx = (int)(esp_random() % NUM_FOOD_COLORS);
    } while (idx == food_color_idx && NUM_FOOD_COLORS > 1);
    food_color_idx = idx;
}

static void reset_game(void)
{
    snake_len = 3;
    snake[0].x = 3; snake[0].y = 4;  /* dau */
    snake[1].x = 2; snake[1].y = 4;
    snake[2].x = 1; snake[2].y = 4;  /* duoi */
    dir_x = 1; dir_y = 0;
    pending_x = 1; pending_y = 0;
    foods_eaten = 0;
    step_ms = START_STEP_MS;
    food_color_idx = -1;
    spawn_food();
    state = SNAKE_PLAYING;
}

static void render(led_strip_handle_t strip)
{
    led_strip_clear(strip);

    /* Moi. */
    if (food.x >= 0) {
        set_pixel(strip, xy_to_index(food.x, food.y),
                  food_colors[food_color_idx][0],
                  food_colors[food_color_idx][1],
                  food_colors[food_color_idx][2]);
    }

    /* Ran: dau sang hon than. */
    for (int i = 0; i < snake_len; i++) {
        if (i == 0) {
            set_pixel(strip, xy_to_index(snake[i].x, snake[i].y),
                      0, 200, 0);
        } else {
            set_pixel(strip, xy_to_index(snake[i].x, snake[i].y),
                      0, 70, 0);
        }
    }
    led_strip_refresh(strip);
}

/* Ve dau X khi thua: 2 duong cheo cua bang 8x8. */
static void render_game_over(led_strip_handle_t strip)
{
    led_strip_clear(strip);
    for (int i = 0; i < MATRIX_W; i++) {
        set_pixel(strip, xy_to_index(i, i), 120, 0, 0);
        set_pixel(strip, xy_to_index(i, MATRIX_W - 1 - i), 120, 0, 0);
    }
    led_strip_refresh(strip);
}

/* Nhan huong moi tu joystick, chi 1 truc, khong cho quay nguoc.
 * Truc nao gat manh hon thi thang; gat cheo deu nhau thi giu
 * huong cu de khoi giat. Huong moi chi la "cho doi": den buoc
 * di tiep theo moi ap dung, nen gat nhanh 2 lan (vi du phai ->
 * len -> trai) cung khong bi quay nguoc ngay lap tuc. */
static void update_direction(int x_norm, int y_norm)
{
    int nx = 0, ny = 0;
    if (abs(x_norm) > abs(y_norm)) {
        nx = (x_norm > 0) ? 1 : -1;
    } else if (abs(y_norm) > abs(x_norm)) {
        ny = (y_norm > 0) ? 1 : -1;
    } else {
        return;
    }
#if SNAKE_INVERT_X
    /* Dao truc X rieng cho game nay, ap truoc khi kiem tra
     * quay nguoc, de huong sau khi dao moi la huong tinh. */
    if (nx != 0) {
        nx = -nx;
    }
#endif
    /* Quay nguoc 180 do so voi huong dang chay thi bo qua. */
    if (nx == -dir_x && ny == -dir_y) {
        return;
    }
    pending_x = nx;
    pending_y = ny;
}

/* Di 1 buoc. Tra ve true neu van song, false neu thua. */
static bool move_snake(void)
{
    dir_x = pending_x;
    dir_y = pending_y;

    point_t head = { snake[0].x + dir_x, snake[0].y + dir_y };

    /* Xuyen tuong: qua mep thi vong lai mep doi dien, khong thua.
     * Chi can than minh moi thua (kiem tra o duoi). */
    if (head.x < 0) {
        head.x = MATRIX_W - 1;
    } else if (head.x >= MATRIX_W) {
        head.x = 0;
    }
    if (head.y < 0) {
        head.y = MATRIX_H - 1;
    } else if (head.y >= MATRIX_H) {
        head.y = 0;
    }

    bool will_eat = (head.x == food.x && head.y == food.y);

    /* Can than minh. Neu khong an moi, duoi se roi di nen khong
     * tinh dot duoi cuoi vao va cham. */
    int check_count = will_eat ? snake_len : snake_len - 1;
    for (int i = 0; i < check_count; i++) {
        if (head.x == snake[i].x && head.y == snake[i].y) {
            ESP_LOGI(TAG, "thua: can than minh, diem=%d", foods_eaten);
            return false;
        }
    }

    /* Dich toan bo than ve phia duoi, gan dau moi vao.
     * Neu khong an moi, chi dich toi dot ke duoi (dot duoi cu
     * roi di), khong dich qua khoi mang khi ran da kin 64 o. */
    int shift_from = (will_eat && snake_len < MATRIX_LEDS) ? snake_len : snake_len - 1;
    for (int i = shift_from; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = head;

    if (will_eat) {
        if (snake_len < MATRIX_LEDS) {
            snake_len++;
        }
        foods_eaten++;
        if (step_ms > MIN_STEP_MS + STEP_PER_FOOD) {
            step_ms -= STEP_PER_FOOD;
        } else {
            step_ms = MIN_STEP_MS;
        }
        ESP_LOGI(TAG, "an moi! diem=%d dai=%d toc_do=%dms/o",
                 foods_eaten, snake_len, step_ms);
        spawn_food();
    }
    return true;
}

void app_main(void)
{
    /* --- Khoi tao panel --- */
    led_strip_config_t strip_config = {
        .strip_gpio_num = MATRIX_GPIO,
        .max_leds = MATRIX_LEDS,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = { .invert_out = false },
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags = { .with_dma = false },
    };
    led_strip_handle_t strip = NULL;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));

    /* --- Khoi tao joystick (qua component) --- */
    joystick_handle_t joystick = NULL;
    ESP_ERROR_CHECK(joystick_init_default(&joystick));
    ESP_LOGI(TAG, "joystick_init OK, giu joystick o giua de calibrate...");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_ERROR_CHECK(joystick_calibrate(joystick, JOYSTICK_CAL_SAMPLES));

    reset_game();
    render(strip);
    ESP_LOGI(TAG, "vao game: gat de lai ran, xuyen tuong, chi can minh moi thua");

    bool button_prev = false;
    int move_accum_ms = 0;

    while (1) {
        int x_norm, y_norm;
        bool button_down = false;
        /* Moi vong chi doc 1 lan. */
        esp_err_t err = joystick_read_normalized(joystick, &x_norm, &y_norm, &button_down);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(POLL_MS));
            continue;
        }

        /* Canh len cua nut: chi tinh 1 lan moi lan bam. */
        bool button_pressed = button_down && !button_prev;
        button_prev = button_down;

        if (state == SNAKE_GAME_OVER) {
            /* Chi cho bam nut de choi lai, gat joystick luc nay khong lam gi. */
            if (button_pressed) {
                reset_game();
                render(strip);
                move_accum_ms = 0;
                ESP_LOGI(TAG, "choi lai!");
            }
            vTaskDelay(pdMS_TO_TICKS(POLL_MS));
            continue;
        }

        update_direction(x_norm, y_norm);

        move_accum_ms += POLL_MS;
        if (move_accum_ms >= step_ms) {
            move_accum_ms = 0;
            if (!move_snake()) {
                state = SNAKE_GAME_OVER;
                render_game_over(strip);
                ESP_LOGI(TAG, "GAME OVER - bam nut de choi lai");
            } else {
                render(strip);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}
