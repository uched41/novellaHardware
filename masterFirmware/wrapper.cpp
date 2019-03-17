#include "wrapper.h"
#include "myconfig.h"

void setupIsr(){
  // Initialize trigger pin as interrupt to notify us form master
  debugln("INIT: Setting up ISRs for arms");
  pinMode(hallSensor1, INPUT_PULLUP);
  pinMode(hallSensor2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallSensor1), isr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hallSensor2), isr2, CHANGE);
}

void isr1(){
  arm1.execIsr();
}

void isr2(){
  arm2.execIsr();
}

void Arm1Task(void *pvparameters){
  arm1.showImage();
}

void Arm2Task(void *pvparameters){
  arm2.showImage();
}

bool createTask(arm* myarm){
  if(myarm->_imgData == NULL){         // make sure that the image data location has been set
    debug("ERROR: Image data not set");
    return false;
  }

  if(!myarm->taskCreated){
    myarm->_colPointer = 0;     // set column pointer back to zero
    debugln("INIT: Creating task on arm: "+ String(myarm->arm_no));

    if(myarm == &arm1){
      xTaskCreatePinnedToCore(
       Arm1Task,     /* Function to implement the task */
       "Arm1 Display", /* Name of the task */
       10000,         /* Stack size in words */
       NULL,          /* Task input parameter */
       0,             /* Priority of the task */
       &(myarm->_mytask),       /* Task handle. */
       myarm->_core);        /* Core where the task should run */

    }
    else{
      xTaskCreatePinnedToCore(
       Arm2Task,     /* Function to implement the task */
       "Arm2 Display", /* Name of the task */
       10000,         /* Stack size in words */
       NULL,          /* Task input parameter */
       0,             /* Priority of the task */
       &(myarm->_mytask),       /* Task handle. */
       myarm->_core);        /* Core where the task should run */

    }

    myarm->taskCreated = true;
  }
  else{
    debugln("Task Already created on arm: "+ String(myarm->arm_no));
  }
  return true;
}
