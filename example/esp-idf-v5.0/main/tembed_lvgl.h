#pragma once

#include "tembed.h"
#include "lvgl.h"

extern lv_disp_drv_t lvgl_disp_drv;
extern lv_disp_t *tembed_lvgl_init(tembed_t tembed);
extern bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
