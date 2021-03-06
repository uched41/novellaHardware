#ifndef MYCONFIG_H
#define MYCONFIG_H

#include <Arduino.h>
#include <memory>
#include <cassert>
#include <stdio.h>
#include <SPI.h>

// Overall Global Variables
extern bool canPlay;
extern volatile bool started_1;
extern volatile bool started_2;

// constants
#define LED_COUNT       88
#define IMAGE_LENGTH    150
#define IMAGE_HEIGHT    LED_COUNT
#define MAX_BRIGHTNESS  20
#define sColumn     IMAGE_HEIGHT*3            // size of one column array

// Pin configurations
#define LDR_PIN         4     // anolog pin for light sensor
#define SLAVE1_TX       2     // pins for serial communication with slave
#define SLAVE1_RX       15

#define DATA_PIN1       23    // SPI1 pins was13
#define MISO_PIN1       19   // was 33
#define CLOCK_PIN1      18    // was 14
#define SS_PIN1         22    // was 32

//#define DATA_PIN2       23    // SPI2 pins
//#define CLOCK_PIN2      18
//#define MISO_PIN2       19
//#define SS_PIN2         22

#define RED_LED         27    // leds for status
#define GREEN_LED       22
#define BLUE_LED        21

#define hallSensor1     25    // Trigger pin for arm1, To be connected to HALL SENSOR 1
#define hallSensor2     26    // Trigger pin for arm2, To be connected to HALL SENSOR 2

#define RESET_SWITCH    5

// Debug setup
#define myBaudRate    115200
#define debugSerial   Serial
#define debugln(c)    debugSerial.println(c)
#define debug(c)      debugSerial.print(c)

extern portMUX_TYPE serMutex;

// funtion definitions
void printArray(char* arr, int len);
void errorHandler(char* msg);

#endif

