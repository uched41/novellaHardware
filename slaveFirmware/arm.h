#ifndef ARM_H
#define ARM_H

#include "led_driver.h"
#include "comm.h"

class arm{

  public:
    ledDriver *leds;
    TaskHandle_t _mytask;            
    SemaphoreHandle_t  mSema = NULL;
    BaseType_t xHigherPriorityTaskWoken;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    
    volatile int _colPointer = 0;        // variable that points to the current column being displayed
    volatile int _imgPointer = 0;
    volatile bool dir = false;               // variable that determines direction
    
    uint8_t arm_no=0;
    uint8_t _core;                           // core on which this arm will run its display function
    int _len;                                // length of this arm, (no of leds)
    volatile bool taskCreated;
    volatile bool isRunning;
    static ARM_DATA* _imgData;        // pointer to where the data of columns is located
  
    void showColumn(uint8_t* buf);    // Function that will show a column
    void showImage();
    void IRAM_ATTR execIsr();         // ISR to respond to gpio triggers from master

    

    arm(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin, int len, uint8_t armno);
    void setCore(uint8_t core);
    void stop();
    void resume();
    bool isTaskCreated();
};

void setArmData(const char* file);
void setArmData(ARM_DATA* newBuf);

// declare arms
extern arm arm1;


#endif

