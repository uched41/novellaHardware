#ifndef CONTROL_H
#define CONTROL_H

#include "FS.h"
#include "SPIFFS.h"
#include "comm.h"

#define MQTT_BASE_TOPIC "novella/devices"
#define MQTT_PING_INTERVAL 10000  //  in milliseconds

#define STATUS_FLASH_INTERVAL   200   // interval in millis when flashing status led

// Settings object
class Settings{
  public:
    uint8_t brightnessMode = 0;  // 0 = manual, 1, automatic
    uint8_t brightnessPercent = 50;  // value of brightness in percentage
    uint8_t brightnessRaw = 16;  // brightness value on 1 - 32 scale
    uint8_t divider = 16;      // divider for colors
    uint8_t isPaired = 0;
    
    int delayBtwColumns = 50;
   
    void setBrightness(uint8_t val){
      brightnessPercent = val;
      brightnessRaw = map(val, 0, 100, 1, 31);
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
    int scount;
     
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

    void initBuffer(int len){
      clearBuffer();
      _buffer = (uint8_t**)malloc( sizeof(uint8_t*)*len );
      for(int i=0; i<len; i++){
        uint8_t* newBuf = (uint8_t*)malloc(_colLength);
        _buffer[i] = newBuf;
      }
      _noColumns = len;
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

    bool setBuffer(const char* filename){
      Serial.print("Setting buffer with file: "); Serial.println(filename);
      File file= SPIFFS.open(filename);
      if(!file) {
        Serial.println("ERROR: failed to open file.");
        return false;
      }
      Serial.println("File opened");
      clearBuffer();

      uint8_t arrt [2];
      int noCols;
      file.read(arrt, 2);
      noCols = arrt[0]<<8 | arrt[1] ;
      debugln("Buffer, No of columns: " + String(noCols));
      _buffer = (uint8_t**)malloc( sizeof(uint8_t*)*noCols );
    
      for(uint8_t it=0; it<noCols; it++)  {
        uint8_t* newCol = (uint8_t*)malloc(_colLength);   // new column array
        file.readBytes((char*)newCol, _colLength);
        _buffer[it]=newCol;
      }
      _noColumns = noCols;
      
      file.close();
      Serial.println("DataBuffer: data split complete");
      return true;
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

      digitalWrite(r, HIGH);  // Set pins high because of anode cond
      digitalWrite(g, HIGH);
      digitalWrite(b, HIGH);
    }

    void red(){
      digitalWrite(r, LOW);
      digitalWrite(g, HIGH);
      digitalWrite(b, HIGH);
    }

    void green(){
      digitalWrite(g, LOW);
      digitalWrite(r, HIGH);
      digitalWrite(b, HIGH);
    }

    void blue(){
      digitalWrite(b, LOW);
      digitalWrite(r, HIGH);
      digitalWrite(g, HIGH);
    }

    void yellow(){
      digitalWrite(r, LOW);
      digitalWrite(g, LOW);
      digitalWrite(b, HIGH);
    }

    void purple(){
      digitalWrite(r, LOW);
      digitalWrite(g, HIGH);
      digitalWrite(b, LOW);
    }
    
    void flashRed(uint8_t count) {
      off();
      while(count--){
        digitalWrite(r, LOW);
        delay(STATUS_FLASH_INTERVAL);
        digitalWrite(r, HIGH);
        delay(STATUS_FLASH_INTERVAL);
      }  
    }

    void flashGreen(uint8_t count) {
      off();
      while(count--){
        digitalWrite(g, LOW);
        delay(STATUS_FLASH_INTERVAL);
        digitalWrite(g, HIGH);
        delay(STATUS_FLASH_INTERVAL);
      }
    }

    void flashBlue(uint8_t count)  {
      off();
      while(count--){
        digitalWrite(b, LOW);
        delay(STATUS_FLASH_INTERVAL);
        digitalWrite(b, HIGH);
        delay(STATUS_FLASH_INTERVAL);
      }
    }

    void flashYellow(uint8_t count){
      off();
      while(count--){
        digitalWrite(r, LOW);
        digitalWrite(g, LOW);
        delay(STATUS_FLASH_INTERVAL);
        digitalWrite(r, HIGH);
        digitalWrite(g, HIGH);
      }
    }

    void off(){   // function to put off all eds
      digitalWrite(b, HIGH);
      digitalWrite(g, HIGH);
      digitalWrite(r, HIGH);
    }

    void onDisplaying(){
      blue();
    }

    void onTransferring(){
      yellow();
    }

    void onReceiving(){
      purple();
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
bool startImage(const char* img);
void mqttCommand(char* msg);

#endif

