#pragma once
#include <FastLED.h>

extern CRGB leds[];  // defined in usb_msc.cpp, used for FastLED.addLeds in main.cpp

void usb_msc_begin(void);
