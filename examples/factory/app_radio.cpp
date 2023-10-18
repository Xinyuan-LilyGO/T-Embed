#include "app_radio.h"
#include "Arduino.h"
#include "pin_config.h"

int radio_init_succeed = 0;

void radioPingPong(lv_obj_t *parent);
extern void radio_power_cb(lv_event_t *e);
extern void radio_freq_cb(lv_event_t *e);
extern void radio_bandwidth_cb(lv_event_t *e);
extern void radio_rxtx_cb(lv_event_t *e);

#define DEFAULT_COLOR                           (lv_color_make(252, 218, 72))

// Save Radio Transmit Interval
static uint32_t configTransmitInterval = 0;
lv_timer_t *transmitTask;
static lv_obj_t *radio_ta = NULL;


char set_text_radio_data[250] = {0};
static int new_data = 0;


void set_text_radio_ta(const char * txt, int cmd)
{
    if(cmd && new_data)
    {
        new_data = 0;
        if(radio_ta != NULL)
            lv_label_set_text(radio_ta, (const char * )set_text_radio_data);
    }
    else if(cmd == 0)
    {
        memset(set_text_radio_data, 0, 250);
        if(txt != NULL)
        {
            memcpy(set_text_radio_data, txt, 250);
            new_data = 1;
        }
    }
}

extern u32_t radio_task_delay_ms;

static void radio_tx_interval_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    uint32_t id = lv_dropdown_get_selected(obj);
    Serial.printf("Option: %s id:%u\n", buf, id);

    // set carrier bandwidth
    uint16_t interval[] = {100, 200, 500, 1000, 2000, 3000};
    if (id > sizeof(interval) / sizeof(interval[0])) {
        Serial.println("invalid  tx interval params!");
        return;
    }
    // Save the configured transmission interval
    configTransmitInterval = interval[id];
    radio_task_delay_ms = interval[id];
    //lv_timer_set_period(transmitTask, interval[id]);
}

void app_radio_load(lv_obj_t *cont) {
    digitalWrite(RADIO_CS_PIN, LOW);
    radioPingPong(cont);
    //lv_timer_resume(transmitTask);
}

void radioPingPong(lv_obj_t *parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_border_width(&style, 5);
    lv_style_set_border_color(&style, DEFAULT_COLOR);
    lv_style_set_outline_color(&style, DEFAULT_COLOR);
    lv_style_set_bg_opa(&style, LV_OPA_50);

    static lv_style_t cont_style;
    lv_style_init(&cont_style);
    lv_style_set_bg_opa(&cont_style, LV_OPA_TRANSP);
    lv_style_set_bg_img_opa(&cont_style, LV_OPA_TRANSP);
    lv_style_set_line_opa(&cont_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&cont_style, 0);
    lv_style_set_text_color(&cont_style, DEFAULT_COLOR);

    lv_obj_t *cont = lv_obj_create(parent);
    //lv_obj_set_size(cont, lv_disp_get_hor_res(NULL), 400);
    lv_obj_set_size(cont, 240, 400);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_add_style(cont, &cont_style, LV_PART_MAIN);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);
//return ;
    radio_ta = lv_label_create(cont);
    lv_obj_set_size(radio_ta, 210, 80);
    lv_obj_align(radio_ta, LV_ALIGN_TOP_MID, 0, 20);
    if(radio_init_succeed)
        lv_label_set_text(radio_ta, "Radio Test");
    else 
        lv_label_set_text(radio_ta, "radio init fail!!!");
    //lv_textarea_set_text(radio_ta, "Radio Test");
    //lv_textarea_set_max_length(radio_ta, 256);
    //lv_textarea_set_cursor_click_pos(radio_ta, false);
    //lv_textarea_set_text_selection(radio_ta, false);
    lv_obj_add_style(radio_ta, &style, LV_PART_MAIN);
    lv_obj_set_style_border_color(radio_ta, lv_color_hex(0xffffff), LV_PART_MAIN);
    //lv_obj_set_style_line_color(radio_ta, lv_color_hex(0x6a6c62), LV_PART_MAIN);

    if(!radio_init_succeed)
        return;

    static lv_style_t cont1_style;
    lv_style_init(&cont1_style);
    lv_style_set_bg_opa(&cont1_style, LV_OPA_TRANSP);
    lv_style_set_bg_img_opa(&cont1_style, LV_OPA_TRANSP);
    lv_style_set_line_opa(&cont1_style, LV_OPA_TRANSP);
    lv_style_set_text_color(&cont1_style, DEFAULT_COLOR);
    lv_style_set_text_color(&cont1_style, lv_color_white());
    lv_style_set_border_width(&cont1_style, 5);
    lv_style_set_border_color(&cont1_style, DEFAULT_COLOR);
    lv_style_set_outline_color(&cont1_style, DEFAULT_COLOR);
    
    //! cont1
    lv_obj_t *cont1 = lv_obj_create(cont);
    lv_obj_set_scrollbar_mode(cont1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW_WRAP);
    // lv_obj_set_scroll_dir(cont1, LV_DIR_HOR);
    lv_obj_set_size(cont1, 210, 300);
    lv_obj_add_style(cont1, &cont1_style, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont1, lv_color_hex(0xffffff), LV_PART_MAIN);
    //lv_obj_set_style_line_color(cont1, lv_color_hex(0x6a6c62), LV_PART_MAIN);

    lv_obj_t *dd ;

    dd = lv_dropdown_create(cont1);
    lv_dropdown_set_options(dd, "TX\n"
                            "RX\n"
                            "Disable"
                           );
    lv_dropdown_set_selected(dd, 2);
    lv_obj_add_flag(dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(dd, 170, 50);
    lv_obj_add_event_cb(dd, radio_rxtx_cb,
                        LV_EVENT_VALUE_CHANGED
                        , NULL);

    dd = lv_dropdown_create(cont1);
    lv_dropdown_set_options(dd, "301M\n"
                            "315M\n"
                            "434M\n"
                            "868M\n"
                            "915M"
                           );
    lv_dropdown_set_selected(dd, 3);
    lv_obj_add_flag(dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(dd, 170, 50);
    lv_obj_add_event_cb(dd, radio_freq_cb,
                        LV_EVENT_VALUE_CHANGED
                        , NULL);


    dd = lv_dropdown_create(cont1);
    lv_dropdown_set_options(dd, "125KHz\n"
                            "250KHz\n"
                            "500KHz"
                           );
    lv_obj_add_flag(dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(dd, 170, 50);
    lv_dropdown_set_selected(dd, 1);
    lv_obj_add_event_cb(dd, radio_bandwidth_cb,
                        LV_EVENT_VALUE_CHANGED
                        , NULL);


    dd = lv_dropdown_create(cont1);
    lv_dropdown_set_options(dd, "2dBm\n"
                            "5dBm\n"
                            "10dBm\n"
                            "12dBm\n"
                            "17dBm\n"
                            "20dBm\n"
                            "22dBmn"
                           );
    lv_obj_add_flag(dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(dd, 170, 50);
    lv_dropdown_set_selected(dd, 6);
    lv_obj_add_event_cb(dd, radio_power_cb,
                        LV_EVENT_VALUE_CHANGED
                        , NULL);


    dd = lv_dropdown_create(cont1);
    lv_dropdown_set_options(dd, "100ms\n"
                            "200ms\n"
                            "500ms\n"
                            "1000ms\n"
                            "2000ms\n"
                            "3000ms"
                           );
    lv_dropdown_set_selected(dd, 1);
    lv_obj_add_flag(dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(dd, 170, 50);
    lv_obj_add_event_cb(dd, radio_tx_interval_cb,
                        LV_EVENT_VALUE_CHANGED
                        , NULL);

}

extern void suspend_radioTaskHandler(void);
void app_radio_exit(lv_obj_t *cont) 
{
    suspend_radioTaskHandler();
    digitalWrite(RADIO_CS_PIN, HIGH);
}

app_t app_radio = {
    .setup_func_cb = app_radio_load,
    .exit_func_cb = app_radio_exit,
    .user_data= nullptr,
};

