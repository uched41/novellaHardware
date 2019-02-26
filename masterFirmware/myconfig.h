#ifndef MYCONFIG_H
#define MYCONFIG_H

#include <Arduino.h>
#include <memory>
#include <cassert>
#include <stdio.h>
#include <SPI.h>

// Overall Global Variables
extern bool canPlay;

// constants
#define LED_COUNT       88
#define IMAGE_LENGTH    150
#define IMAGE_HEIGHT    LED_COUNT
#define MAX_BRIGHTNESS  20

// Pin configurations
//#define triggerPin1    25
//#define triggerPin2    33
#define LDR_PIN         4     // anolog pin for light sensor
#define SLAVE1_TX       2     // pins for serial communication with slave
#define SLAVE1_RX       15

#define DATA_PIN1       13    // SPI1 pins
#define MISO_PIN1       33
#define CLOCK_PIN1      14
#define SS_PIN1         32

#define DATA_PIN2       23    // SPI2 pins
#define CLOCK_PIN2      18
#define MISO_PIN2       19
#define SS_PIN2         22

#define RED_LED         27    // leds for status
#define GREEN_LED       12
#define BLUE_LED        21

#define hallSensor1     25    // Trigger pin for arm1, To be connected to HALL SENSOR 1
#define hallSensor2     26    // Trigger pin for arm2, To be connected to HALL SENSOR 2
//#define SLAVE2_TX      21
//#define SLAVE2_RX      22

// Debug setup
#define myBaudRate    115200
#define debugSerial   Serial
#define debugln(c)    debugSerial.println(c)
#define debug(c)      debugSerial.print(c)


// funtion definitions
void printArray(char* arr, int len);
void errorHandler(char* msg);

#endif
