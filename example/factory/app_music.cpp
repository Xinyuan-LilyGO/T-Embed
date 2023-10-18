#include "app_music.h"
#include "SD_MMC.h"
#include "global_flags.h"

static struct {
  lv_obj_t *music_list;

  lv_obj_t *play;
  lv_obj_t *pause;
  lv_obj_t *stop;
  lv_obj_t *progress_bar;
  lv_obj_t *time_label;
} music_param;

extern QueueHandle_t play_music_queue;
extern QueueHandle_t play_time_queue;
extern EventGroupHandle_t global_event_group;

static void select_song_event_cb(lv_event_t *e);
static lv_obj_t *create_music_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,const char *symbol);
static void find_file(fs::FS &fs, const char *dirname, uint8_t levels, String &output,const char *file_type);

static void play_event_cb(lv_event_t *e);
static void drag_music_time_event_cb(lv_event_t *e);
static void pause_event_cb(lv_event_t *e);
static void stop_event_cb(lv_event_t *e);

void app_music_load(lv_obj_t *cont) {

  /*Create a list*/
  lv_obj_t *music_list = music_param.music_list = lv_roller_create(cont);
  lv_obj_set_size(music_list, LV_PCT(85), 120);
  lv_obj_align(music_list, LV_ALIGN_TOP_RIGHT, -5, 5);
  lv_obj_set_style_outline_color(music_list, lv_color_white(), LV_STATE_FOCUS_KEY);

  lv_list_add_text(music_list, "Music name");
  String name_list;
  find_file(SD_MMC, "/", 3, name_list, ".mp3");
  find_file(SD_MMC, "/", 3, name_list, ".wav");

  lv_roller_set_options(music_list, name_list.c_str(), LV_ROLLER_MODE_NORMAL);

  music_param.play = create_music_btn(cont, LV_ALIGN_LEFT_MID, 5, -40, LV_SYMBOL_PLAY);
  music_param.pause = create_music_btn(cont, LV_ALIGN_LEFT_MID, 5, -5, LV_SYMBOL_PAUSE);
  music_param.stop = create_music_btn(cont, LV_ALIGN_LEFT_MID, 5, 30, LV_SYMBOL_STOP);
  lv_obj_set_style_bg_color(music_param.stop, lv_palette_main(LV_PALETTE_PINK), 0);

  // Update music time progress bar
  music_param.progress_bar = lv_slider_create(cont);
  lv_obj_set_width(music_param.progress_bar, 250);
  lv_obj_align(music_param.progress_bar, LV_ALIGN_BOTTOM_LEFT, 20, -10);
  lv_obj_set_style_bg_color(music_param.progress_bar, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_pad_all(music_param.progress_bar, 1, LV_PART_KNOB);
  lv_obj_set_style_bg_color(music_param.progress_bar, lv_color_white(), LV_PART_INDICATOR);
  lv_obj_set_style_pad_all(music_param.progress_bar, 6, 0);
  lv_obj_add_event_cb( // update end time
      music_param.progress_bar,
      [](lv_event_t *e) {
        lv_obj_t *progress_bar = lv_event_get_target(e);
        lv_msg_t *m = lv_event_get_msg(e);
        const uint32_t *endtime = (const uint32_t *)lv_msg_get_user_data(m);
        lv_slider_set_range(progress_bar, 0, *endtime);
      },
      LV_EVENT_MSG_RECEIVED, NULL);

  lv_obj_add_event_cb(music_param.progress_bar, drag_music_time_event_cb, LV_EVENT_ALL, NULL);

  // Update song playback time
  music_param.time_label = lv_label_create(cont);
  lv_obj_align(music_param.time_label, LV_ALIGN_BOTTOM_RIGHT, -5, -8);
  lv_label_set_text(music_param.time_label, "00:00");
  lv_obj_add_event_cb(
      music_param.time_label,
      [](lv_event_t *e) {
        lv_obj_t *label = lv_event_get_target(e);
        lv_msg_t *m = lv_event_get_msg(e);
        const char *fmt = (const char *)lv_msg_get_user_data(m);
        const int32_t *v = (const int32_t *)lv_msg_get_payload(m);
        lv_label_set_text_fmt(label, fmt, (*v) / 60, (*v) % 60);
        if (!lv_obj_has_state(music_param.progress_bar, LV_STATE_EDITED))
          lv_slider_set_value(music_param.progress_bar, *v, LV_ANIM_ON);
      },
      LV_EVENT_MSG_RECEIVED, NULL);
  lv_msg_subsribe_obj(MSG_MUSIC_TIME_ID, music_param.time_label, (void *)"%02d:%02d");

  lv_obj_add_event_cb(music_param.play, play_event_cb, LV_EVENT_CLICKED, music_list);
  lv_obj_add_event_cb(music_param.pause, pause_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(music_param.stop, stop_event_cb, LV_EVENT_CLICKED, NULL);
}

static lv_obj_t *create_music_btn(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,const char *symbol) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 30, 30);
  lv_obj_set_style_radius(btn, 90, 0);
  lv_obj_align(btn, align, x_ofs, y_ofs);
  lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_FOCUS_KEY);

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_center(label);
  lv_label_set_text(label, symbol);

  return btn;
}

// Traverse SD card MP3 files
static void find_file(fs::FS &fs, const char *dirname, uint8_t levels, String &output,const char *file_type) {

  File root = fs.open(dirname);
  if (!root) {
    return;
  }
  if (!root.isDirectory()) {
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (levels) {
        find_file(fs, file.path(), levels - 1, output, file_type);
      }
    } else {
      if (strstr(file.name(), file_type) != nullptr) {
        output += file.path();
        output += "\n";
      }
    }
    file = root.openNextFile();
  }
}

static void play_event_cb(lv_event_t *e) {
  lv_obj_t *list = (lv_obj_t *)lv_event_get_user_data(e);
  char play_music_path[50];
  lv_roller_get_selected_str(list, play_music_path, 0);
  String path = play_music_path;
  Serial.println(path);
  xQueueSend(play_music_queue, &path, 0);
}

static void drag_music_time_event_cb(lv_event_t *e) {
  lv_obj_t *instance = lv_event_get_target(e);
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_RELEASED) {
    uint32_t pos = lv_slider_get_value(instance);
    xQueueSend(play_time_queue, &pos, 0);
  }
}

static void pause_event_cb(lv_event_t *e) { xEventGroupSetBits(global_event_group, RING_PAUSE); }

static void stop_event_cb(lv_event_t *e) { xEventGroupSetBits(global_event_group, RING_STOP); }


app_t app_music = {
    .setup_func_cb = app_music_load,
    .exit_func_cb = nullptr,
    .user_data= nullptr,
};