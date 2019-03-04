#include "myconfig.h"
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>              // Built-in
#include "firstTime.h"
#include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include "WiFiManager.h"
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "myControl.h"

char* myConfigData;            // char array to store config string
volatile bool shouldSaveConfig;

// Function to initialize network
void networkInit() {
    //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.resetSettings();    //reset settings - for testing
  wifiManager.setSaveConfigCallback(saveConfigCallback);   //set config save notify callback
  debugln("NETWORK: Attempting to connect to wifi...");
  if(!wifiManager.autoConnect(AUTOCONNECT_NAME, AUTOCONNECT_PASSWORD)) {
    errorHandler("NETWORK: failed to connect to router and hit timeout");
    ESP.restart();    //reset and try again, or maybe put it to deep sleep
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  debugln("NETWORK: Successfully connected to WiFi.");

  if(shouldSaveConfig){
      generateConfig();   // generate and save default config
  }
}

// function to be called when for saving
void saveConfigCallback () {
  debugln("NETWORK: Should save config");
  shouldSaveConfig = true;
}

// function to generate default config file in memory
void generateConfig(){
  debugln("CONFIG: Generating default config file.");
  // check for config file
  if(SPIFFS.exists(CONFIG_FILENAME)){
    errorHandler("CONFIG: Previous config file detected, Aborting.");
    return;
  }

  DynamicJsonBuffer  jsonBuffer(200);
  JsonObject& root = jsonBuffer.createObject();

  root["Brightness_Control_Mode"] = DEFAULT_BRIGHTNESS_CONTROL_MODE;
  root["Brightness_Value"]        = DEFAULT_BRIGHTNESS_VALUE;
  root["Default_Image"]           = DEFAULT_IMAGE;

  root.prettyPrintTo(debugSerial);
  char sbuf[root.measureLength() + 1];
  root.printTo(sbuf, root.measureLength() + 1);

  // open file to save
  File file = SPIFFS.open(CONFIG_FILENAME, FILE_WRITE);
  if(!file){
      errorHandler("FILE: Failed to open file for writing");
      return;
  }
  if(file.print(sbuf)){
    debugln("FILE: Default Config saved successfully.");
  } else {
    errorHandler("FILE: Error while saving default config.");
  }
  file.close();
}

// Function to get configuration, Note: This function is to be called only once at the beginning
void getConfig(){
  debugln("INIT: Getting Config file ..");
  File file = SPIFFS.open(CONFIG_FILENAME, FILE_READ);
  if(!file){
      errorHandler("FILE: Failed to open Config file for Reading.");
      return;
  }
  int len = file.size();
  myConfigData = (char*)malloc(len);    // Allocate memory for array

  if(file.read((uint8_t*)myConfigData, len)){
    debugln("FILE: Read Config file successfully.");
  } else {
    errorHandler("FILE: Error while reading config file");
    file.close();
    return;
  }
  file.close();

  DynamicJsonBuffer  jsonBuffer(200);
  JsonObject& root = jsonBuffer.parseObject(myConfigData);
  if (!root.success()) {
    errorHandler("parseObject() failed");
    return;
  }
  root.prettyPrintTo(debugSerial);
  /*brightnessMode = root["Brightness_Control_Mode"];
  brightnessVal  = root["Brightness_Value"];*/
}

// Save configuration values back to config file
void saveConfig(){
  debugln("INIT: Saving Configuration.");
  DynamicJsonBuffer  jsonBuffer(200);
  JsonObject& root = jsonBuffer.parseObject(myConfigData);
  if (!root.success()) {
    errorHandler("parseObject() failed");
    return;
  }

  /*root.prettyPrintTo(debugSerial);
  root["Brightness_Control_Mode"] = brightnessMode;
  root["Brightness_Value"]        = brightnessVal;*/
  free(myConfigData);
  int len = root.measureLength() + 1;
  myConfigData = (char*)malloc(len);    // Allocate memory for array
  root.printTo(myConfigData, len);

  SPIFFS.remove(CONFIG_FILENAME);   // remove old config file

  // open file to save
  File file = SPIFFS.open(CONFIG_FILENAME, FILE_WRITE);
  if(!file){
    errorHandler("FILE: Failed to open file for writing");
    return;
  }
  if(file.print(myConfigData)){
    debugln("FILE: Config saved successfully.");
  } else {
    errorHandler("FILE: Error while saving config.");
    return;
  }
  file.close();
}
