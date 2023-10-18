#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl.h"
#include "tembed.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "assert.h"

#define TAG "lvgl"

// LVGL update interval
#define LVGL_TICK_PERIOD_MS 2

static bool lvgl_init_done = false;

bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    if(!lvgl_init_done) return false;
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    }
}

static void increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
lv_disp_drv_t lvgl_disp_drv;      // contains callback functions

lv_disp_t *tembed_lvgl_init(tembed_t tembed) {

    ESP_LOGI(TAG, "Init");
    lv_init();

    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(TEMBED_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(TEMBED_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, TEMBED_LCD_H_RES * 20);

    ESP_LOGI(TAG, "Register display");

    lv_disp_drv_init(&lvgl_disp_drv);
    lvgl_disp_drv.hor_res = TEMBED_LCD_H_RES;
    lvgl_disp_drv.ver_res = TEMBED_LCD_V_RES;
    lvgl_disp_drv.flush_cb = lvgl_flush_cb;
    lvgl_disp_drv.drv_update_cb = lvgl_port_update_callback;
    lvgl_disp_drv.draw_buf = &disp_buf;
    lvgl_disp_drv.user_data = tembed->lcd;

    lv_disp_t *disp = lv_disp_drv_register(&lvgl_disp_drv);
    assert(disp!=NULL);

    ESP_LOGI(TAG, "Tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    // Ensure the coordiate systems align with the physical display
    lv_disp_set_rotation(disp, LV_DISP_ROT_270);

    lvgl_init_done = true;

    return disp;
}
