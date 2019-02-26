#ifndef COMM_H
#define COMM_H

#include "myconfig.h"

extern uint8_t brightnessMode;
extern uint8_t brightnessVal;

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
