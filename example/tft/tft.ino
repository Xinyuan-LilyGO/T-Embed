#include "TFT_eSPI.h" // https://github.com/Bodmer/TFT_eSPI
#include "img_logo.h"
#include "pin_config.h"
TFT_eSPI tft = TFT_eSPI();


void setup()
{
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    Serial.begin(115200);
    Serial.printf("psram size : %d kb\r\n", ESP.getPsramSize() / 1024);
    Serial.printf("FLASH size : %d kb\r\n", ESP.getFlashChipSize() / 1024);

    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.pushImage(0, 0, 320, 170, (uint16_t *)img_logo);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    tft.setSwapBytes(true);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);

    tft.drawString("test yellow", 10, 10, 4);


}

void loop()
{
    delay(10);
}