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
  _myspi.setFrequency(40000000);
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

void ledDriver::startFrame(){
  _myspi.transfer(0);
  _myspi.transfer(0);
  _myspi.transfer(0);
  _myspi.transfer(0);
}

void ledDriver::endFrame(uint8_t count){
  _myspi.transfer(0xFF);
  for (uint16_t i = 0; i < 5 + count / 16; i++)
  {
    _myspi.transfer(0);
  }
}

void ledDriver::sendColor(CRGB col){
  _myspi.transfer(0b11100000 | mySettings.getBrightness());   // global brightness
  _myspi.transfer(col.b);
  _myspi.transfer(col.g);
  _myspi.transfer(col.r);
}

void ledDriver::show(){
  startFrame();
  for(uint8_t i=0; i<_length; i++){
    sendColor(_buffer[i]);   
  }
  endFrame(_length);
}

void ledDriver::clear(){
  startFrame();
  CRGB val = {0, 0, 0};
  for(uint8_t i=0; i<_length; i++){
    sendColor(val);    
  }

  endFrame(_length);
}

