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
  _myspi.setFrequency(30000000);
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
    memcpy(_buffer, buf, pixelLen*3);
}

void ledDriver::startFrame(){
   uint8_t zBuf[4] = {0, 0, 0, 0};
   _myspi.writeBytes(zBuf, 4);
}

void ledDriver::endFrame(uint8_t count){
  int num = 5 + count / 16;
  uint8_t mBuf[num+1];
  memset(mBuf, 0, num+1);
  mBuf[0] = 0xFF;
  
  _myspi.writeBytes((uint8_t*)mBuf, num+1);
}

void ledDriver::sendColor(CRGB col){
   _myspi.write(0b11100000 | mySettings.brightnessRaw);   // global brightness
   _myspi.write(col.b/mySettings.divider);
   _myspi.write(col.g/mySettings.divider);
   _myspi.write(col.r/mySettings.divider);
}

void ledDriver::show(){
  startFrame();
  uint8_t mBuf[_length*4];
  int mBufPointer = 0;
  
  for(int i=0; i<_length; i++){   
    mBuf[mBufPointer++] = 0b11100000 | mySettings.brightnessRaw ;
    mBuf[mBufPointer++] = _buffer[i].b /mySettings.divider ;
    mBuf[mBufPointer++] = _buffer[i].g /mySettings.divider ;
    mBuf[mBufPointer++] = _buffer[i].r /mySettings.divider ;
  }
  _myspi.writeBytes((uint8_t*)mBuf, (int)mBufPointer);
  endFrame(_length);
}

void ledDriver::clear(){
  startFrame();
  CRGB val = {0, 0, 0};
  uint8_t mBuf[_length*4];
  int mBufPointer = 0;
  
  for(int i=0; i<_length; i++){
    mBuf[mBufPointer++] = 0b11100000 | mySettings.brightnessRaw ;
    mBuf[mBufPointer++] = 0;
    mBuf[mBufPointer++] = 0 ;
    mBuf[mBufPointer++] = 0 ;  
  }
  _myspi.writeBytes((uint8_t*)mBuf, (int)mBufPointer);
  endFrame(_length);
}



