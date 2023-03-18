#pragma once

#include "Wire.h"
#include "audio_hal.h"
#include "driver/i2s.h"

#define ES7210_RESET_REG00           0x00 /* Reset control */
#define ES7210_CLOCK_OFF_REG01       0x01 /* Used to turn off the ADC clock */
#define ES7210_MAINCLK_REG02         0x02 /* Set ADC clock frequency division */
#define ES7210_MASTER_CLK_REG03      0x03 /* MCLK source $ SCLK division */
#define ES7210_LRCK_DIVH_REG04       0x04 /* lrck_divh */
#define ES7210_LRCK_DIVL_REG05       0x05 /* lrck_divl */
#define ES7210_POWER_DOWN_REG06      0x06 /* power down */
#define ES7210_OSR_REG07             0x07
#define ES7210_MODE_CONFIG_REG08     0x08 /* Set master/slave & channels */
#define ES7210_TIME_CONTROL0_REG09   0x09 /* Set Chip intial state period*/
#define ES7210_TIME_CONTROL1_REG0A   0x0A /* Set Power up state period */
#define ES7210_SDP_INTERFACE1_REG11  0x11 /* Set sample & fmt */
#define ES7210_SDP_INTERFACE2_REG12  0x12 /* Pins state */
#define ES7210_ADC_AUTOMUTE_REG13    0x13 /* Set mute */
#define ES7210_ADC34_MUTERANGE_REG14 0x14 /* Set mute range */
#define ES7210_ADC34_HPF2_REG20      0x20 /* HPF */
#define ES7210_ADC34_HPF1_REG21      0x21
#define ES7210_ADC12_HPF1_REG22      0x22
#define ES7210_ADC12_HPF2_REG23      0x23
#define ES7210_ANALOG_REG40          0x40 /* ANALOG Power */
#define ES7210_MIC12_BIAS_REG41      0x41
#define ES7210_MIC34_BIAS_REG42      0x42
#define ES7210_MIC1_GAIN_REG43       0x43
#define ES7210_MIC2_GAIN_REG44       0x44
#define ES7210_MIC3_GAIN_REG45       0x45
#define ES7210_MIC4_GAIN_REG46       0x46
#define ES7210_MIC1_POWER_REG47      0x47
#define ES7210_MIC2_POWER_REG48      0x48
#define ES7210_MIC3_POWER_REG49      0x49
#define ES7210_MIC4_POWER_REG4A      0x4A
#define ES7210_MIC12_POWER_REG4B     0x4B /* MICBias & ADC & PGA Power */
#define ES7210_MIC34_POWER_REG4C     0x4C

typedef enum {
  ES7210_AD1_AD0_00 = 0x40,
  ES7210_AD1_AD0_01 = 0x41,
  ES7210_AD1_AD0_10 = 0x42,
  ES7210_AD1_AD0_11 = 0x43,
} es7210_address_t;

typedef enum { ES7210_INPUT_MIC1 = 0x01, ES7210_INPUT_MIC2 = 0x02, ES7210_INPUT_MIC3 = 0x04, ES7210_INPUT_MIC4 = 0x08 } es7210_input_mics_t;

typedef enum gain_value {
  GAIN_0DB = 0,
  GAIN_3DB,
  GAIN_6DB,
  GAIN_9DB,
  GAIN_12DB,
  GAIN_15DB,
  GAIN_18DB,
  GAIN_21DB,
  GAIN_24DB,
  GAIN_27DB,
  GAIN_30DB,
  GAIN_33DB,
  GAIN_34_5DB,
  GAIN_36DB,
  GAIN_37_5DB,
} es7210_gain_value_t;

#define ES7210_DEBUG 1
#if ES7210_DEBUG
#define DEBUG_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#elif
#define DEBUG_PRINTF(format, ...) ;
#endif

struct _coeff_div {
  uint32_t mclk; /* mclk frequency */
  uint32_t lrck; /* lrck */
  uint8_t ss_ds;
  uint8_t adc_div;  /* adcclk divider */
  uint8_t dll;      /* dll_bypass */
  uint8_t doubler;  /* doubler enable */
  uint8_t osr;      /* adc osr */
  uint8_t mclk_src; /* select mclk  source */
  uint32_t lrck_h;  /* The high 4 bits of lrck */
  uint32_t lrck_l;  /* The low 8 bits of lrck */
};

static const struct _coeff_div coeff_div[] = {
    // mclk      lrck    ss_ds adc_div  dll  doubler osr  mclk_src  lrckh   lrckl
    /* 8k */
    {12288000, 8000, 0x00, 0x03, 0x01, 0x00, 0x20, 0x00, 0x06, 0x00},
    {16384000, 8000, 0x00, 0x04, 0x01, 0x00, 0x20, 0x00, 0x08, 0x00},
    {19200000, 8000, 0x00, 0x1e, 0x00, 0x01, 0x28, 0x00, 0x09, 0x60},
    {4096000, 8000, 0x00, 0x01, 0x01, 0x00, 0x20, 0x00, 0x02, 0x00},

    /* 11.025k */
    {11289600, 11025, 0x00, 0x02, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00},

    /* 12k */
    {12288000, 12000, 0x00, 0x02, 0x01, 0x00, 0x20, 0x00, 0x04, 0x00},
    {19200000, 12000, 0x00, 0x14, 0x00, 0x01, 0x28, 0x00, 0x06, 0x40},

    /* 16k */
    {4096000, 16000, 0x00, 0x01, 0x01, 0x01, 0x20, 0x00, 0x01, 0x00},
    {19200000, 16000, 0x00, 0x0a, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x80},
    {16384000, 16000, 0x00, 0x02, 0x01, 0x00, 0x20, 0x00, 0x04, 0x00},
    {12288000, 16000, 0x00, 0x03, 0x01, 0x01, 0x20, 0x00, 0x03, 0x00},

    /* 22.05k */
    {11289600, 22050, 0x00, 0x01, 0x01, 0x00, 0x20, 0x00, 0x02, 0x00},

    /* 24k */
    {12288000, 24000, 0x00, 0x01, 0x01, 0x00, 0x20, 0x00, 0x02, 0x00},
    {19200000, 24000, 0x00, 0x0a, 0x00, 0x01, 0x28, 0x00, 0x03, 0x20},

    /* 32k */
    {12288000, 32000, 0x00, 0x03, 0x00, 0x00, 0x20, 0x00, 0x01, 0x80},
    {16384000, 32000, 0x00, 0x01, 0x01, 0x00, 0x20, 0x00, 0x02, 0x00},
    {19200000, 32000, 0x00, 0x05, 0x00, 0x00, 0x1e, 0x00, 0x02, 0x58},

    /* 44.1k */
    {11289600, 44100, 0x00, 0x01, 0x01, 0x01, 0x20, 0x00, 0x01, 0x00},

    /* 48k */
    {12288000, 48000, 0x00, 0x01, 0x01, 0x01, 0x20, 0x00, 0x01, 0x00},
    {19200000, 48000, 0x00, 0x05, 0x00, 0x01, 0x28, 0x00, 0x01, 0x90},

    /* 64k */
    {16384000, 64000, 0x01, 0x01, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00},
    {19200000, 64000, 0x00, 0x05, 0x00, 0x01, 0x1e, 0x00, 0x01, 0x2c},

    /* 88.2k */
    {11289600, 88200, 0x01, 0x01, 0x01, 0x01, 0x20, 0x00, 0x00, 0x80},

    /* 96k */
    {12288000, 96000, 0x01, 0x01, 0x01, 0x01, 0x20, 0x00, 0x00, 0x80},
    {19200000, 96000, 0x01, 0x05, 0x00, 0x01, 0x28, 0x00, 0x00, 0xc8},
};

class ES7210 {
public:
  ES7210(es7210_address_t address);
  bool begin(TwoWire *wire = &Wire);
  bool mic_select(es7210_input_mics_t mic);
  void adc_config_i2s(audio_hal_codec_mode_t mode, audio_hal_codec_i2s_iface_t *iface);
  void set_bits(audio_hal_iface_bits_t bits);
  int get_coeff(uint32_t mclk, uint32_t lrck);
  void config_fmt(audio_hal_iface_format_t fmt);
  void config_sample(audio_hal_iface_samples_t sample);
  void adc_ctrl_state(audio_hal_codec_mode_t mode, audio_hal_ctrl_t ctrl_state);

  void start(uint8_t clock_reg_value);
  void stop();
  void adc_set_gain(es7210_input_mics_t mic_mask, es7210_gain_value_t gain);
  void adc_set_gain_all(es7210_gain_value_t gain);

  void read_all_reg(void) {
    for (int i = 0; i <= 0x4E; i++) {
      uint8_t reg = readRegister(i);
      printf("REG:%02x, %02x\n", reg, i);
    }
  }

protected:
  bool writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
  bool update_reg_bit(uint8_t reg_addr, uint8_t update_bits, uint8_t data);

  es7210_input_mics_t _mic_select ;

  uint8_t _address;
  TwoWire *_wire;
  uint32_t _error;
};
