#pragma once

#include "app_typedef.h"
#include "lvgl.h"

extern app_t app_radio;

void app_radio_load(lv_obj_t *cont);
void set_text_radio_ta(const char * txt, int cmd);
