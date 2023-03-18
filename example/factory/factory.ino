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

/* external library */
#include "APA102.h"     // https://github.com/pololu/apa102-arduino
#include "Audio.h"      // https://github.com/schreibfaul1/ESP32-audioI2S
#include "TFT_eSPI.h"   // https://github.com/Bodmer/TFT_eSPI
#include "arduinoFFT.h" // https://github.com/kosme/arduinoFFT
// #include <NimBLEDevice.h>  // https://github.com/h2zero/NimBLE-Arduino
#include <OneButton.h>     // https://github.com/mathertel/OneButton
#include <RotaryEncoder.h> // https://github.com/mathertel/RotaryEncoder
#include "lv_conf.h"
#include <lvgl.h> // https://github.com/lvgl/lvgl

TFT_eSPI tft = TFT_eSPI(170, 320);
RotaryEncoder encoder(PIN_ENCODE_A, PIN_ENCODE_B, RotaryEncoder::LatchMode::TWO03);
OneButton button(PIN_ENCODE_BTN, true);
Audio *audio;
APA102<PIN_APA102_DI, PIN_APA102_CLK> ledStrip;
ES7210 mic(ES7210_AD1_AD0_00);
bool wifi_init = false; // When the WIFI initialization is completed, switch the mic spectrum mode.

EventGroupHandle_t global_event_group;
QueueHandle_t led_setting_queue;
/*
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
           |  mode |brightness
*/

QueueHandle_t led_flicker_queue;
QueueHandle_t play_music_queue;
QueueHandle_t play_time_queue;
static EventGroupHandle_t lv_input_event;


void ui_task(void *param);
void wav_task(void *param);
void led_task(void *param);
void mic_spk_task(void *param);
void mic_fft_task(void *param);
static lv_obj_t *create_btn(lv_obj_t *parent, char *text);
void timeavailable(struct timeval *t);
void printLocalTime();
void SD_init(void);

static void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h);
  lv_disp_flush_ready(disp);
}

static void lv_encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
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

void setup() {
  global_event_group = xEventGroupCreate();
  led_setting_queue = xQueueCreate(5, sizeof(uint16_t));
  led_flicker_queue = xQueueCreate(5, sizeof(uint16_t));
  play_music_queue = xQueueCreate(5, sizeof(String));
  play_time_queue = xQueueCreate(5, sizeof(uint32_t));
  lv_input_event = xEventGroupCreate();

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  Serial.begin(115200);
  Serial.printf("psram size : %d kb\r\n", ESP.getPsramSize() / 1024);
  Serial.printf("FLASH size : %d kb\r\n", ESP.getFlashChipSize() / 1024);

  SPIFFS.begin(true);
  Serial.printf("SPIFFS totalBytes : %d kb\r\n", SPIFFS.totalBytes() / 1024);
  Serial.printf("SPIFFS usedBytes : %d kb\r\n", SPIFFS.usedBytes() / 1024);
  SD_init();

  xTaskCreatePinnedToCore(led_task, "led_task", 1024 * 2, led_setting_queue, 0, NULL, 0);
  xTaskCreatePinnedToCore(ui_task, "ui_task", 1024 * 40, NULL, 3, NULL, 1);
}

void loop() { delay(1000); }

void wav_task(void *param) {
  String music_path;
  bool is_pause = false;
  uint32_t time_pos, Millis;
  static uint32_t music_time, end_time;
  audio = new Audio(0, 3, 1);
  audio->setPinout(PIN_IIS_BCLK, PIN_IIS_WCLK, PIN_IIS_DOUT);
  audio->setVolume(21); // 0...21
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
        // if (!audio->isRunning()) {
        audio->stopSong();
        audio->connecttoFS(SPIFFS, "/ring_1.mp3");
        // Serial.println("play \"/ring_1.mp3\"");
        is_pause = false;
        // }
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

void ui_task(void *param) {
  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t *buf1, *buf2;

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  button.attachClick(
      [](void *param) {
        EventGroupHandle_t *lv_input_event = (EventGroupHandle_t *)param;
        xEventGroupSetBits(lv_input_event, LV_BUTTON);
        xEventGroupSetBits(global_event_group, WAV_RING_1);
      },
      lv_input_event);

  lv_init();
  buf1 = (lv_color_t *)heap_caps_malloc(LV_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  assert(buf1);
  buf2 = (lv_color_t *)heap_caps_malloc(LV_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  assert(buf2);
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LV_BUF_SIZE);
  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
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
  // ui_boot_anim();
  lv_obj_t *self_test_btn = create_btn(lv_scr_act(), "self test");
  lv_obj_align(self_test_btn, LV_ALIGN_CENTER, -80, 0);
  lv_obj_add_event_cb(
      self_test_btn,
      [](lv_event_t *e) {
        lv_obj_clean(lv_scr_act());
        xTaskCreatePinnedToCore(mic_spk_task, "mic_spk_task", 1024 * 20, NULL, 3, NULL, 0);
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
      digitalPinToInterrupt(PIN_ENCODE_A), []() { encoder.tick(); }, CHANGE);
  attachInterrupt(
      digitalPinToInterrupt(PIN_ENCODE_B), []() { encoder.tick(); }, CHANGE);

  while (1) {
    delay(1);

    button.tick();
    lv_timer_handler();

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

static lv_obj_t *create_btn(lv_obj_t *parent, char *text) {

  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 120, 100);

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_center(label);
  lv_label_set_text(label, text);
  return btn;
}

// clang-format off
/* Converts a color from HSV to RGB.
 * h is hue, as a number between 0 and 360.
 * s is the saturation, as a number between 0 and 255.
 * v is the value, as a number between 0 and 255. */
rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return rgb_color(r, g, b);
}
// clang-format on

void led_task(void *param) {

  const uint8_t ledSort[7] = {2, 1, 0, 6, 5, 4, 3};
  // Set the number of LEDs to control.
  const uint16_t ledCount = 7;
  // Create a buffer for holding the colors (3 bytes per color).
  rgb_color colors[ledCount];
  // Set the brightness to use (the maximum is 31).
  uint8_t brightness = 1;

  uint16_t temp, mode = 0;
  int8_t last_led;
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
void spk_init(void) {
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
  i2s_pin_config_t pin_config = {0};
  pin_config.bck_io_num = PIN_IIS_BCLK;
  pin_config.ws_io_num = PIN_IIS_WCLK;
  pin_config.data_out_num = PIN_IIS_DOUT;
  i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_1);
  delay(100);
}

void mic_init(void) {
  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
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
  // i2s_start(I2S_NUM_1);
}

void SD_init(void) {
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

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
  WiFi.disconnect();
}

void mic_spk_task(void *param) {
  static uint16_t buffer[3200] = {0};
  uint32_t temp = 0;
  spk_init();
  // FFT_Install();
  mic_init();

  while (1) {
    delay(5);
    // if (!wifi_init) {
    /* Microphone loopback test */
    size_t bytes_read;
    i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, 15);
    i2s_write(I2S_NUM_1, &buffer, sizeof(buffer), &bytes_read, 15);
    // } else {
    //   delay(100);
    //   if (FFT_GetDataFlag()) {
    //     FFT_Calc();
    //     temp = 0;
    //     for (int i = 0; i < 8; i++) {
    //       buffer[i] = FFT_GetAmplitude(i);
    //       temp += FFT_GetAmplitude(i);
    //     }
    //     temp = temp / 8;
    //     temp = temp > 1024 ? 1024 : temp;
    //     temp = (temp / 100) - 3;
    //     temp = constrain(temp, 0, 7);
    //     xQueueSend(led_flicker_queue, &temp, portMAX_DELAY);
    //     FFT_ClrDataFlag();
    //   }
    // }
  }
  vTaskDelete(NULL);
}

void mic_fft_task(void *param) {

  size_t bytes_read;
  uint16_t buffer[SAMPLES] = {0};
  bool start_fft = false;
  FFT_Install();
  pinMode(PIN_ENCODE_BTN, INPUT);
  while (1) {
    delay(50);
    EventBits_t bit = xEventGroupGetBits(global_event_group);
    /* Microphone test */
    if (bit & FFT_READY) {
      xEventGroupClearBits(global_event_group, FFT_READY);
      mic_init();
      // Key point: IO0 will be output MCK if not reinitialized
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