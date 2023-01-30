/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "math.h"
#include "iot_button.h"
#include "iot_knob.h"
#include "tembed.h"
#include "apa102.h"
#include "tembed_lvgl.h"

#define TAG "tembed"

// Set the number of LEDs to control.
const uint16_t ledCount = CONFIG_APA102_LED_COUNT;

// We define "power" in this sketch to be the product of the
// 8-bit color channel value and the 5-bit brightness register.
// The maximum possible power is 255 * 31 (7905).
const uint16_t maxPower = 255 * 31;

// The power we want to use on the first LED is 1, which
// corresponds to the dimmest possible white.
const uint16_t minPower = 1;

// This function sends a white color with the specified power,
// which should be between 0 and 7905.
void sendWhite(const apa102_t *apa102,uint16_t power)
{
  // Choose the lowest possible 5-bit brightness that will work.
  uint8_t brightness5Bit = 1;
  while(brightness5Bit * 255 < power && brightness5Bit < 31)
  {
    brightness5Bit++;
  }

  // Uncomment this line to simulate an LED strip that does not
  // have the extra 5-bit brightness register.  You will notice
  // that roughly the first third of the LED strip turns off
  // because the brightness8Bit equals zero.
  //brightness = 31;

  // Set brightness8Bit to be power divided by brightness5Bit,
  // rounded to the nearest whole number.
  uint8_t brightness8Bit = (power + (brightness5Bit / 2)) / brightness5Bit;

  // Send the white color to the LED strip.  At this point,
  // brightness8Bit multiplied by brightness5Bit should be
  // approximately equal to power.
  apa102_sendColor(apa102,brightness8Bit, brightness8Bit, brightness8Bit, brightness5Bit);
}

void leds(tembed_t tembed) {
    ESP_LOGI(TAG, "LEDS");
// Calculate what the ratio between the powers of consecutive
// LEDs needs to be in order to reach the max power on the last
// LED of the strip.
    float multiplier = pow(maxPower / minPower, 1.0 / (ledCount - 1));
    apa102_startFrame(&tembed->leds);
    float power = minPower;
    for(uint16_t i = 0; i < ledCount; i++)
    {
        sendWhite(&tembed->leds,power);
        power = power * multiplier;
    }
    apa102_endFrame(&tembed->leds,ledCount);
}

lv_obj_t *count_label;

static void knob_left_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "KNOB: KNOB_LEFT Count is %d", iot_knob_get_count_value((knob_handle_t)arg));
    lv_label_set_text_fmt(count_label,"%d",iot_knob_get_count_value((knob_handle_t)arg));
}

static void knob_right_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "KNOB: KNOB_RIGHT Count is %d", iot_knob_get_count_value((knob_handle_t)arg));
    lv_label_set_text_fmt(count_label,"%d",iot_knob_get_count_value((knob_handle_t)arg));
}

static void button_press_down_cb(void *arg, void *data) {
    ESP_LOGI(TAG, "Down!");
}

void lvgl_demo_ui(lv_disp_t *disp) {
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_disp_get_scr_act(disp), lv_color_hex(0x00FF00), LV_PART_MAIN);

    static lv_style_t l_style;
    lv_style_init(&l_style);
    lv_style_set_width(&l_style, 75);
    lv_style_set_height(&l_style, 40);
    lv_style_set_text_color(&l_style, lv_color_white());
    lv_style_set_text_align(&l_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_bg_color(&l_style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&l_style,LV_OPA_COVER);

    /*Create a container with ROW flex direction*/
    lv_obj_t * cont_row = lv_obj_create(lv_disp_get_scr_act(disp));
    lv_obj_set_size(cont_row, 300, 75);
    lv_obj_align(cont_row, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_color(cont_row, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

    lv_obj_t * label;

    /*Add items to the row*/
    label = lv_label_create(cont_row);
    lv_label_set_text_static(label, "RED");
    lv_obj_add_style(label, &l_style, LV_PART_MAIN);
    lv_obj_set_style_bg_color(label, lv_color_hex(0xff0000), LV_PART_MAIN);
    lv_obj_center(label);

    label = lv_label_create(cont_row);
    lv_label_set_text_static(label, "GREEN");
    lv_obj_add_style(label, &l_style, LV_PART_MAIN);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    lv_obj_center(label);

    label = lv_label_create(cont_row);
    lv_label_set_text_static(label, "BLUE");
    lv_obj_add_style(label, &l_style, LV_PART_MAIN);
    lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    lv_obj_center(label);


    // Create a white label, set its text and align it to the center
    count_label = lv_label_create(lv_disp_get_scr_act(disp));
    lv_label_set_text(count_label, "0");
    lv_obj_set_style_text_color(count_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(count_label, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(count_label, LV_OPA_COVER, LV_PART_MAIN);
}

void app_main(void)
{
    ESP_LOGI(TAG,"Hello lcd!");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%uMB %s flash\n", flash_size / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    // Initialize the T-Embed
    tembed_t tembed = tembed_init(notify_lvgl_flush_ready, &lvgl_disp_drv);

    leds(tembed);

    iot_button_register_cb(tembed->dial.btn, BUTTON_PRESS_DOWN, button_press_down_cb, NULL);

    iot_knob_register_cb(tembed->dial.knob, KNOB_LEFT, knob_left_cb, NULL);
    iot_knob_register_cb(tembed->dial.knob, KNOB_RIGHT, knob_right_cb, NULL);

    lv_disp_t *lvgl_disp = tembed_lvgl_init(tembed);

    ESP_LOGI(TAG, "Display LVGL");
    lvgl_demo_ui(lvgl_disp);

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
