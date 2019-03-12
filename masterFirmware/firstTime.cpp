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

volatile bool shouldSaveConfig;

// Function to initialize network
void networkInit() {
  WiFiManager wifiManager;
  //wifiManager.resetSettings();    //reset settings - for testing
  //wifiManager.setSaveConfigCallback(saveConfigCallback);   //set config save notify callback
  debugln("NETWORK: Attempting to connect to wifi...");
  if(!wifiManager.autoConnect(AUTOCONNECT_NAME, AUTOCONNECT_PASSWORD)) {
    errorHandler("NETWORK: failed to connect to router and hit timeout");
    ESP.restart();   
    delay(1000);
  }
  debugln("NETWORK: Successfully connected to WiFi.");
}


// function to be called when for saving
void saveConfigCallback () {
  debugln("NETWORK: Should save config");
  shouldSaveConfig = true;
}

