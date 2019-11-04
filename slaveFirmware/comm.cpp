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

ARM_DATA tempStore;         // Data storer object
Settings mySettings;                   // Initialize settings object

/*
 * Initialize Communication
 */
void initComm()
{
  debugln("Initializing Serial Communication" );
  mySerial.begin(myBaudRate, SERIAL_8N1, SLAVE1_RX, SLAVE1_TX);
  mySerial.setTimeout(2000);

  // allocate buffer for columns
  columnBuf = (uint8_t*)malloc( sColumn + 2); // the extra 2 bytes is for the crc
  tempBuf = (uint8_t*)malloc(200);
  serClear(); 
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
            mySettings.brightnessMode = 0;
          }
          else if(val == 1){
            debugln("Setting brightness mode to automatic");
            mySettings.brightnessMode = 1;
          }
          debugln("CONFIG: Brightness Mode set.");
          return;
       }

      // Command to stop display
      else if(cmd == "Stop_Display"){
        debugln("MQTT: Stopping display");
        arm1.stop();
      }

      // set brightness value
      else if(cmd == "Brightness"){
        int val = root["value"];
        debugln("MQTT: Setting brightness value: "); debugln(val);
        mySettings.setBrightness(val);
        debugln("CONFIG: Brightness Value set.");
        return;
      }

      // set column delay
      else if(cmd == "Column_Delay"){
        int val = root["value"];
        debugln("MQTT: Setting delay between columns: "); debugln(val);
        mySettings.delayBtwColumns = val;
        return;
      }

      // set move delay
      else if(cmd == "Move_Image"){
        int val = root["value"];
        debugln("MQTT: Setting move delay: "); debugln(val);
        mySettings.moveDelay = val;
        return;
      }

      // set move delay
      else if(cmd == "Slave_Delay"){
        int val = root["value"];
        debugln("MQTT: Setting slave delay: "); debugln(val);
        mySettings.specialDelay = val;
        return;
      }

      // set  delay
      else if(cmd == "Delay0s"){
        int val = root["value"];
        debugln("MQTT: Setting 0 degrees delay: "); debugln(val);
        mySettings.delay0s = val;
        return;
      }

      // set  delay
      else if(cmd == "Delay180s"){
        int val = root["value"];
        debugln("MQTT: Setting 180 degrees delay: "); debugln(val);
        mySettings.delay180s = val;
        return;
      }
      
      // set gif speed
      else if(cmd == "Gif_Speed"){
        int val = root["value"];
        debugln("MQTT: Setting gif speed: "); debugln(val);
        mySettings.gifRepeat = val;
        return;
      }
      
      // set column delay
      else if(cmd == "Divider"){
        debugln("Setting divy");
        int val = root["value"];
        debugln("MQTT: Setting color divider: "); debugln(val);
        mySettings.divider = val;
        return;
      }

      else if(cmd == "Sync"){
        disableIsr();
        while(!digitalRead(SYNC_PIN));  // wait for sync pin
        arm1._imgPointer = 0;
        enableIsr();   // Start interrupts
      }

      else if(cmd == "Disable_S"){
        disableIsr();
      }
  
      else if(cmd == "Enable_S"){
        enableIsr();
      }
    
      else if(cmd == "Saved_Data"){
        debugln("MQTT: Receiving saved settings");
        mySettings.delayBtwColumns = root["Delay_Columns"];
        mySettings.setBrightness(root["Brightness"]);
        mySettings.brightnessMode = root["Brightness_Mode"];
        mySettings.divider = root["Divider"];
        mySettings.moveDelay = root["Move_Delay"];
        mySettings.specialDelay = root["Slave_Delay"];
        mySettings.delay0s = root["Delay0s"];   // edited by leo
        mySettings.delay180s = root["Delay180s"];     // edited by leo
        //enableIsr();   // Start interrupts
     }
    
    }
        
    return;
  }


  /*
    Parse Data message
  */
  else{
    if( !isEqual(tempBuf, (uint8_t*)dataSignal, 2 ) ){
      debugln("ERROR: Wrong starting sequence"); 
      serClear(); 
      ESP.restart(); 
      return ;
    }
    debug("OK: Received Starting Sequence.");
  
    if(!checkCrc(tempBuf, 8)){
      debugln("ERROR: incorrect crc for startign response");
      ESP.restart();             
      return;
    }
  
    uint16_t noImgs = tempBuf[2]<<8 | tempBuf[3];
    uint16_t noCols = tempBuf[4]<<8 | tempBuf[5];
  
    ARM_DATA bckUpStore;
    bckUpStore.initGif(noImgs, sColumn, noCols);
    
    uint8_t* col = (uint8_t*)malloc( sColumn + 2);
  
    uint16_t imgCount = 0;
    uint16_t colCount = 0;   // read all the columns
    debugln("INIT: Receiving data from master: " );
    debugln("Number of Images: " + String(bckUpStore._noImages));
    debugln("NUmber of columns per image: " + String(bckUpStore._noColumnsPerImage));
    
    sendBuf((uint8_t*)readySignal, buflen(readySignal));
    if(arm1.isTaskCreated()){
      arm1.stop();          // resume task if already created
    }
  
    disableIsr();   // Stop interrupts
    while(imgCount < bckUpStore._noImages){
      while(colCount < bckUpStore._noColumnsPerImage){
        if( !waitForData(col, sColumn+2 ) ) {
          debugln("ERROR: No Data received from master"); serClear();
          ESP.restart();
          goto freeBackup;      // free allocated memory before exiting
        }
        
        if(!checkCrc(col, sColumn+2)){
          ESP.restart();
          goto freeBackup;      // free allocated memory before exiting
        }
         
        memcpy( bckUpStore._frames[imgCount]->_data[colCount], col, sColumn );
        //dumpBuf(col, sColumn);
        sendBuf(col+sColumn, 2);    // reply to reply with crc of msg
        colCount++;
      }
      colCount = 0;
      debug('.');
      imgCount++;
      vTaskDelay(5/ portTICK_PERIOD_MS);
    }
    
    debugln("");
    debugln("OK: Complete Image data received");
    
    tempStore.initGif(noImgs, sColumn, noCols);
    debugln("INIT: Copying data from backup buffer to main buffer.");
    for(int i=0; i<noImgs; i++){
      for(int j=0; j<noCols; j++){
        memcpy(tempStore._frames[i]->_data[j], bckUpStore._frames[i]->_data[j], sColumn);
      }
    }
    debugln("OK: Done copying data to main buffer");
  
    //delay(3000);
    setArmData(&tempStore);
    resetArms();
    
  freeBackup:
    if(arm1.isTaskCreated()){
        arm1.resume();          // resume task if already created
    }
    else{
       createTask(&arm1);          // Create task if not already created
    }  
  
    while(!digitalRead(SYNC_PIN));  // wait for sync pin
    //while(digitalRead(SYNC_PIN));
    
    enableIsr();   // Start interrupts
   
    bckUpStore.clearGif();
    debugln("OK: Done Freeing backup buffers");
    free(col);
    serClear();
  }

}


/*
 *  read a certain number of bytes from serial
 */
bool waitForData(uint8_t* buf, uint16_t len)
{
   auto ans = mySerial.readBytes( buf, len);
   serClear();
   if(ans == len){
    //dumpBuf(buf, len);
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
  mySerial.write(buf, len);
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
  unsigned short ncrc =  crc(buf, buflen-2);
  
  if( ncrc!= tempCrc ){   // check crc, last 2 members of array is the crc16
    debug("ERROR: Wrong crc");
    return 0;
  }
  return 1;
}
