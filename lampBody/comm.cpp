#include "myconfig.h"
#include "comm.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

WiFiClient espClientm;
PubSubClient mqttclient(espClientm);

long lastPingTime = 0;

const char* mqtt_server = "192.168.1.109";
const char* mqtt_output_topic = "novella/devices/lampbody/response/";
const char* mqtt_ping_topic   = "novella/devices/lampbody/ping/";
const char* mqtt_command_topic   = "novella/devices/lampbody/command/";

char mqtt_input_topic[50];
char deviceID[18];
String devID = " ";
char* pingMsg;

void mqttCallback(char* topic, byte* payload, unsigned int length) ;

Settings mySettings;

// function to initiialize mqtt
void mqttInit(void){
  memset(mqtt_input_topic, 0, 50);
  debugln("MQTT: Initializing mqtt");
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(mqttCallback);

  uint64_t chipid = ESP.getEfuseMac();
  sprintf(deviceID, "B%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid); // Lampbodies will have a 'B' in front
  devID = String(deviceID);
  String tem = String(MQTT_BASE_TOPIC) + "/" + devID + "/#";
  
  tem.toCharArray(mqtt_input_topic, tem.length()+1);
  debug("MQTT Topics: "); debugln(mqtt_input_topic);

  // Generate ping message to save overhead
  DynamicJsonBuffer  jsonBuffer(200);
  JsonObject& root = jsonBuffer.createObject();
  root["id"] = devID;
  root["type"] = "lampbody";
  pingMsg = (char*)malloc(root.measureLength()+1);
  root.printTo(pingMsg, root.measureLength()+1);
  debug("Ping Message: "); debugln(pingMsg);

  mqttReconnect();
  mqttCommand("Starting");
}


// Call back function to process received data
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  debug("MQTT: Message arrived [");  debug(topic); debugln("] ");
  printArray((char*)payload, length);

  String tstring = String(topic);

  /* Parse MQTT Message */
  DynamicJsonBuffer  jsonBuffer(200);     // Config messages will be sent as Json
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    mqttReply("parseObject() failed");
    return;
  }

  if(root.containsKey("command")){
    String cmd = root["command"];

    // switch through the different types of commands
    if(cmd == "Motor_On"){
      motor.enable(); 
      mqttReply("OK");
    }
    else if(cmd == "Motor_Off"){
      motor.disable(); 
      mqttReply("OK");
    }
    else if(cmd == "Set_Motor_Speed"){  // In percentage (%)
      if(root.containsKey("value")){
        mySettings.setMotorSpeed(root["value"]);
        mqttReply("OK");
      }
      else {
        mqttReply("Error, value not specified");
      }
    }
    else if(cmd == "Set_ColdLed_Brightness"){  // In percentage (%)
      if(root.containsKey("value")){
        mySettings.setColdLed(root["value"]);
        mqttReply("OK");
      }
      else {
        mqttReply("Error, value not specified");
      }
    }
    else if(cmd == "Set_WarmLed_Brightness"){  // In percentage (%)
      if(root.containsKey("value")){
        mySettings.setWarmLed(root["value"]);
        mqttReply("OK");
      }
      else {
        mqttReply("Error, value not specified");
      }
    }

   else if(cmd == "Saved_Data"){
      debugln("MQTT: Receiving saved settings");
      mySettings.setMotorSpeed(root["Motor_Speed"]);
      mySettings.setWarmLed(root["WarmLed_Brightness"]);
      mySettings.setColdLed(root["ColdLed_Brightness"]);
    }
   
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

// mqtt command
void mqttCommand(char* msg){
  DynamicJsonBuffer  jsonBuffer(300);
  JsonObject& root = jsonBuffer.createObject();
  root["id"] = devID;
  root["type"] = "lampbody";
  root["command"] = String(msg);
  char* temp1;
  temp1 = (char*)malloc(root.measureLength()+1);
  root.printTo(temp1, root.measureLength()+1);
  debug("MQTT Command: "); debugln(temp1);
  
  mqttclient.publish(mqtt_command_topic, temp1);
  free(temp1);
}

// mqtt Reply
void mqttReply(char* msg){
  DynamicJsonBuffer  jsonBuffer(200);
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
