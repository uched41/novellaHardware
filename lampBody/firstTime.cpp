#include "FS.h"
#include "SPIFFS.h"
#include "myconfig.h"
#include <WiFi.h>              // Built-in
#include "firstTime.h"
#include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include "WiFiManager.h"
#include <DNSServer.h>
#include <ArduinoJson.h>

char* myConfigData;            // char array to store config string
volatile bool shouldSaveConfig;

// Function to initialize network
void networkInit() {
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

}

// Function to get configuration, Note: This function is to be called only once at the beginning
void getConfig(){

}

// Save configuration values back to config file
void saveConfig(){

}
