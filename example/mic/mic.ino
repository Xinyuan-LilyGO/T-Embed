#include "Arduino.h"
#include "driver/i2s.h"
#include "es7210.h"
#include "pin_config.h"

ES7210 mic(ES7210_AD1_AD0_00);
#define SAMPLES      512
#define SAMPLE_BLOCK 64
#define SAMPLE_FREQ  16000

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  Serial.begin(115200);
  Serial.printf("psram size : %d kb\r\n", ESP.getPsramSize() / 1024);
  Serial.printf("FLASH size : %d kb\r\n", ESP.getFlashChipSize() / 1024);

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
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
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

void loop() {
  size_t bytes_read;
  uint16_t buffer[3200] = {0};
  delay(10);
  i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, 15);
}