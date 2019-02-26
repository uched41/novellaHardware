#ifndef ARM_H
#define ARM_H

#include "led_driver.h"

#define delayBetweenColumns 1000    // in microseconds
#define delayBetweenImages  1000  // in microseconds

class arm{

  public:
     uint8_t arm_no=0;
    ledDriver *leds;
    volatile uint8_t _colPointer = 0;          // variable that points to the current column being displayed
    uint8_t** _imgData = NULL;        // pointer to where the data of columns is located
    volatile bool taskCreated;
    volatile bool isRunning;
    bool canRun = false;

    TaskHandle_t* _mytask;              // Task Handle
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    void showColumn(uint8_t* buf);    // Function that will show a column
    void showImage();
    void IRAM_ATTR execIsr();      // ISR to respond to gpio triggers from master

    uint8_t _core;                    // core on which this arm will run its display function
    int _len;                         // length of this arm, (no of leds)

    arm(SPIClass myspi, uint8_t clkPin, uint8_t misoPin, uint8_t dataPin, uint8_t ssPin, int len);
    void setCore(uint8_t core);
    void setImage(uint8_t** imgData);
    void stop();
    void resume();
    bool isTaskCreated();
};

// declare arms
extern arm arm1;
extern arm arm2;


#endif
