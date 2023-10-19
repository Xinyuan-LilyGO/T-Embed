/**
 * @file      TFTHelper.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-19
 *
 */
#pragma once

#include <TFT_eSPI.h>

#define BOARD_POWER_ON_PIN                  46

#define BOARD_SDA_PIN                       18
#define BOARD_SCL_PIN                       8

#define BOARD_PIXELS_SCK_PIN                45
#define BOARD_PIXELS_DIN_PIN                42

#define BOARD_ENCODE_A_PIN                  2
#define BOARD_ENCODE_B_PIN                  1
#define BOARD_ENCODE_CENTEN_PIN             0

#define BOARD_TFT_BL                        15
#define BOARD_TFT_DC                        13
#define BOARD_TFT_CS                        10
#define BOARD_TFT_SCK                       12
#define BOARD_TFT_MOSI                      11
#define BOARD_TFT_RST                       9

#define BOARD_ADC_PIN                       4

#define BOARD_DECODER_BCK_PIN               7
#define BOARD_DECODER_SCK_PIN               5
#define BOARD_DECODER_DOUT_PIN              6

#define BOARD_ES7210_BCK_PIN                47
#define BOARD_ES7210_LRCK_PIN               21
#define BOARD_ES7210_DIN_PIN                14
#define BOARD_ES7210_MCLK_PIN               48


#define BOARD_SDCARD_CS_PIN                 39

// SPI Share with SDCard , NFC , Radio(CC1101)
#define BOARD_SPI_SCK_PIN                   40
#define BOARD_SPI_MOSI_PIN                  41
#define BOARD_SPI_MISO_PIN                  38

#define SHIELD_RADIO_CS_PIN                 17
#define SHIELD_RADIO_DIO0_PIN               18
#define SHIELD_RADIO_DIO2_PIN               8
#define SHIELD_RADIO_SW1_PIN                41
#define SHIELD_RADIO_SW0_PIN                43
#define SHIELD_RADIO_RST_PIN                -1
#define SHIELD_NFC_CS_PIN                   16



void initTFT();
void setBrightness(uint8_t value);

extern TFT_eSPI tft;











