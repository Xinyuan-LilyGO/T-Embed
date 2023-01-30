#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"

// ST7789 Parameters not documented in ESP IDF esp_lcd_panel_commands
#define ST7789_RAMCTRL          0xB0      // RAM control
#define ST7789_PORCTRL          0xB2      // Porch control
#define ST7789_GCTRL            0xB7      // Gate control
#define ST7789_VCOMS            0xBB      // VCOMS setting
#define ST7789_LCMCTRL          0xC0      // LCM control
#define ST7789_IDSET            0xC1      // ID setting
#define ST7789_VDVVRHEN         0xC2      // VDV and VRH command enable
#define ST7789_VRHS             0xC3      // VRH set
#define ST7789_VDVSET           0xC4      // VDV setting
#define ST7789_VCMOFSET         0xC5      // VCOMS offset set
#define ST7789_FRCTR2           0xC6      // FR Control 2
#define ST7789_PWCTRL1          0xD0      // Power control 1
#define ST7789_VAPVANEN         0xD2      // Enable VAP/VAN signal output
#define ST7789_CMD2EN           0xDF      // Command 2 enable
#define ST7789_PVGAMCTRL        0xE0      // Positive voltage gamma control
#define ST7789_NVGAMCTRL        0xE1      // Negative voltage gamma control
#define ST7789_DGMLUTR          0xE2      // Digital gamma look-up table for red
#define ST7789_DGMLUTB          0xE3      // Digital gamma look-up table for blue
#define ST7789_GATECTRL         0xE4      // Gate control
#define ST7789_SPI2EN           0xE7      // SPI2 enable
#define ST7789_PWCTRL2          0xE8      // Power control 2
#define ST7789_EQCTRL           0xE9      // Equalize time control
#define ST7789_PROMCTRL         0xEC      // Program control
#define ST7789_PROMEN           0xFA      // Program mode enable
#define ST7789_NVMSET           0xFC      // NVM setting
#define ST7789_PROMACT          0xFE      // Program action

// Why is this called LCD PIXEL CLOCK when it's really SPI Bus speed?
#define LCD_PIXEL_CLOCK_HZ      (20 * 1000 * 1000)

extern esp_lcd_panel_handle_t tembed_init_lcd_st7789(esp_lcd_panel_io_color_trans_done_cb_t notify_color_trans_done, void *user_data);
