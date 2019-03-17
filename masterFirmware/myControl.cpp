#include <WiFi.h>              // Built-in
#include "myconfig.h"
#include "myControl.h"

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
const char* mqtt_server = "192.168.1.109";
const char* mqtt_output_topic = "novella/devices/lampshade/response";
const char* mqtt_ping_topic   = "novella/devices/lampshade/ping/";
const char* mqtt_command_topic   = "novella/devices/lampshade/command/";

char mqtt_input_topic[50];
char deviceID[18];
String devID = " ";
char* pingMsg;
int len=0;
int FILE_BUF_SIZE = 512;    // must correlate with mqtt buf size

File mqttFile;

DataBuffer dataStore(sColumn);         // Data storer object
Settings mySettings;                   // Initialize settings object
  
// function to initiialize mqtt
void mqttInit(void){
  debugln("MQTT: Initilializing mqtt");
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(mqttCallback);

  uint64_t chipid = ESP.getEfuseMac();
  sprintf(deviceID, "S%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
  devID = String(deviceID);
  String tem = String(MQTT_BASE_TOPIC) + "/" + devID;
  
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
}


// Call back function to process received data
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  bool sendToSlave = false;
  debug("MQTT: Message arrived [");  debug(topic); debugln("] ");
  printArray((char*)payload, length);
  char* sBuf[length];
  memcpy(sBuf, payload, length);
  
  String tstring = String(topic);
  DynamicJsonBuffer  jsonBuffer(200);     // Config messages will be sent as Json
  JsonObject& root = (tstring.indexOf("mid") == -1) ? jsonBuffer.parseObject(payload) : jsonBuffer.createObject();

  // Receiving file
  if(tstring.indexOf("image") > -1){    // Check if the message is a file ( Picture )
    if(tstring.indexOf("mid") > -1){   // receiving file content
      debugln(".here");
      mqttFile.write(payload, FILE_BUF_SIZE);        // write file
      mqttReply("OK");
    }
    else if(tstring.indexOf("start") > -1){  // check is this is a start message
      String filen = root["filename"];
      debug("MQTT: Receiving file: "); debugln(filen); 
      if(SPIFFS.exists(filen)){             // Delete file if it already exists
        debugln("MQTT: Previous file found, deleting...");
        SPIFFS.remove(filen);
      }
      mqttFile = SPIFFS.open(filen, FILE_WRITE);
      if(!mqttFile){
        errorHandler("Error opening file");
      }
      mqttReply("OK");
    }
    else if(tstring.indexOf("end") > -1){    // handle end of file reception
      debugln("MQTT: File Complete.");
      mqttFile.close();
      mqttReply("OK");
      statusLed.flashGreen(4);
    }
    return ;
  }

  // Receiving command
  else if(root.containsKey("command")){   
    String cmd = root["command"];

    // Command to start displaying file
    if(cmd == "Start_Display"){
      if(!root.containsKey("file")){
        mqttReply("No File Specified"); 
        return;
      }
      const char* dispFile = root["file"];
      bool ans = startImage(dispFile);
      if(ans){
        mqttReply("Success");
      }
      else {
        mqttReply("Failed");  
        errorHandler("Error starting display");    
      }
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
      arm2.stop();
      mqttReply("OK");
      return;
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
      String temp = root["value"];
      int val = temp.toInt();
      debug("MQTT: Setting delay between columns: "); debugln(val);
      mySettings.delayBtwColumns = val;
      mqttReply("OK");
      sendToSlave = true;
    }

    else if(cmd == "Saved_Data"){
      debugln("MQTT: Receiving saved settings");
      mySettings.delayBtwColumns = root["Delay_Columns"];
      mySettings.setBrightness(root["Brightness"]);
      mySettings.brightnessMode = root["Brightness_Mode"];

      mySettings.isPaired = 1;
      statusLed.off();
      
      sendToSlave = true;
       
      const char* img = root["image"];
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
  debug("MQTT Output: "); debugln(temp1);
  
  mqttclient.publish(mqtt_output_topic, temp1);
  free(temp1);
}

// mqtt command
void mqttCommand(char* msg){
  DynamicJsonBuffer  jsonBuffer(300);
  JsonObject& root = jsonBuffer.createObject();
  root["id"] = devID;
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
  dataStore.setBuffer((char*)img);   // copy data from file into buffer
  arm1.stop();
  arm2.stop();

  statusLed.flashYellow(2);
  if(!slave1.attemptSend()){   // send to slave1
    debugln("MQTT: Unable to send to slave");
    mqttReply("Error");
    return 0;
  }
  
  debugln("Data sent to slave");
  mqttReply("OK");
  setArmData(dataStore._buffer, dataStore._noColumns);

 if(arm1.isTaskCreated() && arm2.isTaskCreated()){
    arm1.resume();          // resume task if already created
    arm2.resume();
  }
  else{
    createTask(&arm1);          // Create task if not already created
    createTask(&arm2);
  }  
  debugln("Image display started");
  statusLed.flashBlue(4);
  return 1;
}

