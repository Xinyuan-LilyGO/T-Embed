# APA102/SK9822 library for Arduino

Version: 3.0.0<br/>
Release date: 2018-11-14<br/>
[![Build Status](https://travis-ci.org/pololu/apa102-arduino.svg?branch=master)](https://travis-ci.org/pololu/apa102-arduino) <br>

## Summary

This is a C++ library for the Arduino IDE that helps control addressable [RGB LED strips and panels based on the SK9822/APA102/APA102C RGB LED controller ICs](https://www.pololu.com/category/178).  This library provides full access to the 24-bit color register and 5-bit brightness register of each LED.

The library provides a high-level interface where you store all the LED strip colors in an array and then write them to the LED strip.  It also has a lower-level interface that allows you to send colors to the strip as you are computing them, which reduces RAM usage.

## Supported platforms

This library is designed to work with the Arduino IDE versions 1.6.x or later and will probably not work with older versions.  This library supports any Arduino-compatible board.

## Getting started

### Hardware

An LED strip based on the SK98222 or APA102C can be purchased from Pololu's website.  The LED strip's input connector has three pins that should be connected to the Arduino.  The LED strip's ground will need to be connected to one of the Arduino's GND pins.  The LED strip's data input line (DI or DIN) will need to be connected to one of the Arduino's I/O lines.  The LED strip's clock input line (CI or CIN) will also need to be connected to one of the Arduino's I/O lines.  Our examples use pin 11 as the data pin and pin 12 as the clock pin.  These connections can be made using three [Male-Female Premium Jumper Wires](http://www.pololu.com/catalog/category/67) with the female ends plugging into the LED strip.

You will also need to connect a suitable power supply to the LED strip using the power wires.  The power supply must be at the right voltage and provide enough current to meet the LED strip's requirements.

### Software

If you are using version 1.6.2 or later of the [Arduino software (IDE)](https://www.arduino.cc/en/Main/Software), you can use the Library Manager to install this library:

1. In the Arduino IDE, open the "Sketch" menu, select "Include Library", then "Manage Libraries...".
2. Search for "APA102".
3. Click the APA102 entry in the list.
4. Click "Install".

If this does not work, you can manually install the library:

1. Download the [latest release archive from GitHub](https://github.com/pololu/apa102-arduino/releases) and decompress it.
2. Rename the folder "apa102-arduino-xxxx" to "APA102".
3. Drag the "APA102" folder into the "libraries" directory inside your Arduino sketchbook directory.  You can view your sketchbook location by opening the "File" menu and selecting "Preferences" in the Arduino IDE.  If there is not already a "libraries" folder in that location, you should make the folder yourself.
4. After installing the library, restart the Arduino IDE.

## Examples

Several example sketches are available that show how to use the library. You can access them from the Arduino IDE by opening the "File" menu, selecting "Examples", and then selecting "APA102". If you cannot find these examples, the library was probably installed incorrectly and you should retry the installation instructions above.

## Timing details

The APA102 LEDs have no specific timing requirements.  This library does not explicitly enable or disable any interrupts, and the occurrence of interrupts that last less than a few milliseconds does not interfere with this library.

If an interrupt or series of interrupts occurs that preempts the library code for more than a few milliseconds as you are writing colors to the LED strip, then you could see visible glitches on the LED strip.

An SK9822 or APA102C LED will start displaying its new color as soon as it receives the color.  The update time is not coordinated with the other LEDs in the strip.  If a long interrupt happens while the color data is being sent, you might notice that the beginning of the strip got updated before the end of the strip.  Also, the APA102C turns off after receiving the second-to-last bit of its new color.


## Speed

By default, this library uses the `pinMode` and `digitalWrite` functions provided by the Arduino environment to control the LED strip.  On an ATmega32U4-based board running at 16 MHz, using Arduino 1.8.7, we found that it takes this library 28.2 milliseconds to update 60 LEDs.  That means the maximum update rate for that number of LEDs is only 35 Hz.

To support faster update rates, this library has an option that makes it use the [FastGPIO library](https://github.com/pololu/fastgpio-arduino).  If the FastGPIO library supports your board, then we recommend installing FastGPIO and adding these lines to the top of your sketch to make APA102 use it:

~~~~{.cpp}
#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO
#include <APA102.h>
~~~~

With FastGPIO, it takes about 1.40 milliseconds to update 60 LEDs on an ATmega32U4-based board running at 16 MHz; the update is faster by a factor of 20.

## Creating an APA102 object

To create an APA102 object that represents an SK9822/APA102 LED strip, add code like this near the top of your sketch:

~~~~{.cpp}
const uint8_t dataPin = 11;
const uint8_t clockPin = 12;
APA102<dataPin, clockPin> ledStrip;
~~~~

The APA102 object works with both SK9822 LED strips and APA102 LED strips.

## High-level interface

The APA102 class provides a high-level interface that allows you to pass in an array of colors.  This interface is similar to the interface provided by other LED strip libraries like [PololuLedStrip](https://github.com/pololu/pololu-led-strip-arduino).

First, you need to define an array to hold your LED colors.  This array will take 3 bytes of RAM per LED.  The first entry in the array corresponds to the LED closest to the input connector.  You can put this code near the top of your sketch to define the array:

~~~~{.cpp}
const uint16_t ledCount = 60;
rgb_color colors[ledCount];
~~~~

Then you need to write code to update the colors in the array.  Our Gradient example uses this simple loop:

~~~~{.cpp}
uint8_t time = millis() >> 2;
for(uint16_t i = 0; i < ledCount; i++)
{
  uint8_t x = time - i * 8;
  colors[i].red = x;
  colors[i].green = 255 - x;
  colors[i].blue = x;
}
~~~~

After updating the colors, you can write them to the LED strip:

~~~~{.cpp}
ledStrip.write(colors, ledCount, 31);
~~~~

The last parameter to `write()` is an optional brightness value from 0 to 31.  The default value is 31, which is the brightest setting.  This value gets written to the 5-bit brightness register of each APA102 LED in the strip.  This provides a convenient way to adjust the brightness of your LEDs without having to do expensive calculations and without losing 24-bit color resolution.

For a complete example sketch showing how to use the high-level interface, see the Gradient example included with this library.

One limitation of the APA102 high-level interface is that it does not provide independent control over the 5-bit brightness register in each LED; that register will be set to the same value in each LED.  Another limitation is that all the LED colors must be stored in an array ahead of time, which takes up RAM.  Also, it is not be possible to handle other tasks while the update is happening.  To get around these limitations, the library also provides a low-level interface, which is described below.

## Low-level interface

With the low-level interface, you don't need to define or update an array of colors ahead of time.  Instead, just call `startFrame()` when you want to start updating the colors on the LED strip:

~~~~{.cpp}
ledStrip.startFrame();
~~~~

Then call `sendColor()` multiple times to send the colors to the LEDs.  The first call corresponds to the LED that is closest to the input connector.  The `sendColor()` function can either take an `rgb_color` object as an argument or it can take three individual arguments for the red, green, and blue components of the color.  The `sendColor()` function also has an optional brightness parameter, just like the `write()` function.

~~~~{.cpp}
for(uint16_t i = 0; i < ledCount; i++)
{
  ledStrip.sendColor(77, 8, 10, 12);
}
~~~~

After you are done sending colors, call `endFrame` with the number of LEDs that you updated:

~~~~{.cpp}
ledStrip.endFrame(ledCount);
~~~~

For a complete example sketch showing how to use the low-level interface, see the Brightness example included with this library.

## Chaining LED strips together

No special code is required to control a chain of multiple LED strips that have been connected together using their input and output connectors.  An LED strip with *X* LEDs chained an LED strip with *Y* LEDs and be controlled in exactly the same way as a single LED strip with *X*+*Y* LEDs.

## Controlling multiple chains of LEDs

Multiple chains of SK9822/APA102 LEDs can be controlled by creating multiple `APA102` objects with different names, using different pins and different color arrays.

If you want to conserve I/O pins, we recommend wiring the clock inputs of all the LED chains together and controlling them with a single I/O line, while using separate I/O lines for each data input.  It would also be possible to control all the data lines with a single I/O line and use separate lines for each clock input.  However, we recommend the single clock wiring because it allows the possibility of writing advanced code that efficiently writes to all of the LED chains simultaneously.

## Version history

* 3.0.0 (2018-11-14):
  * Change the code back to the way it was in version 1.2.0.  The SK9822 protocol is actually compatible with the APA102C protocol, so no changes were needed to support it.
* 2.0.0 (2017-05-15):
  * Added support for the SK9822 IC.
  * To support the SK9822, the `endFrame` function was changed.  As a side
    effect, if you try to write to a smaller number of LEDs than are on your
    LED strip, the LED after the last one you update will be set to black.
  * These changes were actually unnecessary.
* 1.2.0 (2017-03-20): Added a constructor for rgb_color that takes the three color values and changed the examples to use it.
* 1.1.2 (2017-02-07): Update testing for new FastGPIO directory layout
* 1.1.1 (2016-08-19): Add continuous integration testing
* 1.1.0 (2016-04-06):
    * Added two examples for two-dimensional LED panels: RainbowPanel and GameOfLife.
    * Changed the Xmas example to use a brightness of 1 by default, like the other examples.
* 1.0.0 (2015-07-07): Original release.
