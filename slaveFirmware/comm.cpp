#include "comm.h"
#include "driver/i2c.h"
#include "arm.h"
#include "wrapper.h"
#include <ArduinoJson.h>

#define buflen(c) sizeof(c)/sizeof(c[0])

// definitions and variables for serial communication
HardwareSerial slave1(1);

auto mySerial = slave1;

const uint8_t msgSignal[]   = {0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A };
const uint8_t dataSignal[]  = {0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA };
const uint8_t readySignal[] = {0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC, 0xEC };

uint8_t* columnBuf;         // Buffer for columns
uint8_t* tempBuf;           // Temporary Buffer for storing

uint8_t brightnessMode = 0;
uint8_t brightnessVal = 1;

DataBuffer dataStore(sColumn);         // Data storer object
Settings mySettings;                   // Initialize settings object

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
    debugln("OK: Received Config message");

    if(!checkCrc(tempBuf, 8))
      return;
    uint16_t len = tempBuf[4]<<8 | tempBuf[5];
    sendBuf((uint8_t*)readySignal, buflen(readySignal));

    if( !waitForData(tempBuf, len+2) ) {      // wait for the main data
      debugln("ERROR: No length data from master."); serClear(); return;
    }
    if(!checkCrc(tempBuf, len+2))
      return;

    char* b2 [len];
    memcpy(b2, tempBuf, len);
    Serial.println((char*)b2);
    DynamicJsonBuffer  jsonBuffer(200);     // Config messages will be sent as Json
    JsonObject& root = jsonBuffer.parseObject((char*)b2);
    root.prettyPrintTo(debugSerial);

    debugln("COMM: Received Config message.");
    if (!root.success()) {
      debugln("COMM: parseObject() failed");
      return;
    }
    else debugln("COMM: Json parsing success");
    
    if (root.containsKey("command")){
      String cmd = root["command"];

        // set brightness mode 
        if(cmd == "Brightness_Mode"){
          String temp = root["value"];
          int val = temp.toInt();
  
          if(val == 0){
            debugln("Setting brightness mode to manual");
            mySettings.brighnessMode = 0;
          }
          else if(val == 1){
            debugln("Setting brightness mode to automatic");
            mySettings.brighnessMode = 1;
          }
          debugln("CONFIG: Brightness Mode set.");
          return;
       }

      // set brightness value
      else if(cmd == "Brightness"){
        String temp = root["value"];
        int val = temp.toInt();
        debugln("MQTT: Setting brightness value: "); debugln(val);
        mySettings.setBrightness(val);
        debugln("CONFIG: Brightness Value set.");
        return;
      }

      // set column delay
      else if(cmd == "Column_Delay"){
        String temp = root["value"];
        int val = temp.toInt();
        debugln("MQTT: Setting delay between columns: "); debugln(val);
        mySettings.delayBtwColumns = val;
        return;
      }

      
      else if(cmd == "Saved_Data"){
        debugln("MQTT: Receiving saved settings");
        mySettings.delayBtwColumns = root["Delay_Columns"];
        mySettings.setBrightness(root["Brightness"]);
        mySettings.brighnessMode = root["Brightness_Mode"];
     }
    
    }
        
    return;
  }


  /*
    Parse Data message
  */
  if( !isEqual(tempBuf, (uint8_t*)dataSignal, 4 ) ){
    debugln("ERROR: Wrong starting sequence"); 
    serClear(); return ;
  }
  debug("OK: Received Starting Sequence.");

  if(!checkCrc(tempBuf, 8)){
    debugln("ERROR: incorrect crc for startign response");
    return;
  }

  //serClear();
  uint16_t noCols = tempBuf[4]<<8 | tempBuf[5];

  DataBuffer dataStoreBckup(sColumn);
  dataStoreBckup._noColumns = noCols;
  
  dataStoreBckup._buffer = (uint8_t**)malloc( sizeof(uint8_t*)*noCols );  // allocating new backup buffers
  uint8_t* col = (uint8_t*)malloc( sColumn + 2);

  uint16_t count=0;   // read all the columns
  debugln("INIT: Receiving data from master: " );
  
  sendBuf((uint8_t*)readySignal, buflen(readySignal));
  
  while(count < noCols)
  {
    if( !waitForData(col, sColumn+2 ) ) {
      debugln("ERROR: No Data received from master"); serClear();
      goto freeBackup;      // free allocated memory before exiting
      }

    if(!checkCrc(col, sColumn+2))
      goto freeBackup;      // free allocated memory before exiting

    dataStoreBckup._buffer[count] = (uint8_t*)malloc( sColumn);
    memcpy( dataStoreBckup._buffer[count], col, sColumn );

    sendBuf(col+sColumn, 2);    // reply to reply with crc of msg
    count++;
  }
  debugln("OK: Complete Image data received");

  debugln("INIT: Copying data from backup buffer to main buffer.");
  dataStore.setBuffer( dataStoreBckup._buffer, noCols);
  debugln("OK: Done copying data to main buffer");

  setArmData(dataStore._buffer, dataStore._noColumns);
  
  if(arm1.isTaskCreated() && arm2.isTaskCreated()){
    arm1.resume();          // resume task if already created
    arm2.resume();
  }
  else{
    createTask(&arm1);          // Create task if not already created
    createTask(&arm2);
  }  

freeBackup:
  for(uint8_t i=0; i< count; i++){    // Free only the buffers that have been assigned
    free(dataStoreBckup._buffer[i]);
  }
  debugln("OK: Done Freeing backup buffers");
  debug("Num Columns: "); Serial.println(dataStore._noColumns);
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
   debugln(" ");
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
