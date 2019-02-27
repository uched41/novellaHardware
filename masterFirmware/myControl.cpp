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

uint8_t brightnessMode = 0;
uint8_t brightnessVal = 4;

long lastPingTime = 0;

WiFiClient espClientm;
PubSubClient mqttclient(espClientm);
const char* mqtt_server = "192.168.1.125";
const char* mqtt_output_topic = "novella/devices/lampshade/response";
const char* mqtt_ping_topic   = "novella/devices/lampshade/ping/";
char mqtt_input_topic[50];
char deviceID[18];
String devID = " ";
char* pingMsg;
int len=0;
int FILE_BUF_SIZE = 512;    // must correlate with mqtt buf size

File mqttFile;

DataBuffer dataStore(sColumn);         // Data storer object

// function to initiialize mqtt
void mqttInit(void){
  debugln("MQTT: Initilializing mqtt");
  mqttclient.setServer(mqtt_server, 1883);
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
}


// Call back function to process received data
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  debug("MQTT: Message arrived [");  debug(topic); debugln("] ");
  printArray((char*)payload, length);

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
        debugln("Error opening file");
      }
      mqttReply("OK");
    }
    else if(tstring.indexOf("end") > -1){    // handle end of file reception
      debugln("MQTT: File Complete.");
      mqttFile.close();
      mqttReply("OK");
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
      debugln("MQTT: Starting Image, " + String(dispFile));
      dataStore.setBuffer((char*)dispFile);   // copy data from file into buffer

      arm1.stop();
      arm2.stop();
      
      if(!slave1.attemptSend()){   // send to slave1
        debugln("MQTT: Unable to send to slave");
        mqttReply("Error");
        return;
      }
      
      debugln("MQTT: Data sent to slave");
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
      //mqttReply((char*)"OK");
      return;
    } else {
      debug("failed, rc=");
      debug(mqttclient.state());
      debugln(" try again in 2 seconds");
      statusLed.flashGreen();
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

// Brightness control
void brightnessControl(void){
  if(brightnessMode==1){   // Automatic mode will read from ldr sensor
      brightnessVal = map(analogRead(LDR_PIN), 0, 1023, MAX_BRIGHTNESS, 0);
  }
}
