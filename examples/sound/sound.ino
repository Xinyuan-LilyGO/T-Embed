
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

  //  *     ArduinoIDE only 1.8.19 - upload SPIFFS data with ESP32 Sketch Data Upload:
  //  *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
  //  *     Platformio
  // *      ENV -> sound -> Platform -> Upload Filesystem Image 
  bool ret = SPIFFS.begin();
  while(!ret){
    Serial.println("SPIFFS not mount . please Upload mp3 file");delay(1000);
  }
  audio = new Audio(0, 3, 1);
  audio->setPinout(PIN_IIS_BCLK, PIN_IIS_WCLK, PIN_IIS_DOUT);
  audio->setVolume(21); // 0...21
  if(!audio->connecttoFS(SPIFFS, "/sound1.mp3")){
    Serial.println("No find sound1.mp3.");while(1)delay(1000);
  }
  Serial.println("Now play sound ...");
}

void loop() {
  delay(1);
  audio->loop();
}