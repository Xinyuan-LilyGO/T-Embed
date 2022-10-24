/* This example shows how to display a moving two-dimensional
 * rainbow pattern on an APA102-based LED panel.
 *
 * Be sure to edit the ledPanelWidth and ledPanelHeight variables
 * below to match your LED panel.  For the 8x32 panel from Pololu,
 * ledPanelWidth should be 8 and ledPanelHeight should be 32. */

/* This example is meant for controlling large numbers of LEDs,
 * so it requires the FastGPIO library.  If you cannot use the
 * FastGPIO library, you can comment out the two lines below and
 * the example will still work, but it will be slow. */
#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <APA102.h>

// Define which pins to use.
const uint8_t dataPin = 11;
const uint8_t clockPin = 12;

// Create an object for writing to the LED strip.
APA102<dataPin, clockPin> ledStrip;

// Set the size of the LED panel.
const uint8_t ledPanelWidth = 16;
const uint8_t ledPanelHeight = 16;
const uint16_t ledCount = ledPanelWidth * ledPanelHeight;

// Create a buffer for holding the colors (3 bytes per color).
rgb_color colors[ledCount];

// Set the brightness to use (the maximum is 31).
const uint8_t brightness = 1;

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

void setup()
{
}

/* Returns a reference to the color at the specified coordinates,
 * where x specifies the column and y specifies the row.  For
 * efficiency, this function does not check the values of x and
 * y, so x must be less than ledPanelWidth and y must be less
 * than ledPanelHeight. */
rgb_color & color_at(uint8_t x, uint8_t y)
{
  /* The LEDs in the panel are arranged in a serpentine layout,
   * so if we are in an even-numbered column then flip the x
   * coordinate. */
  if (!(y & 1)) { x = ledPanelWidth - 1 - x; }
  return colors[(ledPanelWidth * y) + x];
}

void loop()
{
  uint8_t time = millis() >> 2;
  for (uint8_t x = 0; x < ledPanelWidth; x++)
  {
    for (uint8_t y = 0; y < ledPanelHeight; y++)
    {
      // The x * 20 and y * 10 terms determine the general
      // direction and width of the rainbow, and the x * y term
      // makes it curve a bit.
      uint8_t p = time - x * 20 - y * 10 - x * y;
      color_at(x, y) = hsvToRgb((uint32_t)p * 359 / 256, 255, 255);
    }
  }

  ledStrip.write(colors, ledCount, brightness);
}
