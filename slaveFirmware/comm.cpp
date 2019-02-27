#include "comm.h"
#include "driver/i2c.h"
#include "arm.h"
#include "wrapper.h"
#include <ArduinoJson.h>

#define buflen(c) sizeof(c)/sizeof(c[0])
#define sColumn   IMAGE_HEIGHT*3           // size of one column array

// definitions and variables for serial communication
HardwareSerial slave1(1);

auto mySerial = slave1;

const uint8_t msgSignal[]   = {0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A };
const uint8_t dataSignal[]  = {0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA };
const uint8_t readySignal[] = {0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC };

uint8_t **dataStore;   // array of pointers to store the data. Allocated dynamically

volatile uint16_t noColumns = 0;
uint8_t* columnBuf;         // Buffer for columns
uint8_t* tempBuf;           // Temporary Buffer for storing

extern arm arm1;
extern arm arm2;

uint8_t brightnessMode = 0;
uint8_t brightnessVal = 1;

/*
 * Initialize Communication
 */
void initComm()
{
  debug("Initializing Serial Communication" );
  mySerial.begin(myBaudRate, SERIAL_8N1, SLAVE1_RX, SLAVE1_TX);
  mySerial.setTimeout(2000);

  // allocate buffer for columns
  columnBuf = (uint8_t*)malloc( sColumn + 2); // the extra 2 bytes is for the crc
  tempBuf = (uint8_t*)malloc(200);
  //serClear(); 
}


/*
 * Function to receive data from master
 */

void commParser()
{
  if(!mySerial.available()) return ;    // return if no data is available

  waitForData(tempBuf, buflen(dataSignal) );    // read first 8 bytes which should be data signal

  /*
    Parse Config message
  */
  if( isEqual(tempBuf, (uint8_t*)msgSignal, 4 ) ){    // check first 4 members of array
    debug("OK: Received Config message");

    if(!checkCrc(tempBuf, 8))
      return;
    uint16_t len = tempBuf[4]<<8 | tempBuf[5];
    sendBuf((uint8_t*)readySignal, buflen(readySignal));

    if( !waitForData(tempBuf, len) ) {      // wait for the main data
      debug("ERROR: No length data from master."); serClear(); return;
    }
    if(!checkCrc(tempBuf, len))
      return;

    DynamicJsonBuffer  jsonBuffer(200);     // Config messages will be sent as Json
    JsonObject& root = jsonBuffer.parseObject((char*)tempBuf);
    root.prettyPrintTo(debugSerial);

    debug("COMM: Received Config message.");
    if (!root.success()) {
      debug("COMM: parseObject() failed");
      return;
    }

    if(root.containsKey("Brightness_Control_Mode")){
      brightnessMode = root["brightnessMode"];
      debug("CONFIG: Brightness Mode set.");
    }

    if(root.containsKey("Brightness_Value")){
      brightnessVal = root["Brightness_Value"];
      debug("CONFIG: Brightness Value set.");
    }
    return;
  }


  /*
    Parse Data message
  */
  if( !isEqual(tempBuf, (uint8_t*)dataSignal, 4 ) ){
    debug("ERROR: Wrong starting sequence"); serClear(); return ;
  }
  debug("OK: Received Starting Sequence.");

  if(!checkCrc(tempBuf, 8)){
    debug("ERROR: incorrect crc for startign response");
    return;
  }

  //serClear();
  uint16_t noCols = tempBuf[4]<<8 | tempBuf[5];

  uint8_t **dataStoreBckup = (uint8_t**)malloc( sizeof(uint8_t*)*noCols );  // allocating new backup buffers
  uint8_t* col = (uint8_t*)malloc( sColumn + 2);

  uint16_t count=0;   // read all the columns
  debug("INIT: Receiving data from master: " ); Serial.println(noCols);
  
  sendBuf((uint8_t*)readySignal, buflen(readySignal));
  
  while(count < noCols)
  {
    if( !waitForData(col, sColumn+2 ) ) {
      debug("ERROR: No Data received from master"); serClear();
      goto freeBackup;      // free allocated memory before exiting
      }

    if(!checkCrc(col, sColumn+2))
      goto freeBackup;      // free allocated memory before exiting

    dataStoreBckup[count] = (uint8_t*)malloc( sColumn);
    memcpy( dataStoreBckup[count], col, sColumn );

    sendBuf(col+sColumn, 2);    // reply to reply with crc of msg
    count++;
  }
  debug("OK: Complete Image data received");

  arm1.stop();          // stop both tasks to avoid memory access collision
  arm2.stop();

  if(noColumns>0){      // free old memory spaces first
    debug("INIT: Freeing pointers to main buffer");
    for(uint8_t i=0; i<noColumns; i++){
      free(dataStore[i]);
    }
    //****** NOTE: uncommented today 05-feb, might be possible error.
    //free(dataStore);
  }

  dataStore = (uint8_t**)malloc( sizeof(uint8_t*)*noCols );

  debug("INIT: Copying data from backup buffer to main buffer.");
  for(uint8_t i=0; i<noCols; i++)
  {
    dataStore[i] = (uint8_t*)malloc( sColumn);
    memcpy( dataStore[i], dataStoreBckup[i], sColumn );
  }
  debug("OK: Done copying data to main buffer");

  //free(dataStoreBckup);

  noColumns = noCols;
  debug("Data reception complete");

  arm1.setImage(dataStore);   // set image pointers
  arm2.setImage(dataStore);
  createTask(&arm1);          // Create task if not already created
  createTask(&arm2);

freeBackup:
  //for(uint8_t i=0; i< noCols; i++){
    free(dataStoreBckup);
  //}
  debug("OK: Done Freeing backup buffers");
  debug("Num Columns: "); Serial.println(noColumns);
  free(col);
}


/*
 *  read a certain number of bytes from serial
 */
bool waitForData(uint8_t* buf, uint16_t len)
{
   auto ans = mySerial.readBytes( buf, len);
   serClear();
   if(ans == len){
    dumpBuf(buf, len);
    return true;
   }
   else return false;
}

// flush incoming buffer
void serClear()
{
  while(mySerial.available())
    mySerial.read();
}

// checks if 2 arrays are equal
bool isEqual(uint8_t* buf1, uint8_t* buf2, uint16_t len)
{
  for(uint16_t i=0; i< len; i++)
  {
    if(buf1[i] != buf2[i]) return false;
  }
  return true;
}

// prints 8-bit data in hex with leading zeroes
void dumpBuf(uint8_t *data, uint16_t length)
{
   Serial.print("0x");
   for (int i=0; i<length; i++) {
     if (data[i]<0x10) {Serial.print("0");}
     Serial.print(data[i],HEX);
     Serial.print(" ");
   }
   debug(" ");
}

// send buffer
void sendBuf(uint8_t* buf, uint16_t len)
{
  for(uint16_t i=0; i< len; i++)
  {
    mySerial.write(buf[i]);
  }
  mySerial.flush();   // wait until all data is sent
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

// check if the crc of a buffer is correct
bool checkCrc(uint8_t* buf, uint16_t buflen){
  uint16_t tempCrc = (buf[buflen-2]<<8 | buf[buflen-1]);
  Serial.print("Original Crc: ");
  Serial.println(tempCrc, HEX); 

  unsigned short ncrc =  crc(buf, buflen-2);
  Serial.print("New Crc: ");
  Serial.println(ncrc, HEX); 
  
  if( ncrc!= tempCrc ){   // check crc, last 2 members of array is the crc16
    debug("ERROR: Wrong crc");
    return 0;
  }
  return 1;
}
