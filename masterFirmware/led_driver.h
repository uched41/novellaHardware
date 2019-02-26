#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>
#include <memory>
#include <cassert>
#include <stdio.h>
#include <SPI.h>

// structure for storing color
typedef struct
{
 uint8_t r, g, b;
}CRGB;


// Class for led driver
class ledDriver
{
      uint8_t _clkPin;
      uint8_t _misoPin;
      uint8_t _dataPin;
      uint8_t _ssPin;
      SPIClass _myspi;
      CRGB* _buffer;
      uint8_t _length;

  public:
      ledDriver(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin);
      void begin(uint16_t length1);
      void setPixel(uint16_t pixel, CRGB col);
      void setBuffer(uint8_t* buf, uint16_t pixelLen);
      void show();
};


#endif
