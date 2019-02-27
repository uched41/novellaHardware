#include "arm.h"
#include "myconfig.h"
#include "wrapper.h"

arm::arm(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin, int len){
  leds = new ledDriver(myspi, clkPin, misoPin, dataPin, ssPin);
  _len = len;
  leds->begin(len);
}

void IRAM_ATTR arm::execIsr(){
  /* 
   *  ISR will start the animation if it has not started, or reset columnPointer to zero
   *  which means that we hav reached the 180 degrees point
   */
   portENTER_CRITICAL_ISR(&mux);
   canRun = true;
   debug("Restarting Image on arm: " + String(arm_no) );
    _colPointer = 0;
   portEXIT_CRITICAL_ISR(&mux);
}


void arm::showColumn(uint8_t* buf){
  
    portENTER_CRITICAL_ISR(&mux);     // mutex will prevent 2 cores from accessing memory at the same time
    leds->setBuffer(buf, _len);
    portEXIT_CRITICAL_ISR(&mux);
    leds->show();
}


void arm::setCore(uint8_t core){
  _core = core;
  arm_no = _core + 1;
}


void arm::setImage(uint8_t** imgData){
  _imgData = imgData;
}


void arm::showImage(){
  debug("INIT: Showing Image on arm: " + String(arm_no));
  Serial.print("no colls"); Serial.println(noColumns);
  while(1){
    isRunning = true;
<<<<<<< HEAD
    if(canRun){
=======
    if(_colPointer < noColumns){
>>>>>>> refs/remotes/origin/master
      showColumn( _imgData[_colPointer] );           // show the next column
      _colPointer   = (_colPointer + 1);    // increment the column pointer and make sure that we dont exceed the maximum
      delayMicroseconds(delayBetweenColumns);
      canRun = false;
    }
<<<<<<< HEAD
=======
    else{
      leds->clear();
    }
>>>>>>> refs/remotes/origin/master
    //vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}



bool arm::isTaskCreated(){
  return taskCreated;
}


void arm::stop(){
   if(isTaskCreated()){
    debug("INIT: Suspending task on arm: "+ String(arm_no));
    //vTaskSuspend(NULL);
    isRunning = false;
   }
   else debug("ERROR: No Task found");
}


void arm::resume(){
  if(isTaskCreated()){
    debug("INIT: Resuming task on arm: "+ String(arm_no));
    _colPointer = 0;     // set column pointer back to zero
    //vTaskResume(_mytask);
  }
  else debug("ERROR: No Task found");
}


