#include "wrapper.h"
#include "myconfig.h"

void setupIsr(){
  // Initialize trigger pin as interrupt to notify us form master
  debugln("INIT: Setting up ISRs for arms");
  pinMode(hallSensor1, INPUT_PULLUP);
  pinMode(hallSensor2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallSensor1), isr1, CHANGE);
}

void resetArms(){
  started_1 = false;
  arm1.dir = false;
}

void IRAM_ATTR isr1(){
  portENTER_CRITICAL_ISR(&(arm1.mux));
  if(!started_1){
    if(digitalRead(hallSensor1)){     // if we have not started, start only if we are latched to high
      started_1 = true;
      arm1.execIsr();
    }
  }
  else{
    arm1.execIsr();
  }
  portEXIT_CRITICAL_ISR(&(arm1.mux));
}


void Arm1Task(void *pvparameters){
  arm1.showImage();
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
       -1,             /* Priority of the task original 1 */
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

