#include "led_driver.h"
#include "comm.h"

static const uint32_t spiFreq = 2000000;

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
  //_myspi.begin(_clkPin, _misoPin, _dataPin, _ssPin);  // start spi connection
  //_myspi.setFrequency(spiFreq);
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
  refresh();
  for(uint8_t i=0; i<4; i++)
    transfer( 0x00);

  // data sequence
  for(uint8_t i=0; i<_length; i++)
  {
    transfer( 0b11100000 | (uint8_t)31 );
    transfer(_buffer[i].b);
    transfer(_buffer[i].g);
    transfer(_buffer[i].r);
  }

  // end sequence
   transfer( 0xff );
   for (uint16_t i = 0; i < 5 + _length / 16; i++){
       transfer( 0x00);
   }

  free(tbuf);
}

void ledDriver::refresh()
{
  digitalWrite(_dataPin, LOW);
  pinMode(_dataPin, OUTPUT);
  digitalWrite(_clkPin, LOW);
  pinMode(_clkPin, OUTPUT);
}
    
void ledDriver::transfer(uint8_t b){
  digitalWrite(_dataPin, b >> 7 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 6 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 5 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 4 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 3 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 2 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 1 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
  digitalWrite(_dataPin, b >> 0 & 1);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_clkPin, LOW);
}

