# 1 "C:\\Users\\helen\\AppData\\Local\\Temp\\tmp_a90dlva"
#include <Arduino.h>
# 1 "C:/Users/helen/Documents/Arduino/libraries/T-Embed/examples/factory/factory.ino"
#ifndef BOARD_HAS_PSRAM
#error "PSRAM is not ENABLE, Please set ArduinoIDE PSRAM option to OPI!"
#endif

#include "Arduino.h"
#include "FS.h"
#include "MyFFT.h"
#include "SD_MMC.h"
#include "SPIFFS.h"
#include "driver/i2s.h"
#include "es7210.h"
#include "global_flags.h"
#include "pin_config.h"
#include "self_test.h"
#include "ui.h"


#include "APA102.h"
#include "Audio.h"
#include "TFT_eSPI.h"
#include "arduinoFFT.h"

#include <OneButton.h>
#include <RotaryEncoder.h>
#include "lv_conf.h"
#include <lvgl.h>
#include <RadioLib.h>
#include <Adafruit_PN532.h>
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
};

TFT_eSPI tft = TFT_eSPI(170, 320);
RotaryEncoder encoder(PIN_ENCODE_A, PIN_ENCODE_B, RotaryEncoder::LatchMode::TWO03);
OneButton button(PIN_ENCODE_BTN, true);
Audio *audio;
APA102<PIN_APA102_DI, PIN_APA102_CLK> ledStrip;
ES7210 mic(ES7210_AD1_AD0_00);
bool wifi_init = false;
EventGroupHandle_t global_event_group;
QueueHandle_t led_setting_queue;





QueueHandle_t led_flicker_queue;
QueueHandle_t play_music_queue;
QueueHandle_t play_time_queue;
static EventGroupHandle_t lv_input_event;

static TaskHandle_t radioTaskHandler;
static TaskHandle_t nfcTaskHandler;
static bool isCoderOnline = false;

u32_t radio_task_delay_ms = 200;


static bool transmitFlag = false;

static int transmissionState = RADIOLIB_ERR_NONE;

static bool radioTransmitFlag = false;

SPIClass radioBus = SPIClass(HSPI);

CC1101 radio = new Module(RADIO_CS_PIN, PIN_IIC_SDA, RADIOLIB_NC, PIN_IIC_SCL, radioBus);
Adafruit_PN532 nfc(NFC_CS, &radioBus);

extern int nfc_init_succeed;
extern int radio_init_succeed;
extern lv_timer_t *transmitTask;
extern void lv_msg_send(uint32_t msg_id, const void * payload);


void ui_task(void *param);
void wav_task(void *param);
void led_task(void *param);
void radio_task(void *param);
void nfc_task(void *param);
void mic_spk_task(void *param);
void mic_fft_task(void *param);
static lv_obj_t *create_btn(lv_obj_t *parent,const char *text);
void timeavailable(struct timeval *t);
void printLocalTime();
void SD_init(void);
static void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void lv_encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void setup();
void loop();
rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v);
void suspend_nfcTaskHandler(void);
void resume_nfcTaskHandler(void);
void suspend_radioTaskHandler(void);
void resume_radioTaskHandler(void);
void radio_task(void * param);
void uint8_to_hexstr(uint8_t *in_data,int len,char *out_data);
void spk_init(void);
void mic_init(void);
void setFlag(void);
void radio_init(void);
void radio_power_cb(lv_event_t *e);
void float_to_str(float in_data, int decimal_place, char *out_data);
void radioTask(lv_timer_t *parent);
void radio_rxtx_cb(lv_event_t *e);
void radio_bandwidth_cb(lv_event_t *e);
void radio_freq_cb(lv_event_t *e);
#line 107 "C:/Users/helen/Documents/Arduino/libraries/T-Embed/examples/factory/factory.ino"
static void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h);
    lv_disp_flush_ready(disp);
}

static void lv_encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    EventGroupHandle_t *lv_input_event = (EventGroupHandle_t *)indev_drv->user_data;
    EventBits_t bit = xEventGroupGetBits(lv_input_event);
    data->state = LV_INDEV_STATE_RELEASED;
    if (bit & LV_BUTTON) {
        xEventGroupClearBits(lv_input_event, LV_BUTTON);
        data->state = LV_INDEV_STATE_PR;
    } else if (bit & LV_ENCODER_CW) {
        xEventGroupClearBits(lv_input_event, LV_ENCODER_CW);
        data->enc_diff = 1;
    } else if (bit & LV_ENCODER_CCW) {
        xEventGroupClearBits(lv_input_event, LV_ENCODER_CCW);
        data->enc_diff = -1;
    }
}

void setup()
{
    global_event_group = xEventGroupCreate();
    led_setting_queue = xQueueCreate(5, sizeof(uint16_t));
    led_flicker_queue = xQueueCreate(5, sizeof(uint16_t));
    play_music_queue = xQueueCreate(5, sizeof(String));
    play_time_queue = xQueueCreate(5, sizeof(uint32_t));
    lv_input_event = xEventGroupCreate();

    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    pinMode(RADIO_CS_PIN, OUTPUT);
    digitalWrite(RADIO_CS_PIN, HIGH);

    Serial.begin(115200);
    Serial.printf("psram size : %d kb\r\n", ESP.getPsramSize() / 1024);
    Serial.printf("FLASH size : %d kb\r\n", ESP.getFlashChipSize() / 1024);

    SPIFFS.begin(true);
    Serial.printf("SPIFFS totalBytes : %d kb\r\n", SPIFFS.totalBytes() / 1024);
    Serial.printf("SPIFFS usedBytes : %d kb\r\n", SPIFFS.usedBytes() / 1024);






    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.beginTransmission(0x40);
    if (Wire.endTransmission() == 0) {
        Serial.println("ES7210 Online.");
        isCoderOnline = true;
    } else {
        Serial.println("ES7210 not online.");

        Wire.endTransmission();
    }

    SD_init();

    xTaskCreatePinnedToCore(led_task, "led_task", 1024 * 2, led_setting_queue, 0, NULL, 0);
    xTaskCreatePinnedToCore(ui_task, "ui_task", 1024 * 40, NULL, 3, NULL, 1);

    if (!isCoderOnline) {
        radio_init();
        xTaskCreate(radio_task, "radio_task", 2048, NULL, 10, &radioTaskHandler);
        xTaskCreate(nfc_task, "nfc_task", 2048, NULL, 10, &nfcTaskHandler);
    }
}

void loop()
{
    delay(1000);
}

void wav_task(void *param)
{
    String music_path;
    bool is_pause = false;
    uint32_t time_pos = 0, Millis = 0;
    static uint32_t music_time = 0, end_time = 0;
    audio = new Audio(0, 3, 1);
    audio->setPinout(PIN_IIS_BCLK, PIN_IIS_WCLK, PIN_IIS_DOUT);
    audio->setVolume(21);
    audio->connecttoFS(SPIFFS, "/ring_setup.mp3");
    Serial.println("play \"/ring_setup.mp3\"");
    while (1) {

        EventBits_t bit = xEventGroupGetBits(global_event_group);
        if (bit) {
            if (bit & RING_PAUSE) {
                xEventGroupClearBits(global_event_group, RING_PAUSE);
                is_pause = !is_pause;
            }
            if (bit & RING_STOP) {
                xEventGroupClearBits(global_event_group, RING_STOP);
                audio->stopSong();
                is_pause = false;
            }
            if (bit & WAV_RING_1) {
                xEventGroupClearBits(global_event_group, WAV_RING_1);

                audio->stopSong();
                audio->connecttoFS(SPIFFS, "/ring_1.mp3");

                is_pause = false;

            }
        }

        if (xQueueReceive(play_music_queue, &music_path, 0)) {
            Serial.print("play ");
            Serial.println(music_path.c_str());
            audio->stopSong();
            audio->connecttoFS(SD_MMC, music_path.c_str());
            is_pause = false;
        }

        if (xQueueReceive(play_time_queue, &time_pos, 0)) {
            audio->setAudioPlayPosition(time_pos);
        }

        if (audio->isRunning() && Millis - millis() > 100) {
            music_time = audio->getAudioCurrentTime();
            lv_msg_send(MSG_MUSIC_TIME_ID, &music_time);

            end_time = audio->getTotalPlayingTime();
            lv_msg_send(MSG_MUSIC_TIME_END_ID, &end_time);
            Millis = millis();
        }
        if (!is_pause)
            audio->loop();
        delay(1);
    }
}

void ui_task(void *param)
{
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t *buf1, *buf2;

    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);


    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7f); j++) {
            tft.writedata(lcd_st7789v[i].data[j]);
        }

        if (lcd_st7789v[i].len & 0x80) {
            delay(120);
        }
    }

    button.attachClick(
        [](void *param) {
        EventGroupHandle_t *lv_input_event = (EventGroupHandle_t *)param;
        xEventGroupSetBits(lv_input_event, LV_BUTTON);
        xEventGroupSetBits(global_event_group, WAV_RING_1);
    },
    lv_input_event);

    lv_init();
    buf1 = (lv_color_t *)ps_malloc(LV_BUF_SIZE * sizeof(lv_color_t));
    assert(buf1);
    buf2 = (lv_color_t *)ps_malloc(LV_BUF_SIZE * sizeof(lv_color_t));
    assert(buf2);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LV_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = LV_SCREEN_WIDTH;
    disp_drv.ver_res = LV_SCREEN_HEIGHT;
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = lv_encoder_read;
    indev_drv.user_data = lv_input_event;
    static lv_indev_t *lv_encoder_indev = lv_indev_drv_register(&indev_drv);
    lv_group_t *g = lv_group_create();
    lv_indev_set_group(lv_encoder_indev, g);
    lv_group_set_default(g);


    LV_IMG_DECLARE(image_lilygo_fcc);
    lv_obj_t *fcc = lv_img_create(lv_scr_act());
    lv_img_set_src(fcc, &image_lilygo_fcc);
    lv_task_handler();
    delay(3000);
    lv_obj_del(fcc);



    lv_obj_t *self_test_btn = create_btn(lv_scr_act(), "self test");
    lv_obj_align(self_test_btn, LV_ALIGN_CENTER, -80, 0);
    lv_obj_add_event_cb(
        self_test_btn,
        [](lv_event_t *e) {
        lv_obj_clean(lv_scr_act());
        if (!isCoderOnline) {
            xTaskCreatePinnedToCore(mic_spk_task, "mic_spk_task", 1024 * 20, NULL, 3, NULL, 0);
        }
        xEventGroupSetBits(lv_input_event, LV_SELF_TEST_START);
    },
    LV_EVENT_CLICKED, NULL);

    lv_obj_t *into_ui_btn = create_btn(lv_scr_act(), "into ui");
    lv_obj_align(into_ui_btn, LV_ALIGN_CENTER, 80, 0);
    lv_obj_add_event_cb(
        into_ui_btn,
        [](lv_event_t *e) {
        lv_obj_clean(lv_scr_act());
        xTaskCreatePinnedToCore(wav_task, "wav_task", 1024 * 4, NULL, 2, NULL, 0);
        xTaskCreatePinnedToCore(mic_fft_task, "fft_task", 1024 * 20, NULL, 1, NULL, 0);
        xEventGroupSetBits(lv_input_event, LV_UI_DEMO_START);
    },
    LV_EVENT_CLICKED, NULL);

    attachInterrupt(
        digitalPinToInterrupt(PIN_ENCODE_A), []() {
        encoder.tick();
    }, CHANGE);
    attachInterrupt(
        digitalPinToInterrupt(PIN_ENCODE_B), []() {
        encoder.tick();
    }, CHANGE);

    while (1) {
        delay(1);
        button.tick();
        lv_timer_handler();
        if (isCoderOnline) {
            set_text_radio_ta(NULL, 1);
            set_nfc_message_label(NULL, 1);
        }
        RotaryEncoder::Direction dir = encoder.getDirection();
        if (dir != RotaryEncoder::Direction::NOROTATION) {
            if (dir != RotaryEncoder::Direction::CLOCKWISE) {
                xEventGroupSetBits(lv_input_event, LV_ENCODER_CW);
                xEventGroupSetBits(lv_input_event, LV_ENCODER_LED_CW);
            } else {
                xEventGroupSetBits(lv_input_event, LV_ENCODER_CCW);
                xEventGroupSetBits(lv_input_event, LV_ENCODER_LED_CCW);
            }
            if (is_self_check_completed())
                ui_switch_page();
        }

        EventBits_t bit = xEventGroupGetBits(lv_input_event);
        if (bit & LV_SELF_TEST_START) {
            xEventGroupClearBits(lv_input_event, LV_SELF_TEST_START);
            uint16_t temp = 0x41;
            xQueueSend(led_setting_queue, &temp, 0);
            self_test();
            wifi_init = true;
            temp = (0xF << 6) | 0x02;
            xQueueSend(led_setting_queue, &temp, 0);
        } else if (bit & LV_UI_DEMO_START) {
            xEventGroupClearBits(lv_input_event, LV_UI_DEMO_START);
            ui_init();
        }
    }

    vTaskDelete(NULL);
}

static lv_obj_t *create_btn(lv_obj_t *parent,const char *text)
{

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 100);

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_center(label);
    lv_label_set_text(label, text);
    return btn;
}






rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch ((h / 60) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    }
    return rgb_color(r, g, b);
}



void suspend_nfcTaskHandler(void)
{
    vTaskSuspend(nfcTaskHandler);
}

void resume_nfcTaskHandler(void)
{
    vTaskResume(nfcTaskHandler);
}

void suspend_radioTaskHandler(void)
{
    vTaskSuspend(radioTaskHandler);
}

void resume_radioTaskHandler(void)
{
    vTaskResume(radioTaskHandler);
}


void radio_task(void * param)
{
    digitalWrite(RADIO_CS_PIN, HIGH);
    Serial.print("==========radio_init sta===========\r\n");

    Serial.print("radio_init end\r\n");

    suspend_radioTaskHandler();

    while(1)
    {
        if(radio_init_succeed)
            radioTask(NULL);

        delay(radio_task_delay_ms);
    }
}



void uint8_to_hexstr(uint8_t *in_data,int len,char *out_data)
{
    int i = 0, out_cont = 0;
    for(i = 0; i < len; i++)
    {

        if(in_data[i] > 15)
        {
            out_data[out_cont++] = (in_data[i]/16) >= 10?('A'+(in_data[i]/16)-10):('0'+in_data[i]/16);
        }
        else
            out_data[out_cont++] = '0';
        out_data[out_cont++] = (in_data[i]%16) >= 10?('A'+(in_data[i]%16)-10):('0'+in_data[i]%16);
        out_data[out_cont++] = ' ';
    }
    out_data[out_cont] = '\0';
}

void nfc_task(void *param)
{
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);


    digitalWrite(NFC_CS, LOW);
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("Didn't find PN53x board");
        nfc_init_succeed = 0;


    }
    else
    {
        nfc_init_succeed = 1;

        Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
        Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
        Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
        Serial.println("Waiting for an ISO14443A Card ...");
    }


    digitalWrite(NFC_CS, HIGH);
    suspend_nfcTaskHandler();

    uint32_t nfc_Success_count = 0;

    if(!nfc_init_succeed)
    {
        while(1)
        {
            delay(100);
        }
    }

    while(1)
    {
        delay(100);

        uint8_t success;
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
        uint8_t uidLength;




        Serial.println("readPassiveTargetID sta\n");
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        Serial.printf("readPassiveTargetID end =%d=\n", success);
        if (success) {
            char text_nfc_data[200] = {0};
            nfc_Success_count++;

            Serial.println("Found an ISO14443A card");
            Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
            Serial.print("  UID Value: ");
            nfc.PrintHex(uid, uidLength);
            Serial.println("");

            if (uidLength == 4)
            {

                Serial.println("Seems to be a Mifare Classic card (4 byte UID)");



                Serial.println("Trying to authenticate block 4 with default KEYA value");
                uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };




                success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

            if (success)
            {
                    Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
                    uint8_t data[16];







                    success = nfc.mifareclassic_ReadDataBlock(4, data);

                if (success)
                {

                        Serial.println("Reading Block 4:");
                        nfc.PrintHexChar(data, 16);
                        Serial.println("");


                        sprintf(text_nfc_data, "Success recognition count: %d\n", nfc_Success_count);
                        sprintf(&text_nfc_data[strlen(text_nfc_data)], "Seems to be a Mifare Classic card (4 byte UID)\nUID Value: " );
                        uint8_to_hexstr(uid, 4, &text_nfc_data[strlen(text_nfc_data)]);
                        sprintf(&text_nfc_data[strlen(text_nfc_data)], "\nSector 1 (Blocks 4..7) has been authenticated" );

                        set_nfc_message_label(text_nfc_data, 0);

                delay(1000);
                }
                else
                {
                Serial.println("Ooops ... unable to read the requested block.  Try another key?");
                }
            }
            else
            {
                sprintf(text_nfc_data, "Success recognition count: %d\n", nfc_Success_count);
                sprintf(&text_nfc_data[strlen(text_nfc_data)], "Seems to be a Mifare Ultralight tag (7 byte UID)\nUID Value: " );
                uint8_to_hexstr(uid, 4, &text_nfc_data[strlen(text_nfc_data)]);
                sprintf(&text_nfc_data[strlen(text_nfc_data)], "\nOoops ... authentication failed: Try another key?" );
                set_nfc_message_label(text_nfc_data, 0);

                Serial.println("Ooops ... authentication failed: Try another key?");
            }
            }

            if (uidLength == 7)
            {

                Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");


                Serial.println("Reading page 4");
                uint8_t data[32];
                success = nfc.mifareultralight_ReadPage (4, data);
            if (success)
            {

                nfc.PrintHexChar(data, 4);
                Serial.println("");
                sprintf(text_nfc_data, "Success recognition count: %d\n", nfc_Success_count);
                sprintf(&text_nfc_data[strlen(text_nfc_data)], "Seems to be a Mifare Ultralight tag (7 byte UID)\nUID Value: " );
                uint8_to_hexstr(uid, 4, &text_nfc_data[strlen(text_nfc_data)]);
                sprintf(&text_nfc_data[strlen(text_nfc_data)], "\nSector 1 (Blocks 4..7) has been authenticated" );
                set_nfc_message_label(text_nfc_data, 0);

                delay(1000);
            }
            else
            {
                sprintf(text_nfc_data, "Success recognition count: %d\n", nfc_Success_count);
                sprintf(&text_nfc_data[strlen(text_nfc_data)], "Seems to be a Mifare Ultralight tag (7 byte UID)\nUID Value: " );
                uint8_to_hexstr(uid, 4, &text_nfc_data[strlen(text_nfc_data)]);
                sprintf(&text_nfc_data[strlen(text_nfc_data)], "\nOoops ... unable to read the requested page!?" );
                set_nfc_message_label(text_nfc_data, 0);

                Serial.println("Ooops ... unable to read the requested page!?");
            }
            }
        }
    }
}

void led_task(void *param)
{

    const uint8_t ledSort[7] = {2, 1, 0, 6, 5, 4, 3};

    const uint16_t ledCount = 7;

    rgb_color colors[ledCount];

    uint8_t brightness = 1;

    uint16_t temp, mode = 0;
    int8_t last_led = 0;
    EventBits_t bit;

    while (1) {

        if (xQueueReceive(led_setting_queue, &temp, 0)) {
            mode = (temp >> 6) & 0xF;
            brightness = temp & 0x3F;

            Serial.printf("temp : 0x%X\r\n", temp);
            Serial.printf("mode : 0x%X\r\n", mode);
            Serial.printf("brightness : 0x%X\r\n", brightness);
        }

        uint8_t time = millis() >> 4;
        switch (mode) {
        case 0:
            for (uint16_t i = 0; i < ledCount; i++) {
                colors[i] = rgb_color(0, 0, 0);
            }
            ledStrip.write(colors, ledCount, brightness);
            break;
        case 1:
            for (uint16_t i = 0; i < ledCount; i++) {
                colors[i] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
            }
            ledStrip.write(colors, ledCount, brightness);
            break;
        case 2:
            for (uint16_t i = 0; i < ledCount; i++) {
                uint8_t p = time - i * 8;
                colors[i] = hsvToRgb((uint32_t)p * 359 / 256, 255, 255);
            }
            ledStrip.write(colors, ledCount, brightness);
            break;
        case 3:
            memset(colors, 0, sizeof(colors));
            bit = xEventGroupGetBits(lv_input_event);
            if (bit & LV_ENCODER_LED_CW) {
                xEventGroupClearBits(lv_input_event, LV_ENCODER_LED_CW);
                if (--last_led < 0) {
                    last_led = ledCount - 1;
                }
            } else if (bit & LV_ENCODER_LED_CCW) {
                xEventGroupClearBits(lv_input_event, LV_ENCODER_LED_CCW);
                if (++last_led > ledCount - 1) {
                    last_led = 0;
                }
            }
            colors[last_led] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
            ledStrip.write(colors, ledCount, brightness);
            break;
        case 4:
            for (uint16_t i = 0; i < ledCount; i++) {
                colors[i] = rgb_color(0xff, 0xff, 0xff);
            }
            ledStrip.write(colors, ledCount, brightness);
            break;
        case 5:
            for (uint16_t i = 0; i < ledCount; i++) {
                colors[i] = rgb_color(255, 128, 0);
            }
            ledStrip.write(colors, ledCount, brightness);
            break;
        case 0xF:
            if (xQueueReceive(led_flicker_queue, &temp, 0)) {
                for (uint16_t i = 0; i < ledCount; i++) {
                    colors[i] = rgb_color(0, 0, 0);
                }
                for (uint8_t i = 1; i < temp; i++) {
                    colors[ledSort[i]] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
                }
                colors[ledSort[0]] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
                ledStrip.write(colors, ledCount, brightness);
            }
            break;
        default:
            break;
        }
        delay(10);
    }
}
void spk_init(void)
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_FREQ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 160,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
    };
    i2s_pin_config_t pin_config;
    pin_config.data_in_num = -1;
    pin_config.mck_io_num = -1;
    pin_config.bck_io_num = PIN_IIS_BCLK;
    pin_config.ws_io_num = PIN_IIS_WCLK;
    pin_config.data_out_num = PIN_IIS_DOUT;
    i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_1);
    delay(100);
}

void mic_init(void)
{
    mic.begin(&Wire);
    audio_hal_codec_i2s_iface_t i2s_cfg;
    i2s_cfg.bits = AUDIO_HAL_BIT_LENGTH_16BITS;
    i2s_cfg.fmt = AUDIO_HAL_I2S_NORMAL;
    i2s_cfg.mode = AUDIO_HAL_MODE_SLAVE;
    i2s_cfg.samples = AUDIO_HAL_16K_SAMPLES;
    mic.adc_config_i2s(AUDIO_HAL_CODEC_MODE_ENCODE, &i2s_cfg);
    mic.mic_select((es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2));
    mic.adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2), GAIN_9DB);
    mic.adc_ctrl_state(AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_FREQ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = SAMPLE_BLOCK,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
    };
    i2s_pin_config_t pin_config = {0};
    pin_config.bck_io_num = PIN_ES7210_BCLK;
    pin_config.ws_io_num = PIN_ES7210_LRCK;
    pin_config.data_in_num = PIN_ES7210_DIN;
    pin_config.mck_io_num = PIN_ES7210_MCLK;
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);

}

void SD_init(void)
{
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, 1);
    SD_MMC.setPins(PIN_SD_SCK, PIN_SD_MOSI, PIN_SD_MISO);
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
    }
    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    delay(500);
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("No time available (yet)");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    printLocalTime();
    WiFi.disconnect();
}

void mic_spk_task(void *param)
{
    static uint16_t buffer[3200] = {0};

    spk_init();

    mic_init();

    while (1) {
        delay(5);


        size_t bytes_read;
        i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, 15);
        i2s_write(I2S_NUM_1, &buffer, sizeof(buffer), &bytes_read, 15);
# 887 "C:/Users/helen/Documents/Arduino/libraries/T-Embed/examples/factory/factory.ino"
    }
    vTaskDelete(NULL);
}

void mic_fft_task(void *param)
{

    uint16_t buffer[SAMPLES] = {0};
    bool start_fft = false;
    FFT_Install();
    pinMode(PIN_ENCODE_BTN, INPUT);
    while (1) {
        delay(50);
        EventBits_t bit = xEventGroupGetBits(global_event_group);

        if (bit & FFT_READY) {
            xEventGroupClearBits(global_event_group, FFT_READY);
            mic_init();

            pinMode(PIN_ENCODE_BTN, INPUT);
            start_fft = true;
        }
        if (bit & FFT_STOP) {
            xEventGroupClearBits(global_event_group, FFT_STOP);
            start_fft = false;
        }

        if (start_fft) {

            if (FFT_GetDataFlag()) {
                FFT_Calc();
                for (int i = 0; i < SAMPLES; i++) {
                    buffer[i] = FFT_GetAmplitude(i);
                }

                lv_msg_send(MSG_FFT_ID, buffer);
                FFT_ClrDataFlag();
            }
        }
    }
    vTaskDelete(NULL);
}





volatile bool enableInterrupt = true;




void setFlag(void) {

  if(!enableInterrupt) {
    return;
  }



    radioTransmitFlag = true;
}






void radio_init(void)
{
    digitalWrite(PIN_SD_CS, HIGH);

    pinMode(NFC_CS, OUTPUT);
    digitalWrite(NFC_CS, HIGH);

    pinMode(RADIO_SW1_PIN, OUTPUT);
    pinMode(RADIO_SW0_PIN, OUTPUT);
    digitalWrite(RADIO_SW0_PIN, HIGH);
    digitalWrite(RADIO_SW1_PIN, LOW);

    radioBus.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI);

    Serial.print(F("[CC1101] Initializing ... "));
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
        radio_init_succeed = 1;
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        radio_init_succeed = 0;
        return;
        while (true);
    }

    if (radio.setFrequency(868.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("[CC1101] Selected frequency is invalid for this module!"));
        while (true);
    }


    state = radio.setBitRate(5.0);
    if (state == RADIOLIB_ERR_INVALID_BIT_RATE) {
        Serial.println(F("[CC1101] Selected bit rate is invalid for this module!"));
        while (true);
    } else if (state == RADIOLIB_ERR_INVALID_BIT_RATE_BW_RATIO) {
        Serial.println(F("[CC1101] Selected bit rate to bandwidth ratio is invalid!"));
        Serial.println(F("[CC1101] Increase receiver bandwidth to set this bit rate."));
        while (true);
    }


    if (radio.setRxBandwidth(135.0) == RADIOLIB_ERR_INVALID_RX_BANDWIDTH) {
        Serial.println(F("[CC1101] Selected receiver bandwidth is invalid for this module!"));
        while (true);
    }


    if (radio.setFrequencyDeviation(30.0) == RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION) {
        Serial.println(F("[CC1101] Selected frequency deviation is invalid for this module!"));
        while (true);
    }


    if (radio.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("[CC1101] Selected output power is invalid for this module!"));
        while (true);
    }


    if (radio.setSyncWord(0x01, 0x23) == RADIOLIB_ERR_INVALID_SYNC_WORD) {
        Serial.println(F("[CC1101] Selected sync word is invalid for this module!"));
        while (true);
    }


    radio.setGdo0Action(setFlag);


    Serial.print(F("[CC1101] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

}

void radio_power_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    uint32_t id = lv_dropdown_get_selected(obj);
    Serial.printf("Option: %s id:%u\n", buf, id);
# 1053 "C:/Users/helen/Documents/Arduino/libraries/T-Embed/examples/factory/factory.ino"
    uint8_t dBm[] = {
        2, 5, 10, 12, 17, 20, 22
    };
    if (id > sizeof(dBm) / sizeof(dBm[0])) {
        Serial.println("invalid dBm params!");
        return;
    }

    if (radio.setOutputPower(dBm[id]) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
    }

    if (transmitFlag) {
        radio.startTransmit("");
    } else {
        radio.startReceive();
    }





}


void float_to_str(float in_data, int decimal_place, char *out_data)
{
    int out_len_cont = 0;
    u_int32_t divisor = 1, decimal_place_divisor = 1;
    if(in_data < 0)
    {
        out_data[out_len_cont++] = '-';
        in_data *= -1;
    }

    while((int)in_data / divisor > 0)
    {
        divisor*=10;
    }

    divisor/=10;

    int i = 0;
    while(i++ < decimal_place)
    {
        in_data*=10;
        divisor*=10;
        decimal_place_divisor *= 10;
    }

    while(divisor >= decimal_place_divisor)
    {
        out_data[out_len_cont++] = (int)in_data/divisor+'0';
        in_data = (int)in_data % divisor;
        divisor/=10;
    }
    out_data[out_len_cont++] = '.';
    while(decimal_place-- > 0)
    {
        out_data[out_len_cont++] = ((int)in_data)/divisor+'0';
        in_data = (int)in_data % divisor;
        divisor/=10;
    }

}

u_int32_t radio_tx_count = 0;
u_int32_t radio_rx_count = 0;
void radioTask(lv_timer_t *parent)
{
    char buf[256] = {0};


    if (radioTransmitFlag) {

        radioTransmitFlag = false;
        enableInterrupt = false;

        if (transmitFlag) {



            if (transmissionState == RADIOLIB_ERR_NONE) {

                Serial.println(F("transmission finished!"));
                radio_tx_count++;
            } else {
                Serial.print(F("failed, code "));
                Serial.println(transmissionState);
            }

            lv_snprintf(buf, 256, "[%u]:Tx %s", radio_tx_count, transmissionState == RADIOLIB_ERR_NONE ? "Successed" : "Failed");
            set_text_radio_ta(buf, 0);
            transmissionState = radio.startTransmit("Hello World!");
        } else {



            String str;
            int state = radio.readData(str);

            if (state == RADIOLIB_ERR_NONE) {
                radio_rx_count++;

                Serial.println(F("[SX1262] Received packet!"));


                Serial.print(F("[SX1262] Data:\t\t"));
                Serial.println(str);


                Serial.print(F("[SX1262] RSSI:\t\t"));
                float rssi_t = radio.getRSSI();
                Serial.print(rssi_t);
                Serial.println(F(" dBm"));
# 1177 "C:/Users/helen/Documents/Arduino/libraries/T-Embed/examples/factory/factory.ino"
                lv_snprintf(buf, 256, "[%u]:Rx %s \n", radio_rx_count, str.c_str());
                lv_snprintf(&buf[strlen(buf)], 256, "RSSI:%d.%d", (int)rssi_t, ((int)(rssi_t*100)%100)>0?((int)(rssi_t*100)%100):((int)(rssi_t*100)%100)*-1);





                set_text_radio_ta(buf, 0);
            }

            radio.startReceive();
        }
        enableInterrupt = true;
    }
}

void radio_rxtx_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    uint32_t id = lv_dropdown_get_selected(obj);
    Serial.printf("Option: %s id:%u\n", buf, id);
    switch (id) {
    case 0:
        digitalWrite(RADIO_CS_PIN, LOW);



        Serial.print(F("[Radio] Sending first packet ... "));
        transmissionState = radio.startTransmit("Hello World!");
        transmitFlag = true;
        resume_radioTaskHandler();
        break;
    case 1:
        digitalWrite(RADIO_CS_PIN, LOW);


        Serial.print(F("[Radio] Starting to listen ... "));
        if (radio.startReceive() == RADIOLIB_ERR_NONE) {
            Serial.println(F("success!"));
        } else {
            Serial.println(F("failed "));
        }
        transmitFlag = false;
        set_text_radio_ta("[RX]:Listening.", 0);
        resume_radioTaskHandler();
        break;
    case 2:






        suspend_radioTaskHandler();
        break;
    default:
        break;
    }
}

void radio_bandwidth_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    uint32_t id = lv_dropdown_get_selected(obj);
    Serial.printf("Option: %s id:%u\n", buf, id);


    const float bw[] = {125.0, 250.0, 500.0};
    if (id > sizeof(bw) / sizeof(bw[0])) {
        Serial.println("invalid bandwidth params!");
        return;
    }
# 1261 "C:/Users/helen/Documents/Arduino/libraries/T-Embed/examples/factory/factory.ino"
    radio.standby();

    if (radio.setRxBandwidth(bw[id]) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
    }

    if (transmitFlag) {
        radio.startTransmit("");
    } else {
        radio.startReceive();
    }





}

void radio_freq_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    uint32_t id = lv_dropdown_get_selected(obj);
    Serial.printf("Option: %s id:%u\n", buf, id);


    const float freq[] = {301.0, 315.0, 434.0, 868.0, 915.0};
    if (id > sizeof(freq) / sizeof(freq[0])) {
        Serial.println("invalid params!");
        return;
    }

    if(id <= 1)
    {
        digitalWrite(RADIO_SW0_PIN, LOW);
        digitalWrite(RADIO_SW1_PIN, HIGH);
    }
    if(id >= 3)
    {
        digitalWrite(RADIO_SW0_PIN, HIGH);
        digitalWrite(RADIO_SW1_PIN, LOW);
    }
    else
    {
        digitalWrite(RADIO_SW0_PIN, HIGH);
        digitalWrite(RADIO_SW1_PIN, HIGH);
    }





    digitalWrite(RADIO_CS_PIN, HIGH);

    if (radio.setFrequency(freq[id]) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
    }

    if (transmitFlag) {
        radio.startTransmit("");
    } else {
        radio.startReceive();
    }





}