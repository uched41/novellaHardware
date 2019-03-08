#include "comm.h"
#include "myControl.h"

#define NO_TRIALS   3                         // number of time to attempt to send data to the slave
#define buflen(c)   sizeof(c)/sizeof(c[0])

uint8_t msgSignal[8]   = {0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A };
uint8_t dataSignal[8]  = {0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA };
uint8_t readySignal[8] = {0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC };

uint8_t* slave::columnBuf = NULL;          // Buffer for columns
uint8_t* slave::tempBuf = NULL;            // Temporary Buffer for storing
uint16_t slave::tempCrc = 0;

slave::slave(uint8_t tx, uint8_t rx, uint8_t serInstance){
   debugln("Initializing slave " + String(serInstance));
   txpin = tx;
   rxpin = rx;
   serialInstance = serInstance;

   columnBuf = (uint8_t*)malloc(sColumn+2); // the extra 2 bytes is for the crc
   tempBuf = (uint8_t*)malloc(32);

   mySerial = new HardwareSerial(serInstance);
   mySerial->begin(myBaudRate, SERIAL_8N1, rx, tx);
   while(!(*mySerial));     // wait until serial is ready
   mySerial->setTimeout(2000);
}


// Function to attempt sending data to slave a couple of times.
// This is necessary because errors can occur in serial communication
bool slave::attemptSend(){
  int tcount = 0;
  bool res = false;
  while( (tcount++<NO_TRIALS) && (!res) ){
    res = sendData();
  }
  if(res){
    debugln("OK: Data sent to slave" + String(serialInstance));
    return true;
  }
  else{
    errorHandler("ERROR: Failed to send data to slave ");
    return false;
  }
}


// Attempt to send message
bool slave::attempMsg(char* msg, int len){
  int tcount = 0;
  bool res = false;
  while( (tcount++<NO_TRIALS) && (!res) ){
    res = sendMsg( msg, len);
  }
  if(res){
    debugln("OK: Data sent to slave" + String(serialInstance));
    return true;
  }
  else{
    errorHandler("ERROR: Failed to send data to slave ");
    return false;
  }
}

// function to send a message ( not data ) to slave, e.g config message.
bool slave::sendMsg(char* msg, int len){
  // msg will be sent in json format
  debugln("COMM: Sending message to slave.");
  debugln(msg);

  // generate message to send
  msgSignal[4] = (uint8_t)((len&0xff00)>>8);
  msgSignal[5] = (uint8_t)(len&0x00ff);
  tempCrc = crc(msgSignal, 6);
  msgSignal[6] = (uint8_t)((tempCrc&0xff00)>>8);   // add crc to back of message
  msgSignal[7] = (uint8_t)(tempCrc&0x00ff);
  sendBuf((uint8_t*)msgSignal, buflen(msgSignal));  // send message
  
  // wait for slave to be ready
  if( !waitForData(tempBuf, buflen(readySignal)) ) {
    errorHandler("ERROR: Slave not responding"); return 0;
  }

  if( !isEqual(tempBuf, (uint8_t*)readySignal, buflen(readySignal) ) ) {
    errorHandler("ERROR: Wrong response");  return 0;
  }

  debugln("COMM: Slave responded.");
  debugln("COMM: Sending message.");

  uint8_t tbuf[len+2];
  memcpy(tbuf, (uint8_t*)msg, len);   // copy message into temp buffer
  tempCrc = crc((uint8_t*)msg, len);            // get crc of message
  tbuf[len] = (uint8_t)((tempCrc&0xff00)>>8);   // add crc to back of message
  tbuf[len+1] = (uint8_t)(tempCrc&0x00ff);

  sendBuf(tbuf, len+2);   // send message

  return true;
}


// Function to send image data to slave
bool slave::sendData()
{
  // first send signal to notify slave of incoming data
  debugln("COMM: Sending data to slave " + String(serialInstance));
  debugln("COMM: Sending Initialization signal");

  // generate message to send
  dataSignal[4] = (uint8_t)( ( dataStore._noColumns & 0xff00 ) >> 8 );
  dataSignal[5] = (uint8_t)( dataStore._noColumns & 0x00ff );
  tempCrc = crc(dataSignal, 6);
  dataSignal[6] = (uint8_t)((tempCrc&0xff00)>>8);   // add crc to back of message
  dataSignal[7] = (uint8_t)(tempCrc&0x00ff);
  dumpBuf((uint8_t*)dataSignal, buflen(dataSignal));
  sendBuf((uint8_t*)dataSignal, buflen(dataSignal));

  // wait for slave to be ready
  if( !waitForData(tempBuf, buflen(readySignal)) || !isEqual(tempBuf, (uint8_t*)readySignal, buflen(readySignal) ) ) {
    errorHandler("ERROR: Slave not responding or slave response error."); return 0;
    }
  debugln("COMM: Slave responded");

  // Start sending columns
  for(uint16_t it=0; it<dataStore._noColumns; it++)
  {
    memcpy(columnBuf, dataStore._buffer[it], sColumn );
    tempCrc = crc(columnBuf, sColumn);
    columnBuf[ sColumn ] = (uint8_t)( (tempCrc&0xff00)>>8);    // append crc to last 2 bytes of array
    columnBuf[ sColumn+1 ] = (uint8_t)( tempCrc&0x00ff);

    sendBuf(columnBuf, sColumn+2);    // send the message and wait for reply
    dumpBuf(columnBuf, sColumn+2);

    if( !waitForData(tempBuf, 2) ) {  // the slave will reply with the crc of last data sent
      errorHandler("ERROR: Slave did not respond"); return 0;
      }
    if( (uint16_t)((tempBuf[0]<<8) | (tempBuf[1])) != tempCrc ) {
      errorHandler("ERROR: Incorrect Crc"); return 0;
      }
  }

  debugln("COMM: SUCCESS: Complete data sent");
  return 1;
}


// Low level function to send buffer
void slave::sendBuf(uint8_t* buf, uint16_t len)
{
  for(uint16_t i=0; i< len; i++)
  {
    mySerial->write(buf[i]);
  }
  mySerial->flush();    // wait for transmission of outgoing serial data to complete
}


// function to wait for and receive data from slave
bool slave::waitForData(uint8_t* buf, uint16_t len)
{
   auto ans = mySerial->readBytes( buf, len);
   serClear();
   if(ans == len){
     dumpBuf(buf, len);
     return true;
   }
   else return false;
}

// flush incoming buffer
void slave::serClear()
{
  while(mySerial->available())
    mySerial->read();
}

// checks if 2 arrays are equal
bool isEqual(uint8_t* buf1, uint8_t* buf2, uint16_t len)
{
  for(uint16_t i=0; i< len; i++){
    if(buf1[i] != buf2[i]) return false;
  }
  return true;
}

// prints 8-bit data in hex with leading zeroes
void dumpBuf(uint8_t *data, uint16_t length)
{
   debug("0x");
   for (int i=0; i<length; i++) {
     if (data[i]<0x10) {debug("0");}
     debugSerial.print(data[i],HEX);
     debug(" ");
   }
   debugln(" ");
}

// crc function
unsigned short crc(const unsigned char* data_p, unsigned char length)
{
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}
