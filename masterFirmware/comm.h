#ifndef COMM_H
#define COMM_H
#include "FS.h"
#include "SPIFFS.h"
#include "myconfig.h"

class slave{
  uint8_t txpin;
  uint8_t rxpin;
  uint8_t serialInstance;
  bool isready;
  HardwareSerial* mySerial;
  bool sendData();

  public:
    static uint8_t* columnBuf;          // Buffer for columns
    static uint8_t* tempBuf;            // Temporary Buffer for storing
    static uint16_t tempCrc;
    static File file;

    slave(uint8_t tx, uint8_t rx, uint8_t serInstance);
    bool sendMsg(char* msg, int len);
    bool attemptSend();
    void sendBuf(uint8_t* buf, uint16_t len);
    void serClear();
    bool waitForData(uint8_t* buf, uint16_t len);
    bool attempMsg(char* msg, int len);
};

// Declare slave
extern slave slave1;


/*
 * Miscellenous funtions
 */
bool isEqual(uint8_t* buf1, uint8_t* buf2, uint16_t len);
void dumpBuf(uint8_t *data, uint16_t length) ;
unsigned short crc(const unsigned char* data_p, unsigned char length);


#endif
