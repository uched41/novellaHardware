#ifndef COMM_H
#define COMM_H

#include "myconfig.h"

extern uint8_t brightnessMode;
extern uint8_t brightnessVal;

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
