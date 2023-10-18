#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "hal/spi_types.h"
#include "driver/spi_master.h"
#include "img_logo.h"
#include "esp_err.h"
#include "esp_log.h"

#include "st7789.h"

#define TAG "lcd"

// Commands for the LCD panel on init
typedef struct {
    uint8_t cmd;
    uint8_t data[14]; // TODO: Seems like a waste of space, can do better surely
    uint8_t len;
} lcd_cmd_t;

esp_lcd_panel_handle_t tembed_init_lcd_st7789(esp_lcd_panel_io_color_trans_done_cb_t color_trans_done, void *user_data) {

    ESP_LOGI(TAG, "Backlight off");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << 15
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));


    ESP_LOGI(TAG, "Init SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = 12,
        .mosi_io_num = 11,
        .miso_io_num = -1,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .max_transfer_sz = 320 * 80 * sizeof(uint16_t), // transfer 80 lines of pixels (assume pixel is RGB565) at most in one SPI transaction,
        .flags = SPICOMMON_BUSFLAG_GPIO_PINS
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO)); // Enable the DMA feature

    ESP_LOGI(TAG, "Init panel io");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = 13,
        .cs_gpio_num = 10,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 2,
        .trans_queue_depth = 10,
        .flags.dc_low_on_data = 0,
        .flags.lsb_first = 0,
        .flags.sio_mode = 1,
        .flags.cs_high_active = 0,
        .on_color_trans_done = color_trans_done,
        .user_ctx = user_data,
    };

// Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Attach panel");

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = 9,
        .flags.reset_active_high = 0,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };

    // Create LCD panel handle for ST7789, with the SPI IO device handle
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_LOGI(TAG, "Init lcd");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Command sequence from https://github.com/Xinyuan-LilyGO/T-Embed/blob/main/example/tft/tft.ino#L12
    lcd_cmd_t lcd_st7789v[] = {
        {ST7789_PORCTRL, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
        {ST7789_GCTRL, {0X75}, 1},
        {ST7789_VCOMS, {0X28}, 1},
        {ST7789_LCMCTRL, {0X2C}, 1},
        {ST7789_VDVVRHEN, {0X01}, 1},
        {ST7789_VRHS, {0X1F}, 1},
        {ST7789_FRCTR2, {0X13}, 1},
        {ST7789_PWCTRL1, {0XA7}, 1},
        {ST7789_PWCTRL1, {0XA4, 0XA1}, 2},
        {0xD6, {0XA1}, 1},
        {ST7789_PVGAMCTRL, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
        {ST7789_NVGAMCTRL, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
        {LCD_CMD_INVON,{0},0}
        // CAS and RAS are set by the ESP_lcd driver
    };

    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, lcd_st7789v[i].cmd, lcd_st7789v[i].data, lcd_st7789v[i].len & 0x7f));
        if (lcd_st7789v[i].len & 0x80) {
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }

    ESP_LOGI(TAG, "Enable lcd");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Backlight on");
    gpio_set_level(15, 1);

    esp_lcd_panel_set_gap(panel_handle, 0, 35); // Some offset from the start of the line to where the display actually is

    // Swap coordinates around to match the display
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);

    // Draw the LILLYGO Logo as a test and whilst we're initializing the rest of the app
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 320, 170, img_logo);

    return panel_handle;
}
