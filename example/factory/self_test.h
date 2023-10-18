#pragma once

#define UI_BG_COLOR    lv_color_black()
#define UI_FRAME_COLOR lv_color_hex(0x282828)
#define UI_FONT_COLOR  lv_color_white()
#define UI_PAGE_COUNT  2

#define MSG_NEW_HOUR   1
#define MSG_NEW_MIN    2
#define MSG_NEW_VOLT   3




void self_test(void);
void ui_switch_page(void);
bool is_self_check_completed(void);