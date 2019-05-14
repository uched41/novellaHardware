#include "myconfig.h"
#include "arm.h"
#include "wrapper.h"
#include "comm.h"

ARM_DATA* arm::_imgData = NULL;

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
   if(_imgData == NULL) return;
    _colPointer = 0;
    _imgPointer = (_imgPointer + 1 ) % (_imgData->_noImages);    // go to next image
   //if(mSema != NULL)
   {
    xHigherPriorityTaskWoken = pdTRUE;
    xSemaphoreGiveFromISR(mSema,  &xHigherPriorityTaskWoken);
   }
   
}


void arm::showColumn(uint8_t* buf){   
    leds->setBuffer(buf, _len);
    //dumpBuf(buf, _len*3);
    leds->show();
}


void arm::setCore(uint8_t core){
  _core = core;
}


void arm::showImage(){
  debugln("INIT: Showing Image on arm: " + String(arm_no));
  Serial.print("No Images: "); Serial.println(_imgData->_noImages);
  Serial.print("No Columns: "); Serial.println(_imgData->_noColumnsPerImage);
  _imgPointer = 0;
  _colPointer = 0;
  mSema =xSemaphoreCreateBinary();
  
  while(1){
    xSemaphoreTake( mSema, portMAX_DELAY );    // 10/portTICK_PERIOD_MS
    ets_delay_us(mySettings.moveDelay);       // move delay for image alignment
    while(_colPointer < _imgData->_noColumnsPerImage){
        if(_colPointer < 0) break;
        //debugln("Image no: " + String(_imgPointer) + " , column: " + String(_colPointer));
        showColumn( _imgData->_frames[_imgPointer]->_data[_colPointer] );        // show the next column
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

void setArmData(const char* file){
  // first we stop the arms
  debugln("Stopping arms");
  arm1.stop();
  if(arm::_imgData == NULL ){     // for the first time
    arm::_imgData = new ARM_DATA();
  }

  if(arm::_imgData->readFromFile(file) ){
    debugln("New Data copied to arm buffers");
  }
}

void setArmData(ARM_DATA* newBuf){
  // first we stop the arms
  debugln("Stopping arms");
  arm1.stop();
  if(arm::_imgData == NULL ){     // for the first time
    arm::_imgData = new ARM_DATA();
  }

  arm::_imgData = newBuf;
  debugln("New Data copied to arm buffers");
}
