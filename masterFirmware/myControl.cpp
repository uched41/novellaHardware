#include <WiFi.h>              // Built-in
#include "myconfig.h"
#include "myControl.h"
#include <ESPmDNS.h>

#define MQTT_MAX_PACKET_SIZE 1024
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
String mfname;
char deviceID[18];
String devID = " ";
char* pingMsg;
int len=0;
int FILE_BUF_SIZE = 512;    // must correlate with mqtt buf size

File mqttFile;

//DataBuffer dataStore(sColumn);          // Data storer object
DataBuffer mqttStore(FILE_BUF_SIZE);    // This uses a bigger column size because it is used to store data received from mqtt temporarily before it is saved to a file.
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

  debug("MQTT Topics: "); debugln(mqtt_input_topic);
  mqttReconnect();
  //mqttCommand("Starting");
}


// Call back function to process received data
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  bool sendToSlave = false;
    
  char* sBuf[length];
  memcpy(sBuf, payload, length);
  
  String tstring = String(topic);
  DynamicJsonBuffer  jsonBuffer(200);     // Config messages will be sent as Json
  JsonObject& root = (tstring.indexOf("mid") == -1) ? jsonBuffer.parseObject(payload) : jsonBuffer.createObject();

  // Receiving file
  if(tstring.indexOf("image") > -1){    // Check if the message is a file ( Picture )

    if(tstring.indexOf("mid") > -1){   // receiving file content
      memcpy(mqttStore._buffer[mqttStore.scount], payload, FILE_BUF_SIZE);
      //dumpBuf(mqttStore._buffer[mqttStore.scount], FILE_BUF_SIZE);
      mqttStore.scount = mqttStore.scount + 1;
      debug('.');
      mqttReply("OK");
    }
    else if(tstring.indexOf("start") > -1){  // check is this is a start message
      statusLed.onReceiving();
      const char* filen = root["filename"];
      mfname = String(filen);
      int flen = root["no_lines"];

      if(SPIFFS.exists(filen)){             // Delete file if it already exists
        debugln("MQTT: Previous file found, deleting...");
        SPIFFS.remove(filen);
      }
      
      debug("MQTT: Receiving file: "); debugln(filen); 
      debug("MQTT: No of lines "); debugln(flen);
      debugln("Initializing buffer ..");
      if(!mqttStore.initBuffer(flen)) {
        debugln("Failed to receive image");
        return;
      }
      mqttStore.scount = 0;    
      mqttReply("OK");
    }
    else if(tstring.indexOf("end") > -1){    // handle end of file reception
      debug("\nMQTT: File data received: "); debugln(mfname.c_str());
      mqttFile = SPIFFS.open(mfname.c_str(), FILE_WRITE);
      if(!mqttFile){
        errorHandler("Error opening file");
      }

      for(int i=0; i< mqttStore._noColumns; i++){
        mqttFile.write(mqttStore._buffer[i], FILE_BUF_SIZE); 
      }
      mqttStore.clearBuffer();
      
      mqttFile.close();
      debugln("MQTT: File received successfully.");
      mqttReply("OK");
      statusLed.flashGreen(4);
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
      debugln("Starting file: " + String(dispFile));
      
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

    // Command to get files in memory 
    else if(cmd == "Get_Files"){
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

    // Command to set divider
    else if(cmd == "Divider"){
      int val = root["value"];
      debug("MQTT: Setting color divider: "); debugln(val);
      mySettings.divider = val;
      mqttReply("OK");
      sendToSlave = true;
    }

    else if(cmd == "Saved_Data"){
      debugln("MQTT: Receiving saved settings");
      mySettings.delayBtwColumns = root["Delay_Columns"];
      mySettings.setBrightness(root["Brightness"]);
      mySettings.brightnessMode = root["Brightness_Mode"];
      mySettings.divider = root["Divider"];
      
      mySettings.isPaired = 1;
      statusLed.off();
      
      sendToSlave = true;
       
      const char* img = root["Image"];
      bool ans = startImage(img);
      if(ans){
        mqttReply("Success");
      }
      else {
        mqttReply("Failed"); 
        errorHandler("Error starting image");
      }
    }

    
    // forward message to slave if needed
    if(sendToSlave){
      slave1.attempMsg((char*)sBuf, length);
    }
    
    return ;
  }
  
}

// Function to reconnect to mqtt
void mqttReconnect() {
  while (!mqttclient.connected()) {         // Loop until we're reconnected
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
   debugln("pinging");
}

// mqtt Reply
void mqttReply(char* msg){
  DynamicJsonBuffer  jsonBuffer(300);
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
  debugln("Starting Image, " + String(img));
  
  arm1.stop();
  statusLed.onTransferring();
   
  setArmData(img);
  resetArms();
  
 if(arm1.isTaskCreated()){
    arm1.resume();          // resume task if already created
  }
  else{
    createTask(&arm1);          // Create task if not already created
  }  
  debugln("Image display started");
  statusLed.onDisplaying();

  if(!tempStore.readFromFile(img)) {
    debugln("Failed to set buffer");
    return false;
  }
  if(!slave1.attemptSend()){   // send to slave1
    debugln("MQTT: Unable to send to slave");
    mqttReply("Error");
    return 0;
  }
  tempStore.clearGif();
  
  return 1;
}


