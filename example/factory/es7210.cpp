#include "es7210.h"
#include "Arduino.h"
#include "Wire.h"

#define ES7210_OK          0x00
#define ES7210_PIN_ERROR   0x81
#define ES7210_I2C_ERROR   0x82
#define ES7210_VALUE_ERROR 0x83
#define ES7210_PORT_ERROR  0x84

#define I2S_DSP_MODE_A     0
#define MCLK_DIV_FRE       256

ES7210::ES7210(es7210_address_t address) {
  _address = address;
  _error = ES7210_OK;
}

bool ES7210::begin(TwoWire *wire) {
  _wire = wire;

  writeRegister(ES7210_RESET_REG00, 0xff);
  writeRegister(ES7210_RESET_REG00, 0x41);
  // writeRegister(ES7210_CLOCK_OFF_REG01, 0x00);
  writeRegister(ES7210_CLOCK_OFF_REG01, 0x1f);
  writeRegister(ES7210_TIME_CONTROL0_REG09, 0x30); /* Set chip state cycle */
  writeRegister(ES7210_TIME_CONTROL1_REG0A, 0x30); /* Set power on state cycle */

  // writeRegister(ES7210_MODE_CONFIG_REG08, 0x20);
  // update_reg_bit(ES7210_MASTER_CLK_REG03, 0x80, 0x80);

  writeRegister(ES7210_ANALOG_REG40, 0xC3);     /* Select power off analog, vdda = 3.3V, close vx20ff, VMID select 5KΩ start */
  writeRegister(ES7210_MIC12_BIAS_REG41, 0x70); /* Select 2.87v */
  writeRegister(ES7210_MIC34_BIAS_REG42, 0x70); /* Select 2.87v */
  writeRegister(ES7210_OSR_REG07, 0x20);
  writeRegister(ES7210_MAINCLK_REG02,
                0xc1); /* Set the frequency division coefficient and use dll except clock doubler, and need to set 0xc1 to clear the state */

  config_sample(AUDIO_HAL_44K_SAMPLES);
  mic_select(_mic_select);
  adc_set_gain_all(GAIN_0DB);

  return true;
}

void ES7210::adc_config_i2s(audio_hal_codec_mode_t mode, audio_hal_codec_i2s_iface_t *iface) {
  set_bits(iface->bits);
  config_fmt(iface->fmt);
  config_sample(iface->samples);
}

int ES7210::get_coeff(uint32_t mclk, uint32_t lrck) {
  for (int i = 0; i < (sizeof(coeff_div) / sizeof(coeff_div[0])); i++) {
    if (coeff_div[i].lrck == lrck && coeff_div[i].mclk == mclk)
      return i;
  }
  return -1;
}

void ES7210::set_bits(audio_hal_iface_bits_t bits) {
  uint8_t adc_iface = 0;
  adc_iface = readRegister(ES7210_SDP_INTERFACE1_REG11);
  adc_iface &= 0x1f;
  switch (bits) {
  case AUDIO_HAL_BIT_LENGTH_16BITS:
    adc_iface |= 0x60;
    break;
  case AUDIO_HAL_BIT_LENGTH_24BITS:
    adc_iface |= 0x00;
    break;
  case AUDIO_HAL_BIT_LENGTH_32BITS:
    adc_iface |= 0x80;
    break;
  default:
    adc_iface |= 0x60;
    break;
  }
  writeRegister(ES7210_SDP_INTERFACE1_REG11, adc_iface);
}

void ES7210::config_fmt(audio_hal_iface_format_t fmt) {
  uint8_t adc_iface = 0;
  adc_iface = readRegister(ES7210_SDP_INTERFACE1_REG11);
  adc_iface &= 0xfc;
  switch (fmt) {
  case AUDIO_HAL_I2S_NORMAL:
    log_d("ES7210 in I2S Format");
    adc_iface |= 0x00;
    break;
  case AUDIO_HAL_I2S_LEFT:
  case AUDIO_HAL_I2S_RIGHT:
    log_d("ES7210 in LJ Format");
    adc_iface |= 0x01;
    break;
  case AUDIO_HAL_I2S_DSP:
    if (I2S_DSP_MODE_A) {
      log_d("ES7210 in DSP-A Format");
      adc_iface |= 0x03;
    } else {
      log_d("ES7210 in DSP-B Format");
      adc_iface |= 0x13;
    }
    break;
  default:
    adc_iface &= 0xfc;
    break;
  }
  writeRegister(ES7210_SDP_INTERFACE1_REG11, adc_iface);
  /* Force ADC1/2 output to SDOUT1 and ADC3/4 output to SDOUT2 */
  // writeRegister(ES7210_SDP_INTERFACE2_REG12, 0x00);
}

void ES7210::config_sample(audio_hal_iface_samples_t sample) {
  uint8_t regv;
  int coeff;
  int sample_fre = 0;
  int mclk_fre = 0;
  switch (sample) {
  case AUDIO_HAL_08K_SAMPLES:
    sample_fre = 8000;
    break;
  case AUDIO_HAL_11K_SAMPLES:
    sample_fre = 11025;
    break;
  case AUDIO_HAL_16K_SAMPLES:
    sample_fre = 16000;
    break;
  case AUDIO_HAL_22K_SAMPLES:
    sample_fre = 22050;
    break;
  case AUDIO_HAL_24K_SAMPLES:
    sample_fre = 24000;
    break;
  case AUDIO_HAL_32K_SAMPLES:
    sample_fre = 32000;
    break;
  case AUDIO_HAL_44K_SAMPLES:
    sample_fre = 44100;
    break;
  case AUDIO_HAL_48K_SAMPLES:
    sample_fre = 48000;
    break;
  default:
    DEBUG_PRINTF("Unable to configure sample rate %dHz", sample_fre);
    break;
  }
  mclk_fre = sample_fre * MCLK_DIV_FRE;
  coeff = get_coeff(mclk_fre, sample_fre);
  if (coeff < 0) {
    DEBUG_PRINTF("Unable to configure sample rate %dHz with %dHz MCLK", sample_fre, mclk_fre);
    return;
  }
  /* Set clock parammeters */
  if (coeff >= 0) {
    /* Set adc_div & doubler & dll */
    regv = readRegister(ES7210_MAINCLK_REG02) & 0x00;
    regv |= coeff_div[coeff].adc_div;
    regv |= coeff_div[coeff].doubler << 6;
    regv |= coeff_div[coeff].dll << 7;
    writeRegister(ES7210_MAINCLK_REG02, regv);
    /* Set osr */
    regv = coeff_div[coeff].osr;
    writeRegister(ES7210_OSR_REG07, regv);
    /* Set lrck */
    regv = coeff_div[coeff].lrck_h;
    writeRegister(ES7210_LRCK_DIVH_REG04, regv);
    regv = coeff_div[coeff].lrck_l;
    writeRegister(ES7210_LRCK_DIVL_REG05, regv);
  }
}

void ES7210::adc_ctrl_state(audio_hal_codec_mode_t mode, audio_hal_ctrl_t ctrl_state) {
  static uint8_t regv;
  esp_err_t ret = ESP_OK;
  // ESP_LOGW(TAG, "ES7210 only supports ADC mode");
  ret = readRegister(ES7210_CLOCK_OFF_REG01);
  if ((ret != 0x7f) && (ret != 0xff)) {
    regv = readRegister(ES7210_CLOCK_OFF_REG01);
  }
  if (ctrl_state == AUDIO_HAL_CTRL_START) {
    DEBUG_PRINTF("The ES7210_CLOCK_OFF_REG01 value before stop is %x\r\n", regv);
    start(regv);
  } else {
    DEBUG_PRINTF("The codec is about to stop\r\n");
    regv = readRegister(ES7210_CLOCK_OFF_REG01);
    stop();
  }
}

void ES7210::adc_set_gain(es7210_input_mics_t mic_mask, es7210_gain_value_t gain) {
  if (gain < GAIN_0DB) {
    gain = GAIN_0DB;
  }
  if (gain > GAIN_37_5DB) {
    gain = GAIN_37_5DB;
  }
  if (mic_mask & ES7210_INPUT_MIC1) {
    update_reg_bit(ES7210_MIC1_GAIN_REG43, 0x0f, gain);
  }
  if (mic_mask & ES7210_INPUT_MIC2) {
    update_reg_bit(ES7210_MIC2_GAIN_REG44, 0x0f, gain);
  }
  if (mic_mask & ES7210_INPUT_MIC3) {
    update_reg_bit(ES7210_MIC3_GAIN_REG45, 0x0f, gain);
  }
  if (mic_mask & ES7210_INPUT_MIC4) {
    update_reg_bit(ES7210_MIC4_GAIN_REG46, 0x0f, gain);
  }
}
void ES7210::adc_set_gain_all(es7210_gain_value_t gain) {
  uint32_t max_gain_vaule = 14;
  uint32_t _gain = gain;
  if (_gain < 0) {
    _gain = 0;
  } else if (_gain > max_gain_vaule) {
    _gain = max_gain_vaule;
  }
  log_d("SET: gain:%d", gain);
  if (_mic_select & ES7210_INPUT_MIC1) {
    update_reg_bit(ES7210_MIC1_GAIN_REG43, 0x0f, _gain);
  }
  if (_mic_select & ES7210_INPUT_MIC2) {
    update_reg_bit(ES7210_MIC2_GAIN_REG44, 0x0f, _gain);
  }
  if (_mic_select & ES7210_INPUT_MIC3) {
    update_reg_bit(ES7210_MIC3_GAIN_REG45, 0x0f, _gain);
  }
  if (_mic_select & ES7210_INPUT_MIC4) {
    update_reg_bit(ES7210_MIC4_GAIN_REG46, 0x0f, _gain);
  }
}

bool ES7210::mic_select(es7210_input_mics_t mic) {
  _mic_select = mic;
  if (_mic_select & (ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2 | ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4)) {
    for (int i = 0; i < 4; i++) {
      _error |= update_reg_bit(ES7210_MIC1_GAIN_REG43 + i, 0x10, 0x00);
    }
    _error |= writeRegister(ES7210_MIC12_POWER_REG4B, 0xff);
    _error |= writeRegister(ES7210_MIC34_POWER_REG4C, 0xff);
    if (_mic_select & ES7210_INPUT_MIC1) {
      DEBUG_PRINTF("Enable ES7210_INPUT_MIC1\n");
      _error |= update_reg_bit(ES7210_CLOCK_OFF_REG01, 0x0b, 0x00);
      _error |= writeRegister(ES7210_MIC12_POWER_REG4B, 0x00);
      _error |= update_reg_bit(ES7210_MIC1_GAIN_REG43, 0x10, 0x10);
    }
    if (_mic_select & ES7210_INPUT_MIC2) {
      DEBUG_PRINTF("Enable ES7210_INPUT_MIC2\n");
      _error |= update_reg_bit(ES7210_CLOCK_OFF_REG01, 0x0b, 0x00);
      _error |= writeRegister(ES7210_MIC12_POWER_REG4B, 0x00);
      _error |= update_reg_bit(ES7210_MIC2_GAIN_REG44, 0x10, 0x10);
    }
    if (_mic_select & ES7210_INPUT_MIC3) {
      DEBUG_PRINTF("Enable ES7210_INPUT_MIC3\n");
      _error |= update_reg_bit(ES7210_CLOCK_OFF_REG01, 0x15, 0x00);
      _error |= writeRegister(ES7210_MIC34_POWER_REG4C, 0x00);
      _error |= update_reg_bit(ES7210_MIC3_GAIN_REG45, 0x10, 0x10);
    }
    if (_mic_select & ES7210_INPUT_MIC4) {
      DEBUG_PRINTF("Enable ES7210_INPUT_MIC4\n");
      _error |= update_reg_bit(ES7210_CLOCK_OFF_REG01, 0x15, 0x00);
      _error |= writeRegister(ES7210_MIC34_POWER_REG4C, 0x00);
      _error |= update_reg_bit(ES7210_MIC4_GAIN_REG46, 0x10, 0x10);
    }
  } else {
    DEBUG_PRINTF("Microphone selection error\n");
    return false;
  }
  return true;
}
void ES7210::start(uint8_t clock_reg_value) {
  // writeRegister(ES7210_SDP_INTERFACE2_REG12, 0x00);

  writeRegister(ES7210_CLOCK_OFF_REG01, clock_reg_value);
  writeRegister(ES7210_POWER_DOWN_REG06, 0x00);

  // writeRegister(ES7210_ANALOG_REG40, 0x40);

  writeRegister(ES7210_MIC1_POWER_REG47, 0x00);
  writeRegister(ES7210_MIC2_POWER_REG48, 0x00);
  writeRegister(ES7210_MIC3_POWER_REG49, 0x00);
  writeRegister(ES7210_MIC4_POWER_REG4A, 0x00);
  mic_select(_mic_select);


}

void ES7210::stop() {
  writeRegister(ES7210_MIC1_POWER_REG47, 0xff);
  writeRegister(ES7210_MIC2_POWER_REG48, 0xff);
  writeRegister(ES7210_MIC3_POWER_REG49, 0xff);
  writeRegister(ES7210_MIC4_POWER_REG4A, 0xff);
  writeRegister(ES7210_MIC12_POWER_REG4B, 0xff);
  writeRegister(ES7210_MIC34_POWER_REG4C, 0xff);
  //  writeRegister(ES7210_ANALOG_REG40, 0xc0);
  writeRegister(ES7210_CLOCK_OFF_REG01, 0x7f);
  writeRegister(ES7210_POWER_DOWN_REG06, 0x07);
}

bool ES7210::writeRegister(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  if (_wire->endTransmission() != 0) {
    _error = ES7210_I2C_ERROR;
    return false;
  }
  _error = ES7210_OK;
  return true;
}

uint8_t ES7210::readRegister(uint8_t reg) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  int rv = _wire->endTransmission();
  if (rv != 0) {
    _error = ES7210_I2C_ERROR;
    return rv;
  } else {
    _error = ES7210_OK;
  }
  _wire->requestFrom(_address, (uint8_t)1);
  return _wire->read();
}

bool ES7210::update_reg_bit(uint8_t reg_addr, uint8_t update_bits, uint8_t data) {
  uint8_t regv;
  regv = readRegister(reg_addr);
  regv = (regv & (~update_bits)) | (update_bits & data);
  return writeRegister(reg_addr, regv);
}