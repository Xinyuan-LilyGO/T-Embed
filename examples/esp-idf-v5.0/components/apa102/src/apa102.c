#include "esp_log.h"
#include "apa102.h"

#define TAG "APA102"

void apa102_write(const apa102_t* apa102,rgb_color *colors, uint16_t count, uint8_t brightness)
{
    apa102_startFrame(apa102);
    for(uint16_t i = 0; i < count; i++)
    {
        apa102_sendColor24(apa102,&colors[i], brightness);
    }
    apa102_endFrame(apa102,count);
}

/*! Initializes the I/O lines and sends a "Start Frame" signal to the LED
 *  strip.
 *
 * This is part of the low-level interface provided by this class, which
 * allows you to send LED colors as you are computing them instead of
 * storing them in an array.  To use the low-level interface, first call
 * startFrame(), then call sendColor() some number of times, then call
 * endFrame(). */
void apa102_startFrame(const apa102_t *apa102)
{
    apa102_init(apa102);
    apa102_transfer(apa102,0);
    apa102_transfer(apa102,0);
    apa102_transfer(apa102,0);
    apa102_transfer(apa102,0);
}

/*! Sends an "End Frame" signal to the LED strip.  This is the last step in
 * updating the LED strip if you are using the low-level interface described
 * in the startFrame() documentation.
 *
 * After this function returns, the clock and data lines will both be
 * outputs that are driving low.  This makes it easier to use one clock pin
 * to control multiple LED strips. */
void apa102_endFrame(const apa102_t *apa102, uint16_t count)
{
    /* The data stream seen by the last LED in the chain will be delayed by
     * (count - 1) clock edges, because each LED before it inverts the clock
     * line and delays the data by one clock edge.  Therefore, to make sure
     * the last LED actually receives the data we wrote, the number of extra
     * edges we send at the end of the frame must be at least (count - 1).
     *
     * Assuming we only want to send these edges in groups of size K, the
     * C/C++ expression for the minimum number of groups to send is:
     *
     *   ((count - 1) + (K - 1)) / K
     *
     * The C/C++ expression above is just (count - 1) divided by K,
     * rounded up to the nearest whole number if there is a remainder.
     *
     * We set K to 16 and use the formula above as the number of frame-end
     * bytes to transfer.  Each byte has 16 clock edges.
     *
     * We are ignoring the specification for the end frame in the APA102
     * datasheet, which says to send 0xFF four times, because it does not work
     * when you have 66 LEDs or more, and also it results in unwanted white
     * pixels if you try to update fewer LEDs than are on your LED strip. */

    for (uint16_t i = 0; i < (count + 14)/16; i++)
    {
        apa102_transfer(apa102,0);
    }

    /* We call init() here to make sure we leave the data line driving low
     * even if count is 0 or 1. */
    apa102_init(apa102);
}

/*! Sends a single 24-bit color and an optional 5-bit brightness value.
 * This is part of the low-level interface described in the startFrame()
 * documentation. */
void apa102_sendColor(const apa102_t *apa102,uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    apa102_transfer(apa102,0b11100000 | brightness);
    apa102_transfer(apa102,blue);
    apa102_transfer(apa102,green);
    apa102_transfer(apa102,red);
}

/*! Sends a single 24-bit color and an optional 5-bit brightness value.
 * This is part of the low-level interface described in the startFrame()
 * documentation. */
void apa102_sendColor24(const apa102_t * apa102,rgb_color *color, uint8_t brightness)
{
    apa102_sendColor(apa102,color->red, color->green, color->blue, brightness);
}

void apa102_init(const apa102_t *apa102)
{
    ESP_LOGI(TAG, "Init clk:%d data:%d", apa102->clockPin, apa102->dataPin);
    gpio_config_t d_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << apa102->dataPin
    };
    ESP_ERROR_CHECK(gpio_config(&d_gpio_config));
    gpio_set_level(apa102->dataPin, 0);
    gpio_config_t c_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << apa102->clockPin
    };
    ESP_ERROR_CHECK(gpio_config(&c_gpio_config));
    gpio_set_level(apa102->clockPin, 0);
}

void apa102_transfer(const apa102_t *apa102,uint8_t b)
{
    const uint8_t dataPin = apa102->dataPin;
    const uint8_t clockPin = apa102->clockPin;
    gpio_set_level(dataPin,b >> 7 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 6 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 5 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 4 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 3 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 2 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 1 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
    gpio_set_level(dataPin,b >> 0 & 1);
    gpio_set_level(clockPin,1);
    gpio_set_level(clockPin,0);
}
