#include "app_configuration.h"
#include "Arduino.h"
#include "pin_config.h"

static void sleep_event_cb(lv_event_t *e);

void app_configuration_load(lv_obj_t *cont) {
  lv_obj_t *btn = lv_btn_create(cont);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_center(btn);
  lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_FOCUS_KEY);
  lv_obj_set_style_bg_color(btn, lv_color_white(), 0);

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_center(label);
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_label_set_text(label, "Deep Sleep");
  lv_obj_add_event_cb(btn, sleep_event_cb, LV_EVENT_CLICKED, NULL);
}

static void sleep_event_cb(lv_event_t *e) {
  digitalWrite(PIN_POWER_ON, 0);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_ENCODE_BTN, 0);
  esp_deep_sleep_start();
}

app_t app_config = {
    .setup_func_cb = app_configuration_load,
    .exit_func_cb = nullptr,
    .user_data = nullptr,
};