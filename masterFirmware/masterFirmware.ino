#include "FS.h"
#include "SPIFFS.h"
#include "myconfig.h"
#include "comm.h"
#include "myControl.h"
#include "firstTime.h"
#include "arm.h"
#include "wrapper.h"
#include "led_driver.h"
#include "esp_task_wdt.h"

// Slave device
slave slave1(SLAVE1_TX, SLAVE1_RX, 1);

// Led arms on master
arm arm1(HSPI, CLOCK_PIN1, MISO_PIN1, DATA_PIN1, SS_PIN1, IMAGE_HEIGHT, 1);

// Status leds
StatusLed statusLed(RED_LED, GREEN_LED, BLUE_LED);

// Reset task
TaskHandle_t resetTaskHandle;


volatile bool started_1 = false;   // this boolean will tell us if this is the first rotation, so we can act accordingly

portMUX_TYPE serMutex = portMUX_INITIALIZER_UNLOCKED;  // mutex for sending to slave

void setup(){
  myInit();          // Initialization
  networkInit();
  mqttInit();
  armsInit();

  disableCore0WDT();
  disableCore1WDT();
   
  debugln("INIT: Initialization complete.");
  statusLed.flashPurple(3);
}


void loop(){
  vTaskDelay(5/ portTICK_PERIOD_MS);
  mqttLoop();
}

// Function to initialize my settings
void myInit(){
  debugSerial.begin(1000000);
  debugln("INIT: Setting up file system");
  if(!SPIFFS.begin(true)){
      debugln("INIT: SPIFFS Mount Failed");
      statusLed.failed();
      return;
  }

  debugln("\nFlash size: " + String(ESP.getFlashChipSize()) ); 
  debugln("Free heap: " + String(ESP.getFlashChipSize()) ); 
  debugln("SPIFFS Total bytes: " + String(SPIFFS.totalBytes()) );
  debugln("SPIFFS Used bytes:" + String(SPIFFS.usedBytes()) );
  debugln(); 
    
  // set to green until settings does other wise
  statusLed.green();

  // Inialize sync pin
  pinMode(SYNC_PIN, OUTPUT);
  
  // Initialize reset pin
  pinMode(RESET_SWITCH, INPUT);   
  digitalWrite(RESET_SWITCH, LOW);
  attachInterrupt(digitalPinToInterrupt(RESET_SWITCH), resetIsr, RISING );
}

// Isr to trigger reset task
void resetIsr(){
  debugln("Creating reset task");
  xTaskCreate(resetTask,  "Reset_Task",  2000,  &resetTaskHandle,  1, NULL);
}

// task to wait for reset pin to be pressed
void resetTask(void *pvparameters){
  debugln("Starting reset task");
  
  unsigned long duration = millis();
  //statusLed.red();
  while(digitalRead(RESET_SWITCH));  // Wait until button is released
  if(millis()-duration > 5000){  // Timeout occured
    debugln("Resetting lamp");
    statusLed.flashRed(6);
    mqttCommand("Reset");
  }
  vTaskDelete(resetTaskHandle);
}

// Initialize the slave part of our master
void armsInit(void){
  arm1.setCore(1);  
  setupIsr();
}

// function to print an array of bytes
void printArray(char* arr, int len){
  for(int i=0; i<len; i++){
    debug(arr[i]);
  }
  debugln();
}

// Error handler function
void errorHandler(char* msg){
  debugln(msg);
  statusLed.failed();
}

