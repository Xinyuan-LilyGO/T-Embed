// Copyright Pololu Corporation.  For more information, see http://www.pololu.com/

#pragma once

/*! \file APA102.h
 * This is the main header file for the APA102 library. */

#include <Arduino.h>

namespace Pololu
{
  #ifndef _POLOLU_RGB_COLOR
  #define _POLOLU_RGB_COLOR
  /*! A struct that can be used to represent colors.  Each field is a number
   * between 0 and 255 that represents the brightness of a component of the
   * color. */
  typedef struct rgb_color
  {
    uint8_t red, green, blue;
    rgb_color() {};
    rgb_color(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {};
  } rgb_color;
  #endif

  /*! An abstract base class for APA102.  This class is useful if you want
   *  to have a pointer that can point to different APA102 objects.
   *
   * The only virtual function provided by this class is write() because making
   * the low-level functions be virtual causes a noticeable slowdown.
   */
  class APA102Base
  {
  public:
    /*! Writes the specified colors to the LED strip.
     *
     * @param colors A pointer to an array of colors.
     * @param count The number of colors to write.
     * @param brightness A 5-bit brightness value (between 0 and 31) that will
     *   be written to each LED.
     */
    virtual void write(rgb_color * colors, uint16_t count, uint8_t brightness = 31) = 0;
  };

  /*! A template class that represents an APA102 or SK9822 LED strip controlled
   * by a particular clock and data pin.
   *
   * @param dataPin The Arduino pin number or name for the pin that will be
   *   used to control the data input.
   *
   * @param clockPin The Arduino pin number or name for the pin that will be
   *   used to control the clock input.
   */
  template<uint8_t dataPin, uint8_t clockPin> class APA102 : public APA102Base
  {
  public:

    virtual void write(rgb_color * colors, uint16_t count, uint8_t brightness = 31)
    {
      startFrame();
      for(uint16_t i = 0; i < count; i++)
      {
        sendColor(colors[i], brightness);
      }
      endFrame(count);
    }

    /*! Initializes the I/O lines and sends a "Start Frame" signal to the LED
     *  strip.
     *
     * This is part of the low-level interface provided by this class, which
     * allows you to send LED colors as you are computing them instead of
     * storing them in an array.  To use the low-level interface, first call
     * startFrame(), then call sendColor() some number of times, then call
     * endFrame(). */
    void startFrame()
    {
      init();
      transfer(0);
      transfer(0);
      transfer(0);
      transfer(0);
    }

    /*! Sends an "End Frame" signal to the LED strip.  This is the last step in
     * updating the LED strip if you are using the low-level interface described
     * in the startFrame() documentation.
     *
     * After this function returns, the clock and data lines will both be
     * outputs that are driving low.  This makes it easier to use one clock pin
     * to control multiple LED strips. */
    void endFrame(uint16_t count)
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
        transfer(0);
      }

      /* We call init() here to make sure we leave the data line driving low
       * even if count is 0 or 1. */
      init();
    }

    /*! Sends a single 24-bit color and an optional 5-bit brightness value.
     * This is part of the low-level interface described in the startFrame()
     * documentation. */
    void sendColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness = 31)
    {
      transfer(0b11100000 | brightness);
      transfer(blue);
      transfer(green);
      transfer(red);
    }

    /*! Sends a single 24-bit color and an optional 5-bit brightness value.
     * This is part of the low-level interface described in the startFrame()
     * documentation. */
    void sendColor(rgb_color color, uint8_t brightness = 31)
    {
      sendColor(color.red, color.green, color.blue, brightness);
    }

  protected:
    void init()
    {
      #ifdef APA102_USE_FAST_GPIO
      FastGPIO::Pin<dataPin>::setOutputLow();
      FastGPIO::Pin<clockPin>::setOutputLow();
      #else
      digitalWrite(dataPin, LOW);
      pinMode(dataPin, OUTPUT);
      digitalWrite(clockPin, LOW);
      pinMode(clockPin, OUTPUT);
      #endif
    }

    void transfer(uint8_t b)
    {
      #ifdef APA102_USE_FAST_GPIO
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 7 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 6 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 5 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 4 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 3 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 2 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 1 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      FastGPIO::Pin<dataPin>::setOutputValue(b >> 0 & 1);
      FastGPIO::Pin<clockPin>::setOutputValueHigh();
      FastGPIO::Pin<clockPin>::setOutputValueLow();
      #else
      digitalWrite(dataPin, b >> 7 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 6 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 5 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 4 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 3 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 2 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 1 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, b >> 0 & 1);
      digitalWrite(clockPin, HIGH);
      digitalWrite(clockPin, LOW);
      #endif
    }

  };
}

using namespace Pololu;
