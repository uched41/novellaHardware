#ifndef CONTROL_H
#define CONTROL_H

#include "FS.h"
#include "SPIFFS.h"

#define MQTT_BASE_TOPIC "novella/devices"
#define MQTT_PING_INTERVAL 10000  //  in milliseconds

extern uint8_t brightnessMode;
extern uint8_t brightnessVal;

#define STATUS_FLASH_INTERVAL   200   // interval in millis when flashing status led

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

    void setBuffer(char* filename){
      File file= SPIFFS.open(filename);
      if(!file) {
        Serial.println("ERROR: failed to open file.");
        return;
      }
      clearBuffer();
    
      uint8_t noCols;
      file.read(&noCols, 1);
     
      _buffer = (uint8_t**)malloc( sizeof(uint8_t*)*noCols );
    
      for(uint8_t it=0; it<noCols; it++)  {
        uint8_t* newCol = (uint8_t*)malloc(_colLength);   // new column array
        file.readBytes((char*)newCol, _colLength);
        _buffer[it]=newCol;
      }
      _noColumns = noCols;
      
      file.close();
      Serial.println("DataBuffer: data split complete");
    }
};

extern DataBuffer dataStore;


class StatusLed{
  uint8_t r, g, b;
  public:
    StatusLed(uint8_t _r, uint8_t _g, uint8_t _b){
      r=_r; g=_g; b=_b;
      pinMode(r, OUTPUT);
      pinMode(g, OUTPUT);
      pinMode(b, OUTPUT);
    }

    void flashRed() {
      digitalWrite(r, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(r, LOW);
      delay(STATUS_FLASH_INTERVAL);
    }

    void flashGreen() {
      digitalWrite(g, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(g, LOW);
      delay(STATUS_FLASH_INTERVAL);
    }

    void flashBlue()  {
      digitalWrite(b, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(b, LOW);
      delay(STATUS_FLASH_INTERVAL);
    }

    void flashYellow(){
      digitalWrite(r, HIGH);
      digitalWrite(g, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(r, LOW);
      digitalWrite(g, LOW);
    }

    void off(){   // function to put off all eds
      digitalWrite(b, LOW);
      digitalWrite(g, LOW);
      digitalWrite(r, LOW);
    }

};

extern StatusLed statusLed;

void mqttInit(void);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
void mqttLoop();
void mqttReply(char* msg);
void mqttPing();
void splitData(char* filename);

#endif
