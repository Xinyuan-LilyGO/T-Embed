# Battery 18650 Stats

Library to keeps easy to obtain 18650 (battery) charge level in Arduino environment.

This lib was made on top of [Pangodream 18650CL](https://github.com/pangodream/18650CL)

## Compatibility

This Lib theoretically compatible with any ESP that has ACP pins and a 18650 Battery. It's tested with:

- [x] [LILYGO-T-Energy (T18)](https://github.com/LilyGO/LILYGO-T-Energy)

## Usage

### Import and setup
To use this Lib, you need to import and setup:
```cpp
#include <Battery18650Stats.h>

// #define ADC_PIN 35 
// #define CONVERSION_FACTOR 1.7 
// #define READS 5 

Battery18650Stats battery();
// Battery18650Stats battery(ADC_PIN);
// Battery18650Stats battery(ADC_PIN, CONVERSION_FACTOR);
// Battery18650Stats battery(ADC_PIN, CONVERSION_FACTOR, READS);
```

Constructor parameters:
```cpp
Battery18650Stats(<adc_pin>, <conversion_factor>, <reads>);
```

- `adc_pin` (optional): The ADC Pin that lib will read (analogRead) to calculate charge level. Can be obtained at device datasheet. Default Value: `35`;
- `conversion_factor` (optional): Value used to convert ADC pin reading in real battery voltage. This value can be obtained through comparisons between code result and a voltmeter result. Default Value: `1.702`;
- `reads` (optional): Quantity of reads to get an average of pin readings. Its used due pin reading instabilities. Default Value: `20`;

### Methods

After the installation and instantiation, we are able to obtain the charge level and current battery voltage.

#### Method `double getBatteryVolts()`
Returns the current battery voltage.

#### Method `int getBatteryChargeLevel(bool useConversionTable = false)`
Returns the current battery charge level.
  - Parameter `bool useConversionTable`: Indicates if the internal charge level will be obtained using the internal predefined conversion table instead the formula (default). Default value: `false`
> Attention! Using predefined conversion table will consume more RAM than using the formula.

### Usage example
```cpp
#include <Battery18650Stats.h>

Battery18650Stats battery(35);

void setup() {
  Serial.begin(115200);
  Serial.print("Volts: ");
  Serial.println(battery.getBatteryVolts());
	
  Serial.print("Charge level: ");
  Serial.println(battery.getBatteryChargeLevel());
  
  Serial.print("Charge level (using the reference table): ");
  Serial.println(battery.getBatteryChargeLevel(true));
}

void loop {
  //
}
```

## Parameters and calibrations
This is the tested parameters and calibrations for given devices

| Device          | ADC Pin | Conversion Factor |
|-----------------|---------|-------------------|
| [LILYGO-T-Energy (T18)](https://github.com/LilyGO/LILYGO-T-Energy) | 35      | 1.702             |

## License
The MIT License (MIT). Please see License File for more information.
