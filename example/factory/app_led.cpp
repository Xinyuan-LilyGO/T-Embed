#include "app_led.h"
#include "Arduino.h"

static void change_led_event_cb(lv_event_t *e);
static struct {
  uint8_t brightness;
  uint8_t mode;
  lv_obj_t *slider;
  lv_obj_t *roller;
} led_param;

extern QueueHandle_t led_setting_queue;

void app_led_load(lv_obj_t *cont) {

  led_param.slider = lv_slider_create(cont);
  lv_obj_align(led_param.slider, LV_ALIGN_LEFT_MID, 15, 0);
  lv_slider_set_range(led_param.slider, 0, 31);
  lv_obj_set_width(led_param.slider, 150);
  lv_slider_set_value(led_param.slider, led_param.brightness, LV_ANIM_ON);

  lv_obj_t *label = lv_label_create(cont);
  lv_obj_align_to(label, led_param.slider, LV_ALIGN_OUT_TOP_LEFT, 20, -10);
  lv_label_set_text(label, "Brightness setting");
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_refresh_ext_draw_size(led_param.slider);

  led_param.roller = lv_roller_create(cont);
  lv_roller_set_options(led_param.roller,
                        "lights off\n"
                        "Gradient\n"
                        "rotate\n"
                        "independent\n"
                        "White\n"
                        "Orange",
                        LV_ROLLER_MODE_NORMAL);
  lv_roller_set_selected(led_param.roller, led_param.mode, LV_ANIM_ON);
  lv_roller_set_visible_row_count(led_param.roller, 4);
  lv_obj_align(led_param.roller, LV_ALIGN_RIGHT_MID, -5, 0);
  lv_obj_add_event_cb(led_param.roller, change_led_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(led_param.slider, change_led_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void change_led_event_cb(lv_event_t *e) {
  led_param.brightness = lv_slider_get_value(led_param.slider);
  led_param.mode = lv_roller_get_selected(led_param.roller);
  uint16_t temp = (led_param.mode & 0x0f) << 6 | led_param.brightness;
  xQueueSend(led_setting_queue, &temp, 0);
}


app_t app_led = {
    .setup_func_cb = app_led_load,
    .exit_func_cb = nullptr,
    .user_data= nullptr,
};