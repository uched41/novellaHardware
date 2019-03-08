#include "myconfig.h"
#include "arm.h"
#include "wrapper.h"
#include "comm.h"
#include "myControl.h" 

uint8_t** arm::_imgData = NULL;
int arm::_noColumns = 0;

arm::arm(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin, int len, uint8_t armno){
  leds = new ledDriver(myspi, clkPin, misoPin, dataPin, ssPin);
  _len = len;
  leds->begin(len);
  arm_no = armno;
}

void IRAM_ATTR arm::execIsr(){
  /*
   *  ISR will start the animation if it has not started, or reset columnPointer to zero
   *  which means that we hav reached the 180 degrees point
   */
   portENTER_CRITICAL_ISR(&mux);
   debugln("ARM: Restarting Image on arm: " + String(arm_no) );
    _colPointer = 0;
   portEXIT_CRITICAL_ISR(&mux);
}


void arm::showColumn(uint8_t* buf){
    portENTER_CRITICAL_ISR(&mux);     // mutex will prevent 2 cores from accessing memory at the same time
    leds->setBuffer(buf, _len);
    leds->show();
    portEXIT_CRITICAL_ISR(&mux);
    
}


void arm::setCore(uint8_t core){
  _core = core;
}


void arm::showImage(){
  debugln("INIT: Showing Image on arm: " + String(arm_no));
  Serial.print("no columns: "); Serial.println(_noColumns);
  while(1){
    isRunning = true;
    if(_colPointer < _noColumns){
      showColumn( _imgData[_colPointer] );        // show the next column
      _colPointer   = (_colPointer + 1);          // increment the column pointer and make sure that we dont exceed the maximum
      delayMicroseconds(mySettings.delayBtwColumns);
    }
    else{
      leds->clear();
    }
     //vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


bool arm::isTaskCreated(){
  return taskCreated;
}


void arm::stop(){
   if(isTaskCreated()){
    debugln("ARM: Suspending task on arm: "+ String(arm_no));
    vTaskSuspend(_mytask);
    isRunning = false;
   }
   else debug("ARM: No Task found");
}


void arm::resume(){
  if(isTaskCreated()){
    debugln("ARM: Resuming task on arm: "+ String(arm_no));
    _colPointer = 0;     // set column pointer back to zero
    vTaskResume(_mytask);
  }
  else debugln("ARM: No Task found");
}

void setArmData(uint8_t** buf, int newlen){
  // first we stop the arms
  debugln("Stopping arms");
  arm1.stop();
  arm2.stop();

  // then we clear the current buffers
  debugln("Freeing old memory");
  for(int i=0; i<arm::_noColumns; i++){
    free(arm::_imgData[i]);
  }

  // copy new data
  arm::_imgData = (uint8_t**)malloc( sizeof(uint8_t*)*newlen);
  for(int i=0; i<newlen; i++){
    arm::_imgData[i] = (uint8_t*)malloc(sColumn);
    memcpy(arm::_imgData[i], buf[i], sColumn);
  }

  arm::_noColumns = newlen;
  
  debugln("New Data copied to arm buffers");
}
