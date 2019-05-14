#ifndef COMM_H
#define COMM_H

#include "FS.h"
#include "SPIFFS.h"
#include "myconfig.h"

//#define ps_malloc malloc

// Settings object
class Settings{
  public:
    uint8_t brightnessMode = 0;  // 0 = manual, 1, automatic
    uint8_t brightnessPercent = 50;  // value of brightness in percentage
    uint8_t brightnessRaw = 16;  // brightness value on 1 - 32 scale
    int divider = 16;      // divider for colors
    uint8_t isPaired = 0;
    
    int delayBtwColumns = 50;
    int moveDelay = 0;
    
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
    int scount;
     
    DataBuffer(int columnLength){
      _buffer = NULL;
      _noColumns = 0;
      _colLength = columnLength;
    }
    ~DataBuffer(){
      clearBuffer();
    }
    
    void clearBuffer(){     
      Serial.println("DataBuffer: clearing buffer");
      for(int i=0; i< _noColumns; i++){
        free(_buffer[i]);
      }
      _noColumns = 0;
    }

    bool initBuffer(int len){
      clearBuffer();
      _buffer = (uint8_t**)ps_malloc( sizeof(uint8_t*)*len );
      for(int i=0; i<len; i++){
        uint8_t* newBuf = (uint8_t*)ps_malloc(_colLength);
        if(newBuf == NULL){
          debugln("Mem allocation failed");
          clearBuffer();
          return 0;
        }
        _buffer[i] = newBuf;
      }
      _noColumns = len;
      debugln("Buffer Initialized");
      return 1;
    }
    
    void setBuffer(uint8_t** buf, int len){
      clearBuffer();
      _buffer = (uint8_t**)ps_malloc( sizeof(uint8_t*)*len );
      for(int i=0; i<len; i++){
        uint8_t* newBuf = (uint8_t*)ps_malloc(_colLength);
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
      _buffer = (uint8_t**)ps_malloc( sizeof(uint8_t*)*noCols );
    
      for(uint8_t it=0; it<noCols; it++)  {
        uint8_t* newCol = (uint8_t*)ps_malloc(_colLength);   // new column array
        file.readBytes((char*)newCol, _colLength);
        _buffer[it]=newCol;
      }
      _noColumns = noCols;
      
      file.close();
      Serial.println("DataBuffer: data split complete");
      return true;
    }
};

//extern DataBuffer dataStore;


/*
 * Image object
 */
struct Img{
  uint8_t** _data;
  int _noColumns;
  int _columnLength;

  Img(int columnLength) :  _data(NULL), _noColumns(0), _columnLength(columnLength) {};
  ~Img(){
    clearBuffer();
  }
  
  bool initBuffer(int len){
    clearBuffer();
    //debugln("Initializing Image buffer");
    _data = (uint8_t**)ps_malloc( sizeof(uint8_t*)*len );
    for(int i=0; i<len; i++){
      uint8_t* newBuf = (uint8_t*)ps_malloc(_columnLength);
      if(newBuf == NULL){
        debugln("Mem allocation failed");
        clearBuffer();
        return 0;
      }
      _data[i] = newBuf;
    }
    _noColumns = len;
    return 1;
  }
    
  void clearBuffer(){     
    //Serial.println("IMG: clearing buffer");
    for(int i=0; i<_noColumns; i++){
      free(_data[i]);
    }
    _noColumns = 0;
  }

  void setBuffer(uint8_t** buf, int noCols){
    clearBuffer();
    _data = (uint8_t**)ps_malloc( sizeof(uint8_t*)*noCols );
    for(int i=0; i<noCols; i++){
      uint8_t* newBuf = (uint8_t*)ps_malloc(_columnLength);
      memcpy(newBuf, buf[i], _columnLength);
      _data[i] = newBuf;
    }
    _noColumns = noCols;
    Serial.println("DataBuffer: Done Setting buffer");
  }
  
};


struct Gif{
  Img** _frames;
  int _noImages;
  int _noColumnsPerImage;

  Gif(): _frames(NULL), _noImages(0), _noColumnsPerImage(0) {};  // constructor
  ~Gif(){           // destructor
    clearGif();
  }
  
  bool initGif(int noImages, int colLength, int noColumns){
    _frames = (Img**)ps_malloc(sizeof(Img) * noImages);
    for(int i=0; i<noImages; i++){
      _frames[i] = new Img(colLength);
      if(!_frames[i]->initBuffer(noColumns)){
        return 0;
      }
    }
     _noImages = noImages;
     _noColumnsPerImage = noColumns;
    return 1;
  }

  void clearGif(){
    for(int i=0; i<_noImages; i++){
      delete _frames[i];
    }
    _noImages = 0;
    _noColumnsPerImage = 0;
  }

  bool readFromFile(const char* filename){
    debug("Setting buffer with file: "); debugln(filename);
    File file= SPIFFS.open(filename);
    if(!file) {
      Serial.println("ERROR: failed to open file.");
      return false;
    }
    Serial.println("File opened");
    clearGif();

    uint8_t arrt [10];    // 10 is choosen to remain future proof
    file.read(arrt, 10);
    int noCols;
    int noImages;

    noImages = arrt[0]<<8 | arrt[1];
    noCols = arrt[2]<<8 | arrt[3] ;
    
    debugln("Buffer, No of columns per image: " + String(noCols) + ", No of images: " + String(noImages));
    if(!initGif(noImages, sColumn, noCols)){
      return 0;
    }

    for(int i=0; i<noImages; i++){   // Iterate through images
      for(int j=0; j<noCols; j++){   // Iterate through columns
        file.readBytes((char*)_frames[i]->_data[j], sColumn);
        //dumpBuf(_frames[i]->_data[j], sColumn);
      }
    }

    file.close();
    Serial.println("DataBuffer: data split complete");
    return true;
  }
  
};

typedef Gif ARM_DATA;   // Name to be used when iniitalizing ImageData, to avoid confusion
extern ARM_DATA tempStore;


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
