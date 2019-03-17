#include "FS.h"
#include "SPIFFS.h"
#include "myconfig.h"
#include "comm.h"
#include "myControl.h"
#include "firstTime.h"
#include "arm.h"
#include "wrapper.h"
#include "led_driver.h"

// Slave device
slave slave1(SLAVE1_TX, SLAVE1_RX, 1);

// Led arms on master
arm arm1(HSPI, CLOCK_PIN1, MISO_PIN1, DATA_PIN1, SS_PIN1, IMAGE_HEIGHT, 1);
arm arm2(VSPI, CLOCK_PIN2, MISO_PIN2, DATA_PIN2, SS_PIN2, IMAGE_HEIGHT, 2);

// Status leds
StatusLed statusLed(RED_LED, GREEN_LED, BLUE_LED);

// Reset task
TaskHandle_t resetTaskHandle;

void setup(){
  myInit();          // Initialization
  networkInit();
  mqttInit();
  armsInit();

  debug("INIT: Initialization complete.");
}


void loop(){
  delay(5);
  mqttLoop();
}

// Function to initialize my settings
void myInit(){
  debugSerial.begin(115200);
  debugln("INIT: Setting up file system");
  if(!SPIFFS.begin(true)){
      debugln("INIT: SPIFFS Mount Failed");
      return;
  }

  // set to green until settings does other wise
  statusLed.green();
  
  // Initialize reset pin
  pinMode(RESET_SWITCH, INPUT_PULLUP);    
  attachInterrupt(digitalPinToInterrupt(RESET_SWITCH), resetIsr, FALLING );
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
  while(!digitalRead(RESET_SWITCH));  // Wait until button is released
  if(millis()-duration > 5000){  // Timeout occured
    debugln("Resetting lamp");
    mqttCommand("Reset");
  }
  vTaskDelete(resetTaskHandle);
}

// Initialize the slave part of our master
void armsInit(void){
  arm1.setCore(0);  // set up arms, let both arm tasks run on core 0
  arm2.setCore(0);
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
  statusLed.flashRed(2);
}
