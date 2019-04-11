#include "myconfig.h"
#include "arm.h"
#include "wrapper.h"
#include "comm.h"
#include "myControl.h" 
#include "esp_task_wdt.h"

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
   /*if(dir){
      _colPointer = 0;
      dir = false;
    }
    else{
      _colPointer = _noColumns - 1;
      dir = true;
    }*/
    _colPointer = 0;
   //if(mSema != NULL)
   {
    xHigherPriorityTaskWoken = pdTRUE;
    xSemaphoreGiveFromISR(mSema,  &xHigherPriorityTaskWoken);
   }
   
}


void arm::showColumn(uint8_t* buf){   
    leds->setBuffer(buf, _len);
    leds->show();
}


void arm::setCore(uint8_t core){
  _core = core;
}


void arm::showImage(){
  debugln("INIT: Showing Image on arm: " + String(arm_no));
  Serial.print("no columns: "); Serial.println(_noColumns);
  mSema =xSemaphoreCreateBinary();
  
  while(1){
    xSemaphoreTake( mSema, 10/portTICK_PERIOD_MS );
    /*if(dir){
      while(_colPointer >= 0){
        if(_colPointer >= _noColumns) break;
        showColumn( _imgData[_colPointer] );        // show the next column
        _colPointer = _colPointer - 1 ;
        ets_delay_us(mySettings.delayBtwColumns);
      }
    }else{
      while(_colPointer < _noColumns){
        if(_colPointer < 0) break;
        showColumn( _imgData[_colPointer] );        // show the next column
        _colPointer = _colPointer + 1 ;
        ets_delay_us(mySettings.delayBtwColumns);
      }
    }*/
    while(_colPointer < _noColumns){
        if(_colPointer < 0) break;
        showColumn( _imgData[_colPointer] );        // show the next column
        _colPointer = _colPointer + 1 ;
        ets_delay_us(mySettings.delayBtwColumns);
      }
    leds->clear();
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

