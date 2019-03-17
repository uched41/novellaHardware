#include "myconfig.h"
#include "firstTime.h"
#include "motor.h"
#include "comm.h"

void setup() {
  myInit();          // Initialization
  networkInit();
  mqttInit();

  debugln("INIT: Initialization complete.");
}

void loop() {
  delay(5);
  mqttLoop();
}

// Function to initialize my settings
void myInit(){
  debugSerial.begin(115200);
  pinMode(CONSTANT_PWM_ENABLE_PIN, OUTPUT);
  
  // Constant pwm for coil 
  ledcSetup(CONSTANT_PWM_CHANNEL, CONSTANT_PWM_FREQUENCY, 8);    // Initialize PWM for motor
  ledcAttachPin(CONSTANT_PWM_PIN, CONSTANT_PWM_CHANNEL);
  ledcWrite(CONSTANT_PWM_CHANNEL, 128);
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
}
