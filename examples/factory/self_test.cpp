#include "self_test.h"
#include "Arduino.h"
#include "SD_MMC.h"
#include "WiFi.h"
#include "pin_config.h"
#include "esp_sntp.h"
#include "time.h"
#include <lvgl.h>

#ifndef LV_DELAY
#define LV_DELAY(x)   {uint32_t t = x; do { lv_timer_handler();delay(1);} while (t--);}
#endif

LV_IMG_DECLARE(lilygo2_gif);
LV_FONT_DECLARE(alibaba_font_60);

static lv_point_t line_points[] = {{-320, 0}, {320, 0}};
static void ui_begin();
static lv_obj_t *dis;
static bool _is_self_check_completed = false;
static void timer_task(lv_timer_t *t) {
  lv_obj_t *seg = (lv_obj_t *)t->user_data;
  static bool j;
  if (j)
    lv_obj_add_flag(seg, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_clear_flag(seg, LV_OBJ_FLAG_HIDDEN);
  j = !j;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    lv_msg_send(MSG_NEW_HOUR, &timeinfo.tm_hour);
    lv_msg_send(MSG_NEW_MIN, &timeinfo.tm_min);
  }
  uint32_t volt = (analogRead(PIN_BAT_VOLT) * 2 * 3.3 * 1000) / 4096;
  lv_msg_send(MSG_NEW_VOLT, &volt);
}

static void update_text_subscriber_cb(lv_event_t *e) {
  lv_obj_t *label = lv_event_get_target(e);
  lv_msg_t *m = lv_event_get_msg(e);

  const char *fmt = (const char *)lv_msg_get_user_data(m);
  const int32_t *v = (const int32_t *)lv_msg_get_payload(m);

  lv_label_set_text_fmt(label, fmt, *v);
}
void ui_switch_page(void) {
  static uint8_t n;
  n++;
  lv_obj_set_tile_id(dis, 0, n % UI_PAGE_COUNT, LV_ANIM_ON);
}

void self_test(void) {
  lv_obj_t *log_label = lv_label_create(lv_scr_act());
  lv_obj_align(log_label, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_width(log_label, LV_PCT(100));
  lv_label_set_text(log_label, "Scan WiFi");
  lv_label_set_long_mode(log_label, LV_LABEL_LONG_SCROLL);
  lv_label_set_recolor(log_label, true);
  Serial.println("Scan WiFi");
  
  LV_DELAY(100);

  sntp_servermode_dhcp(1); // (optional)
  configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("Scan Done");
  String text;
  if (n == 0) {
    text = "no networks found";
  } else {
    text = n;
    text += " networks found\n";
    for (int i = 0; i < n; ++i) {
      text += (i + 1);
      text += ": ";
      text += WiFi.SSID(i);
      text += " (";
      text += WiFi.RSSI(i);
      text += ")";
      text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
      delay(10);
    }
  }
  lv_label_set_text(log_label, text.c_str());
  Serial.println(text);
  LV_DELAY(2000);
  text = "Connecting to ";
  Serial.print("Connecting to ");
  text += WIFI_SSID;
  text += "\n";
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t last_tick = millis();
  bool is_smartconfig_connect = false;
  lv_label_set_long_mode(log_label, LV_LABEL_LONG_WRAP);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    text += ".";
    lv_label_set_text(log_label, text.c_str());
    LV_DELAY(100);
    if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX) { /* Automatically start smartconfig when connection times out */
      text += "\nConnection timed out, start smartconfig";
      lv_label_set_text(log_label, text.c_str());
      LV_DELAY(100);
      is_smartconfig_connect = true;
      WiFi.mode(WIFI_AP_STA);
      Serial.println("\r\n wait for smartconfig....");
      text += "\r\n wait for smartconfig....";
      text += "\nPlease use #ff0000 EspTouch# Apps to connect to the distribution network";
      lv_label_set_text(log_label, text.c_str());
      WiFi.beginSmartConfig();
      while (1) {
        LV_DELAY(100);
        if (WiFi.smartConfigDone()) {
          Serial.println("\r\nSmartConfig Success\r\n");
          Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
          Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
          text += "\nSmartConfig Success";
          text += "\nSSID:";
          text += WiFi.SSID().c_str();
          text += "\nPSW:";
          text += WiFi.psk().c_str();
          lv_label_set_text(log_label, text.c_str());
          LV_DELAY(1000);
          last_tick = millis();
          break;
        }
      }
    }
  }
  if (!is_smartconfig_connect) {
    text += "\nCONNECTED \nTakes ";
    Serial.print("\n CONNECTED \nTakes ");
    text += millis() - last_tick;
    Serial.print(millis() - last_tick);
    text += " ms\n";
    Serial.println(" millseconds");
    lv_label_set_text(log_label, text.c_str());
  }
  LV_DELAY(2000);
  ui_begin();
  _is_self_check_completed = true;
}

static void ui_begin() {
  dis = lv_tileview_create(lv_scr_act());
  lv_obj_align(dis, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_size(dis, LV_PCT(100), LV_PCT(100));
  lv_obj_t *tv1 = lv_tileview_add_tile(dis, 0, 0, LV_DIR_HOR);
  lv_obj_t *tv2 = lv_tileview_add_tile(dis, 0, 1, LV_DIR_HOR);
  /* page 1 */
  lv_obj_t *main_cout = lv_obj_create(tv1);
  lv_obj_set_size(main_cout, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(main_cout, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(main_cout, 0, 0);
  lv_obj_set_style_bg_color(main_cout, UI_BG_COLOR, 0);

  lv_obj_t *hour_cout = lv_obj_create(main_cout);
  lv_obj_set_size(hour_cout, 140, 140);
  lv_obj_align(hour_cout, LV_ALIGN_CENTER, -85, 0);
  lv_obj_set_style_bg_color(hour_cout, UI_FRAME_COLOR, 0);
  lv_obj_clear_flag(hour_cout, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *min_cout = lv_obj_create(main_cout);
  lv_obj_set_size(min_cout, 140, 140);
  lv_obj_align(min_cout, LV_ALIGN_CENTER, 85, 0);
  lv_obj_set_style_bg_color(min_cout, UI_FRAME_COLOR, 0);
  lv_obj_clear_flag(min_cout, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *seg_text = lv_label_create(main_cout);
  lv_obj_align(seg_text, LV_ALIGN_CENTER, 0, -10);
  lv_obj_set_style_text_font(seg_text, &alibaba_font_60, 0);
  lv_label_set_text(seg_text, ":");
  lv_obj_set_style_text_color(seg_text, UI_FONT_COLOR, 0);

  lv_obj_t *hour_text = lv_label_create(hour_cout);
  lv_obj_center(hour_text);
  lv_obj_set_style_text_font(hour_text, &alibaba_font_60, 0);
  lv_label_set_text(hour_text, "12");
  lv_obj_set_style_text_color(hour_text, UI_FONT_COLOR, 0);
  lv_obj_add_event_cb(hour_text, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_HOUR, hour_text, (void *)"%02d");

  lv_obj_t *min_text = lv_label_create(min_cout);
  lv_obj_center(min_text);
  lv_obj_set_style_text_font(min_text, &alibaba_font_60, 0);
  lv_label_set_text(min_text, "34");
  lv_obj_set_style_text_color(min_text, UI_FONT_COLOR, 0);
  lv_obj_add_event_cb(min_text, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_MIN, min_text, (void *)"%02d");


  static lv_style_t style_line;
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 4);
  lv_style_set_line_color(&style_line, UI_BG_COLOR);
  lv_style_set_line_rounded(&style_line, true);

  lv_obj_t *line;
  line = lv_line_create(main_cout);
  lv_line_set_points(line, line_points, 2);
  lv_obj_add_style(line, &style_line, 0);
  lv_obj_center(line);

  /* page 2 */
  lv_obj_t *debug_label = lv_label_create(tv2);
  String text;
  esp_chip_info_t t;
  esp_chip_info(&t);
  text = "chip : ";
  text += ESP.getChipModel();
  text += "\n";
  text += "psram size : ";
  text += ESP.getPsramSize() / 1024;
  text += " KB\n";
  text += "flash size : ";
  text += ESP.getFlashChipSize() / 1024;
  text += " KB\n";
  text += "SD type : ";
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_MMC) {
    text += "MMC\n";
  } else if (cardType == CARD_SD) {
    text += "SDSC\n";
  } else if (cardType == CARD_SDHC) {
    text += "SDHC\n";
  } else {
    text += "UNKNOWN\n";
  }
  text += "SD Card Size : ";
  uint32_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  text += cardSize;
  text += " MB";

  lv_label_set_text(debug_label, text.c_str());
  lv_obj_align(debug_label, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_t *bat_label = lv_label_create(tv2);
  lv_obj_align_to(bat_label, debug_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(bat_label, update_text_subscriber_cb, LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_NEW_VOLT, bat_label, (void *)"VOLT : %d mV");

  lv_timer_create(timer_task, 500, seg_text);
}

bool is_self_check_completed(void) { return _is_self_check_completed; }