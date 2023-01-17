#include "sdkconfig.h"
#include "driver/gpio.h"
#include "tembed.h"

// Define which pins to use.
static struct tembed tembed = {
#ifdef CONFIG_TEMBED_INIT_LEDS
    .leds.dataPin = CONFIG_APA102_DATA_PIN,
    .leds.clockPin = CONFIG_APA102_CLOCK_PIN,
#endif
};

tembed_t tembed_init(
#ifdef CONFIG_TEMBED_INIT_LCD
    esp_lcd_panel_io_color_trans_done_cb_t notify_color_trans_done, void *user_data
#endif
    ) {

#if CONFIG_TEMBED_POWER_PIN != -1
    // Enable power to the T-Embed peripherals
    gpio_config_t pw_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << CONFIG_TEMBED_POWER_PIN
    };
    ESP_ERROR_CHECK(gpio_config(&pw_gpio_config));
    gpio_set_level(CONFIG_TEMBED_POWER_PIN, 1);
#endif

#ifdef CONFIG_TEMBED_INIT_LEDS
    apa102_init(&tembed.leds);
#endif

#ifdef CONFIG_TEMBED_INIT_LCD
    tembed.lcd = tembed_init_lcd_st7789(notify_color_trans_done, user_data);
#endif

#ifdef CONFIG_TEMBED_INIT_DIAL
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = CONFIG_TEMBED_DIAL_BUTTON_IO_NUM ,
            .active_level = CONFIG_TEMBED_DIAL_BUTTON_ACTIVE_LEVEL,
        },
    };

    tembed.dial.btn = iot_button_create(&cfg);

    knob_config_t *kcfg = calloc(1, sizeof(knob_config_t));
    kcfg->default_direction = 0;
    kcfg->gpio_encoder_a = CONFIG_TEMBED_DIAL_KNOB_A;
    kcfg->gpio_encoder_b = CONFIG_TEMBED_DIAL_KNOB_B;

    tembed.dial.knob = iot_knob_create(kcfg);

#endif

    return &tembed;
}
