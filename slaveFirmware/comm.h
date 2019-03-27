#ifndef COMM_H
#define COMM_H

#include "myconfig.h"

// Settings object
class Settings{
  public:
    uint8_t brightnessMode = 0;  // 0 = manual, 1, automatic
    uint8_t brightnessPercent = 50;  // value of brightness in percentage
    uint8_t brightnessRaw = 16;  // brightness value on 1 - 32 scale
    int divider = 16;      // divider for colors
    uint8_t isPaired = 0;
    
    int delayBtwColumns = 50;
   
    void setBrightness(uint8_t val){
      brightnessPercent = val;
      brightnessRaw = map(val, 0, 100, 1, 31);
    }

    int getDivider(){
      return divider;
    }
    
    uint8_t getBrightness(){return brightnessRaw;
      /*if(brightnessMode == 0){
        return brightnessRaw;
      }
      else if(brightnessMode == 1){
        return  (uint8_t)map(analogRead(LDR_PIN), 0, 1023, 31, 0);
      }*/
    }
};

extern Settings mySettings;


// Buffer object type
class DataBuffer{
     int _colLength;
  public:
    uint8_t** _buffer;
    int _noColumns;
     
    DataBuffer(int columnLength){
      _buffer = NULL;
      _noColumns = 0;
      _colLength = columnLength;
    }
    
    void clearBuffer(){     
      Serial.println("DataBuffer: clearing buffer");
      for(int i=0; i< _noColumns; i++){
        free(_buffer[i]);
      }
    }
  
    void setBuffer(uint8_t** buf, int len){
      clearBuffer();
      _buffer = (uint8_t**)malloc( sizeof(uint8_t*)*len );
      for(int i=0; i<len; i++){
        uint8_t* newBuf = (uint8_t*)malloc(_colLength);
        memcpy(newBuf, buf[i], _colLength);
        _buffer[i] = newBuf;
      }
      _noColumns = len;
      Serial.println("DataBuffer: Done Setting buffer");
    }
};

extern DataBuffer dataStore;

void initComm();
void commParser();

bool isEqual(uint8_t* buf1, uint8_t* buf2, uint16_t len);
bool waitForData(uint8_t* buf, uint16_t len);
void dumpBuf(uint8_t *data, uint16_t length) ;
void sendBuf(uint8_t* buf, uint16_t len);
unsigned short crc(const unsigned char* data_p, unsigned char length);
void serClear();
bool checkCrc(uint8_t* buf, uint16_t buflen);
#endif
