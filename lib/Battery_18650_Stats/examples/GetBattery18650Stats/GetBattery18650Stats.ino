#include <Arduino.h>
#include <Battery18650Stats.h>

#define ADC_PIN 35

Battery18650Stats battery(ADC_PIN);

void setup() {
  Serial.begin(115200);

  Serial.print("Volts: ");
  Serial.println(battery.getBatteryVolts());

  Serial.print("Charge level: ");
  Serial.println(battery.getBatteryChargeLevel());

  Serial.print("Charge level (using the reference table): ");
  Serial.println(battery.getBatteryChargeLevel(true));
}

void loop() {
  //
}
