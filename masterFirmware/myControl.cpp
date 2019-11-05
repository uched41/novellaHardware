#include <WiFi.h>              // Built-in
#include "myconfig.h"
#include "myControl.h"
#include <ESPmDNS.h>
//#include <HTTPClient.h>

#define MQTT_MAX_PACKET_SIZE 2048
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include "firstTime.h"
#include "comm.h"
#include "arm.h"
#include "wrapper.h"

long lastPingTime = 0;

WiFiClient espClientm;
PubSubClient mqttclient(espClientm);
const char* mqtt_server = "povlamp";
const char* mqtt_output_topic = "novella/devices/lampshade/response";
const char* mqtt_ping_topic   = "novella/devices/lampshade/ping/";
const char* mqtt_command_topic   = "novella/devices/lampshade/command/";

char mqtt_input_topic[50];
char* specialReply;
String mfname;
char deviceID[18];
String devID = " ";
char* pingMsg;
int len=0;
int FILE_BUF_SIZE = 1024;    // must correlate with mqtt buf size
const char* tempfile = "/temp.bin";

File mqttTempFile;

//DataBuffer dataStore(sColumn);          // Data storer object
//DataBuffer mqttStore(FILE_BUF_SIZE);    // This uses a bigger column size because it is used to store data received from mqtt temporarily before it is saved to a file.
ARM_DATA  tempStore;                 // Stores data in the same structure that it is displayed with
Settings mySettings;                    // Initialize settings object
  
// function to initiialize mqtt
void mqttInit(void){
  /* setup MDNS for ESP32 */
  if (!MDNS.begin("esp32")) {
      Serial.println("Error setting up MDNS responder!");
      while(1) {
          delay(1000);
      }
  }
  IPAddress serverIp = MDNS.queryHost(mqtt_server, 5000);
  Serial.print("IP address of server: ");
  Serial.println(serverIp.toString());
  MDNS.end();
  
  debugln("MQTT: Initilializing mqtt");
  mqttclient.setServer(serverIp, 1883);
  mqttclient.setCallback(mqttCallback);

  uint64_t chipid = ESP.getEfuseMac();
  sprintf(deviceID, "S%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
  devID = String(deviceID);
  String tem = String(MQTT_BASE_TOPIC) + "/" + devID + "/#";
  
  tem.toCharArray(mqtt_input_topic, tem.length()+1);
  debug("MQTT Topics: "); debugln(mqtt_input_topic);

  // Generate ping message to save overhead
  DynamicJsonBuffer  jsonBuffer(200);
  JsonObject& root = jsonBuffer.createObject();
  root["id"] = devID;
  root["type"] = "lampshade";
  pingMsg = (char*)malloc(root.measureLength()+1);
  root.printTo(pingMsg, root.measureLength()+1);
  debug("Ping Message: "); debugln(pingMsg);

  // Generate speical OK Reply
  DynamicJsonBuffer  jsonBuffer1(300);
  JsonObject& root1 = jsonBuffer1.createObject();
  root1["id"] = devID;
  root1["response"] = String("OK");
  specialReply = (char*)malloc(root1.measureLength()+1);
  root1.printTo(specialReply, root1.measureLength()+1);
  
  debug("MQTT Topics: "); debugln(mqtt_input_topic);
  mqttReconnect();
  mqttCommand("Starting");
}


// Call back function to process received data
void IRAM_ATTR mqttCallback(char* topic, byte* payload, unsigned int length) {
  bool sendToSlave = false;

  char* sBuf[length];
  memcpy(sBuf, payload, length);

  String tstring = String(topic);
  DynamicJsonBuffer  jsonBuffer(200);     // Config messages will be sent as Json
  JsonObject& root = (tstring.indexOf("mid") == -1) ? jsonBuffer.parseObject(payload) : jsonBuffer.createObject();

  // Receiving file
  // We save directly to a tempfile, then if the data transfer was successful we rename the file
  if(tstring.indexOf("image") > -1){    // Check if the message is a file ( Picture )
    
    if(tstring.indexOf("start") > -1){  // check is this is a start message
      disableIsr();   // Stop interrupts to stop display
      statusLed.onReceiving();
      const char* filen = root["filename"];
      const char* mUrl  = root["url"];
      mfname = String(filen);
      
      if(SPIFFS.exists(tempfile)){             // Delete temp file if it already exists
        debugln("MQTT: Previous temp file found, deleting...");
        SPIFFS.remove(tempfile);
      }

      bool ans = http_download(tempfile, mUrl);
      if(!ans){
        mqttReply("Error");
        return;
      }
      
      if(SPIFFS.exists(filen)){             // Delete file if it already exists
        debugln("MQTT: Previous file found, deleting...");
        SPIFFS.remove(filen);
      }

      debug("\nMQTT: File data received: "); debugln(filen);
      if( SPIFFS.rename(tempfile, filen ) ){
        debugln("MQTT: File received successfully.");
        mqttReply("OK");
        statusLed.flashGreen(4);
      }else{
        debugln("MQTT: Rename failed");
      }
      enableIsr();
      resync();
    }
    return ;
    
  }
  
  // Receiving command
  else if(root.containsKey("command")){  
    debug("MQTT: Message arrived [");  debug(topic); debugln("] ");
    printArray((char*)payload, length); 
    String cmd = root["command"];
    
    // Command to start displaying file
    if(cmd == "Start_Display"){
      const char* dispFile = root["file"];
      if(!root.containsKey("file")){
        mqttReply("No File Specified"); 
        return;
      }
      bool ans = startImage(dispFile);
      mqttReply("Success");

      debugln("Sending latest settings to slave ...");
      DynamicJsonBuffer  jsonBuffer(300);
      JsonObject& root = jsonBuffer.createObject();
      root["command"] = "Saved_Data";
      root["Delay_Columns"] = mySettings.delayBtwColumns;
      root["Brightness"] = mySettings.brightnessPercent;
      root["Brightness_Mode"] = mySettings.brightnessMode;
      root["Divider"] = mySettings.divider;
      root["Move_Delay"] = mySettings.moveDelay;
      root["Slave_Delay"] = mySettings.slaveDelay;
      root["Delay0s"] = mySettings.delay0s;
      root["Delay180s"] = mySettings.delay180s;
      char* temp1;
      int len = root.measureLength()+1;
      temp1 = (char*)malloc(len);
      root.printTo(temp1, len);
      
      slave1.attempMsg((char*)temp1, len);
    }

     // Command to Delete bin file
    else if(cmd == "Delete_Bin"){
      const char* dFile = root["file"];
      if(SPIFFS.exists(dFile)){             // Delete file if it already exists
        debugln("MQTT: File found, deleting...");
        SPIFFS.remove(dFile);
      }
      else debugln("MQTT: File to be deleted not found");
      
      mqttReply("OK");
      return;
    }

    // Command to stop display
    else if(cmd == "Stop_Display"){
      arm1.stop();
      //arm2.stop();
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to restart
    else if(cmd == "Reset"){
      ESP.restart();
      mqttReply("OK");
    }

    // Command to get files in memory 
    else if(cmd == "Get_Files"){
      disableIsr();
      File rootfs = SPIFFS.open("/");
      File fi = rootfs.openNextFile();
      String files = "";
      while(fi){
        if(!fi.isDirectory()){
          debugln(fi.name());
          files += fi.name();
          files += ",";
        }
        fi = rootfs.openNextFile();
      }
      char* tem3;
      tem3 = (char*)malloc(files.length()+1);
      files.toCharArray(tem3, files.length()+1);
      mqttReply(tem3);
      free(tem3);

      resync();
    }

    // Command to set brightness mode
    else if(cmd == "Brightness_Mode"){
      String temp = root["value"];
      debugln("MQTT: Setting brightness mode");
      int val = temp.toInt();

      if(val == 0){
        debugln("Setting brightness mode to manual");
        mySettings.brightnessMode = 0;
      }
      else if(val == 1){
        debugln("Setting brightness mode to automatic");
        mySettings.brightnessMode = 1;
      }
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to set brightness value
    else if(cmd == "Brightness"){
      String temp = root["value"];
      int val = temp.toInt();
      debug("MQTT: Setting brightness value: "); debugln(val);
      mySettings.setBrightness(val);
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to delay between column
    else if(cmd == "Column_Delay"){
      int val = root["value"];
      debug("MQTT: Setting delay between columns: "); debugln(val);
      mySettings.delayBtwColumns = val;
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to set move delay
    else if(cmd == "Move_Image"){
      int val = root["value"];
      debug("MQTT: Setting move delay: "); debugln(val);
      mySettings.moveDelay = val;
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to set 0degrees delay
    else if(cmd == "Delay0m"){
      int val = root["value"];
      debug("MQTT: Setting delay at 0 degrees: "); debugln(val);
      mySettings.delay0m = val;
      mqttReply("OK");
    }

    // Command to set 0degrees delay
    else if(cmd == "Delay180m"){
      int val = root["value"];
      debug("MQTT: Setting delay at 180 degrees: "); debugln(val);
      mySettings.delay180m = val;
      mqttReply("OK");
    }

    // Command to set 0degrees delay
    else if(cmd == "Delay0s"){
      int val = root["value"];
      debug("MQTT: Setting delay at 0 degrees: "); debugln(val);
      mySettings.delay0s = val;
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to set 0degrees delay
    else if(cmd == "Delay180s"){
      int val = root["value"];
      debug("MQTT: Setting delay at 180 degrees: "); debugln(val);
      mySettings.delay180s = val;
      mqttReply("OK");
      sendToSlave = true;
    }
    
    // Command to set move delay
    else if(cmd == "Master_Delay"){
      int val = root["value"];
      debug("MQTT: Setting master delay: "); debugln(val);
      mySettings.specialDelay = val;
      mqttReply("OK");
      sendToSlave = false;
    }

    // Command to set move delay
    else if(cmd == "Slave_Delay"){
      int val = root["value"];
      mySettings.slaveDelay = val;
      debug("MQTT: Setting slave delay: "); debugln(val);
      mqttReply("OK");
      sendToSlave = true;
    }

    // Command to gif speed
    else if(cmd == "Gif_Speed"){
      int val = root["value"];
      debug("MQTT: Setting gif speed: "); debugln(val);
      mySettings.gifRepeat = val;
      mqttReply("OK");
      sendToSlave = true;
    }
    
    // Command to set divider
    else if(cmd == "Divider"){
      int val = root["value"];
      debug("MQTT: Setting color divider: "); debugln(val);
      mySettings.divider = val;
      mqttReply("OK");
      sendToSlave = true;
    }

    else if(cmd == "Sync"){
      stopSlave();
      disableIsr();
      slave1.attempMsg((char*)sBuf, length); 
      delay(1000);
      arm1._imgPointer = 0;
      startSlave();
      enableIsr();
    }

    else if(cmd == "Disable_M"){
      disableIsr();
    }

    else if(cmd == "Disable_S"){
      sendToSlave = true;
    }

    else if(cmd == "Enable_M"){
      enableIsr();
    }

    else if(cmd == "Enable_S"){
      sendToSlave = true;
    }
    
    else if(cmd == "Saved_Data"){
      debugln("MQTT: Receiving saved settings");
      
      mySettings.delayBtwColumns = root["Delay_Columns"];
      mySettings.setBrightness(root["Brightness"]);
      mySettings.brightnessMode = root["Brightness_Mode"];
      mySettings.divider = root["Divider"];
      mySettings.moveDelay = root["Move_Delay"];
      mySettings.specialDelay = root["Master_Delay"];
      mySettings.slaveDelay = root["Slave_Delay"];
      mySettings.delay0m = root["Delay0m"];
      mySettings.delay180m = root["Delay180m"];
      mySettings.delay0s = root["Delay0s"];
      mySettings.delay180s = root["Delay180s"];
      mySettings.isPaired = 1;
      statusLed.off();
      
      /*if (!slave1.attempMsg((char*)sBuf, length)){
        debugln("message send failed");
        return;
      }
      debugln("config message sent to slave");
      sendToSlave = true;
      delay(500);*/
      
      const char* img = root["Image"];    // Start image first
      bool ans = startImage(img);
      
      if(ans){
        mqttReply("Success");
      }

      debugln("Sending latest settings to slave ...");
      DynamicJsonBuffer  jsonBuffer(300);
      JsonObject& root = jsonBuffer.createObject();
      root["command"] = "Saved_Data";
      root["Delay_Columns"] = mySettings.delayBtwColumns;
      root["Brightness"] = mySettings.brightnessPercent;
      root["Brightness_Mode"] = mySettings.brightnessMode;
      root["Divider"] = mySettings.divider;
      root["Move_Delay"] = mySettings.moveDelay;
      root["Slave_Delay"] = mySettings.slaveDelay;
      root["Delay0s"] = mySettings.delay0s;
      root["Delay180s"] = mySettings.delay180s;
      char* temp1;
      int len = root.measureLength()+1;
      temp1 = (char*)malloc(len);
      root.printTo(temp1, len);
      
      slave1.attempMsg((char*)temp1, len);
    }

    
    // forward message to slave if needed
    if(sendToSlave){
      slave1.attempMsg((char*)sBuf, length);
    }

    statusLed.success();
    return ;
  }
  
}

// Function to reconnect to mqtt
void mqttReconnect() {
  int attempt = 0;
  while (!mqttclient.connected() && attempt++ < 2 ) {         // Loop until we're reconnected
    debugln("MQTT: Attempting MQTT connection...");
    if (mqttclient.connect(deviceID)) {     // Attempt to connect
      debugln("MQTT: connected");
      mqttclient.subscribe(mqtt_input_topic);
      statusLed.flashGreen(4);
      return;
    } else {
      debug("failed, rc=");
      debug(mqttclient.state());
      errorHandler(" try again in 2 seconds");
      delay(1000);      // Wait 5 seconds before retrying
    }
  }

  ESP.restart();     //  Restart if we are unable to connect to MQTT
  delay(1000);
}

// mqtt loop function
void mqttLoop() {
  if (!mqttclient.connected()) {
    mqttReconnect();
  }
  mqttclient.loop();

  if(millis() - lastPingTime > MQTT_PING_INTERVAL){ // ping at regular intervals
    mqttPing();
    lastPingTime = millis();
  }
}

// mqtt server ping
void mqttPing(){
   mqttclient.publish(mqtt_ping_topic, pingMsg);  
   statusLed.flashGreen(1);
   //debugln("pinging");
}

// mqtt Reply
IRAM_ATTR void mqttReply(char* msg){
  DynamicJsonBuffer  jsonBuffer(800);
  JsonObject& root = jsonBuffer.createObject();
  root["id"] = devID;
  root["response"] = String(msg);
  char* temp1;
  temp1 = (char*)malloc(root.measureLength()+1);
  root.printTo(temp1, root.measureLength()+1);
  //debug("MQTT Output: "); debugln(temp1);
  
  mqttclient.publish(mqtt_output_topic, temp1);
  free(temp1);
}

// mqtt special ok reply
inline void mqttSpecialReply(void){
  mqttclient.publish(mqtt_output_topic, specialReply);
}

// mqtt command
void mqttCommand(char* msg){
  DynamicJsonBuffer  jsonBuffer(300);
  JsonObject& root = jsonBuffer.createObject();
  root["id"] = devID;
  root["type"] = "lampshade";
  root["command"] = String(msg);
  char* temp1;
  temp1 = (char*)malloc(root.measureLength()+1);
  root.printTo(temp1, root.measureLength()+1);
  debug("MQTT Command: "); debugln(temp1);
  
  mqttclient.publish(mqtt_command_topic, temp1);
  free(temp1);
}

// Function to start displaying image
bool startImage(const char* img){
  disableIsr();   // Stop interrupts
  debugln("Starting Image, " + String(img));
  stopSlave();
  
  if(!tempStore.readFromFile(img)) {    // read into temp buffer earlier to save time
    debugln("Failed to set buffer");
    enableIsr();   // Start interrupts
    statusLed.failed();
    return false;
  }
  
  arm1.stop();
  statusLed.onTransferring();
  setArmData(img);    // Set arm data in the beginning
  
  // Send to slave first
  if(!slave1.attemptSend()){   // send to slave1
    debugln("MQTT: Unable to send to slave");
    mqttReply("Error");
    enableIsr();   // Start interrupts
    statusLed.failed();
    return 0;
  }
  tempStore.clearGif();
  
  // Start display on master if successfully sent to slave
  resetArms();
  
 if(arm1.isTaskCreated()){
    arm1.resume();          // resume task if already created
  }
  else{
    createTask(&arm1);          // Create task if not already created
  }  

  startSlave();   // trigger slave
  enableIsr();   // Start interrupts
  //mySettings.delayBtwColumns = 0;
  //mySettings.moveDelay = 0;
    
  debugln("Image display started");
  statusLed.success();
  statusLed.onDisplaying();

  resync();
  
  return 1;
}

void startSlave(void){
  //debugln("Activating Sync pin");
  digitalWrite(SYNC_PIN, HIGH);
}

void stopSlave(void){
  digitalWrite(SYNC_PIN, LOW);
}

void resync(void){
  stopSlave();
  disableIsr();

  // Generate message for slave
  DynamicJsonBuffer  jsonBuffer(800);
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "Sync";
  char* temp1;
  int len1 = root.measureLength()+1;
  temp1 = (char*)malloc(len1);
  root.printTo(temp1, len1);
  
  slave1.attempMsg((char*)temp1, len1); 
  
  delay(1000);
  arm1._imgPointer = 0;
  startSlave();
  enableIsr();

  free(temp1);
}


// http download
bool http_download(const char* filename, const char* url){
 /* HTTPClient http;
  debug("Starting http download for file: ");
  debugln(filename);
  
  // configure server and url
  http.begin(url);
  int httpCode = http.GET();
  if(httpCode > 0) {
    debug("[HTTP] GET... code: "); debugln(httpCode);
    if(httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        uint8_t buff[128] = { 0 };

        // get tcp stream
        WiFiClient * stream = http.getStreamPtr();
        File mfile = SPIFFS.open(filename, FILE_WRITE);
        
        // read all data from server
        while(http.connected() && (len > 0 || len == -1)) {
            size_t size = stream->available();

            if(size) {
                // read up to 128 byte
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                mfile.write(buff, c);
                printArray((char*)buff, c);

                if(len > 0) {
                    len -= c;
                }
            }
            delay(1);
        }
        mfile.close();
        debugln();
        debugln("[HTTP] connection closed or file end.\n");
        http.end();
        return true;
      }
  } 
  else {
      debugln("[HTTP] GET... failed, error: %s\n");
      debugln(http.errorToString(httpCode).c_str());
      http.end();
      return false;
    }*/
}
