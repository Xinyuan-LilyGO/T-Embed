
#include "Audio.h" // https://github.com/schreibfaul1/ESP32-audioI2S
#include "FS.h"
#include "SPIFFS.h"
#include "pin_config.h"

Audio *audio;

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  Serial.begin(115200);
  Serial.printf("psram size : %d kb\r\n", ESP.getPsramSize() / 1024);
  Serial.printf("FLASH size : %d kb\r\n", ESP.getFlashChipSize() / 1024);

  SPIFFS.begin(true);
  audio = new Audio(0, 3, 1);
  audio->setPinout(PIN_IIS_BCLK, PIN_IIS_WCLK, PIN_IIS_DOUT);
  audio->setVolume(21); // 0...21
  audio->connecttoFS(SPIFFS, "/sound1.mp3");
}

void loop() {
  delay(1);
  audio->loop();
}