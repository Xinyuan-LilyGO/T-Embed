#include "app_nfc.h"
#include "Arduino.h"
#include "pin_config.h"

LV_IMG_DECLARE(img_card);

int nfc_init_succeed = 0;

extern  void resume_nfcTaskHandler(void);

lv_obj_t * nfc_message_label = NULL;
static int new_data = 0;
char set_text_nfc_data[200] = {0};
void set_nfc_message_label(const char * txt, int cmd)
{
    if(cmd && new_data)
    {
        new_data = 0;
        if(nfc_message_label != NULL)
            lv_label_set_text(nfc_message_label, set_text_nfc_data);
    }
    else if(cmd == 0)
    {
        memset(set_text_nfc_data, 0, 200);
        if(txt != NULL)
        {
            memcpy(set_text_nfc_data, txt, 200);
            new_data = 1;
        }
    }
}

void app_nfc_load(lv_obj_t *cont) {
  /*lv_obj_t *img = lv_img_create(cont);
  lv_obj_set_pos(img, 33, 0);
  lv_obj_set_size(img, 254, 170);
  lv_img_set_src(img, &img_card);*/
    static lv_point_t line_points[] = {{30, 40}, {315, 40}};

    lv_obj_t * nfc_obj = lv_obj_create(cont);
    lv_obj_set_size(nfc_obj, 290, 170);
    lv_obj_set_pos(nfc_obj, 30, 0);
    lv_obj_set_style_border_color(nfc_obj, lv_color_hex(0xffffff), LV_PART_MAIN);
    //lv_obj_set_style_line_color(nfc_obj, lv_color_hex(0x6a6c62), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_obj, 5, 0);
    lv_obj_set_style_radius(nfc_obj, 10, 0); 

    lv_obj_t * line1;
    line1 = lv_line_create(cont);
    lv_line_set_points(line1, line_points, 2);     /*Set the points*/
    lv_obj_set_style_line_width(line1, 5, 0);
    lv_obj_set_style_line_color(line1, lv_color_hex(0xffffff), LV_PART_MAIN);//6a6c62
    //lv_obj_set_style_line_color(line1, lv_color_hex(0x6a6c62), LV_PART_MAIN);

    lv_obj_t * label1 = lv_label_create(cont);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);     
    lv_obj_set_width(label1, 287);
    if(nfc_init_succeed)
        lv_label_set_text(label1, "Please swipe your card");
    else 
        lv_label_set_text(label1, "nfc init fail!!!");
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label1, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_set_style_border_opa(label1, 0, 0);
    lv_obj_set_pos(label1, 30, 8);

    lv_obj_t * nfc_message_obj = lv_obj_create(cont);
    lv_obj_set_size(nfc_message_obj, 280, 120);
    lv_obj_set_pos(nfc_message_obj, 35, 45);
    lv_obj_set_style_border_opa(nfc_message_obj, 0, 0);
    lv_obj_clear_flag(nfc_message_obj, LV_OBJ_FLAG_SCROLLABLE);

    nfc_message_label = lv_label_create(nfc_message_obj);
    //lv_label_set_long_mode(nfc_message_label, LV_LABEL_LONG_WRAP);     
    lv_obj_set_width(nfc_message_label, 280);
    //lv_obj_set_style_size(nfc_message_label, 285, 120);
    if(nfc_init_succeed)
        lv_label_set_text(nfc_message_label, "Wait for the card to be swiped");
    else 
        lv_label_set_text(nfc_message_label, "nfc init fail!!!");
    lv_obj_set_style_text_align(nfc_message_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(nfc_message_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_align(nfc_message_label, LV_ALIGN_CENTER);
    lv_obj_set_style_border_opa(nfc_message_label, 0, 0);
    lv_obj_clear_flag(nfc_message_label, LV_OBJ_FLAG_SCROLLABLE);

    digitalWrite(NFC_CS, LOW);
    resume_nfcTaskHandler();
}

// static void sleep_event_cb(lv_event_t *e) {
//   digitalWrite(PIN_POWER_ON, 0);
//   esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_ENCODE_BTN, 0);
//   esp_deep_sleep_start();
// }

extern void suspend_nfcTaskHandler(void);
void app_nfc_exit(lv_obj_t *cont) 
{
    suspend_nfcTaskHandler();
    digitalWrite(NFC_CS, HIGH);
    nfc_message_label = NULL;
}

app_t app_nfc = {
    .setup_func_cb = app_nfc_load,
    .exit_func_cb = app_nfc_exit,
    .user_data = nullptr,
};