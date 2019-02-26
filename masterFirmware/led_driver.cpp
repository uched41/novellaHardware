#include "led_driver.h"
#include "myconfig.h"
#include "myControl.h"

ledDriver::ledDriver(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin)
{
  _clkPin = clkPin;
  _misoPin = misoPin;
  _dataPin = dataPin;
  _ssPin = ssPin;
  _myspi = myspi;
}

void ledDriver::begin(uint16_t length1)
{
  _length = length1;
  _myspi.begin(_clkPin, _misoPin, _dataPin, _ssPin);  // start spi connection
  _buffer =(CRGB*) malloc(sizeof(CRGB)*_length);
}


void ledDriver::setPixel(uint16_t pixel, CRGB col)
{
  _buffer[pixel] = col;
}


void ledDriver::setBuffer(uint8_t* buf, uint16_t pixelLen)
{
    uint16_t tcount = 0;
    for(uint16_t i=0; i<pixelLen; i++)
    {
      CRGB tcol = { buf[tcount++], buf[tcount++], buf[tcount++] };
      _buffer[i] = tcol;
    }
}


void ledDriver::show()
{
  uint8_t* tbuf;
  tbuf = (uint8_t*)malloc( (sizeof(CRGB)+1) *_length + 8);

  uint16_t posi = 0;

  // start sequence
  for(uint8_t i=0; i<4; i++)
    tbuf[posi++] = 0x00;

  // data sequence
  for(uint8_t i=0; i<_length; i++)
  {
    tbuf[posi++] = 0xf0;
    tbuf[posi++] = _buffer[i].b / brightnessVal;    // rgb brightness scaling
    tbuf[posi++] = _buffer[i].g / brightnessVal;
    tbuf[posi++] = _buffer[i].r / brightnessVal;
  }

  // end sequence
  for(uint8_t i=0; i<4; i++)
    tbuf[posi++] = 0xff;

  _myspi.transferBytes(tbuf, NULL, posi);

  free(tbuf);
}
