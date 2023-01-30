#pragma once

#include "driver/gpio.h"

typedef struct apa102
{
    uint8_t dataPin;
    uint8_t clockPin;
} apa102_t;

#ifndef _APA102_RGB_COLOR
#define _APA102_RGB_COLOR
/*! A struct that can be used to represent colors.  Each field is a number
 * between 0 and 255 that represents the brightness of a component of the
 * color. */
typedef struct rgb_color
{
    uint8_t red, green, blue;
} rgb_color;
#endif

void apa102_init(const apa102_t *apa102);
void apa102_transfer(const apa102_t *apa102, uint8_t val);
void apa102_startFrame(const apa102_t *apa102);
void apa102_endFrame(const apa102_t *apa102, uint16_t count);
void apa102_sendColor(const apa102_t *apa102,uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
void apa102_sendColor24(const apa102_t *apa102,rgb_color *color, uint8_t brightness);
