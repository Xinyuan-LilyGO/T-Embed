#include "APA102.h"
#include "pin_config.h"

APA102<PIN_APA102_DI, PIN_APA102_CLK> ledStrip;

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

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  Serial.begin(115200);
  Serial.printf("psram size : %d kb\r\n", ESP.getPsramSize() / 1024);
  Serial.printf("FLASH size : %d kb\r\n", ESP.getFlashChipSize() / 1024);
}

void loop() {
  const uint8_t ledSort[7] = {2, 1, 0, 6, 5, 4, 3};
  // Set the number of LEDs to control.
  const uint16_t ledCount = 7;
  // Create a buffer for holding the colors (3 bytes per color).
  rgb_color colors[ledCount];
  // Set the brightness to use (the maximum is 31).
  uint8_t brightness = 1;
  static uint64_t time;
  time++;
  for (uint16_t i = 0; i < ledCount; i++) {
    colors[i] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
  }
  ledStrip.write(colors, ledCount, brightness);
  delay(10);
}