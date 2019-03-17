#ifndef ARM_H
#define ARM_H

#include "led_driver.h"

class arm{

  public:
    ledDriver *leds;
    TaskHandle_t _mytask;            
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    
    volatile uint8_t _colPointer = 0;        // variable that points to the current column being displayed
    volatile bool dir = false;               // variable that determines direction
    
    uint8_t arm_no=0;
    uint8_t _core;                           // core on which this arm will run its display function
    int _len;                                // length of this arm, (no of leds)
    volatile bool taskCreated;
    volatile bool isRunning;
    static uint8_t** _imgData;        // pointer to where the data of columns is located
    static int _noColumns;
  
    void showColumn(uint8_t* buf);    // Function that will show a column
    void showImage();
    void IRAM_ATTR execIsr();         // ISR to respond to gpio triggers from master

    

    arm(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin, int len, uint8_t armno);
    void setCore(uint8_t core);
    void stop();
    void resume();
    bool isTaskCreated();
};

void setArmData(uint8_t** buf, int newlen);

// declare arms
extern arm arm1;
extern arm arm2;


#endif
