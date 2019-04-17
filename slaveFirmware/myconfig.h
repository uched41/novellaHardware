#ifndef MYCONFIG_H
#define MYCONFIG_H

#include <Arduino.h>
#include <memory>
#include <cassert>
#include <stdio.h>
#include <SPI.h>

#define LED_COUNT   88
#define IMAGE_LENGTH 150
#define IMAGE_HEIGHT LED_COUNT
#define sColumn   IMAGE_HEIGHT*3           // size of one column array

extern volatile bool started_1;
extern volatile bool started_2;

// SPI1 pins
#define DATA_PIN1   23  // was 13
#define MISO_PIN1   19  // was 12
#define CLOCK_PIN1  18 // was 14
#define SS_PIN1     5  // was 27

// SPI2 pins
//#define DATA_PIN2   23
//#define CLOCK_PIN2  18
//#define MISO_PIN2   19
//#define SS_PIN2     5

#define hallSensor1  26    // was 25 Trigger pin for arm1, To be connected to HALL SENSOR 1
//#define hallSensor2  26    // Trigger pin for arm2, To be connected to HALL SENSOR 2

// Serial communication pins for slave1
#define SLAVE1_TX 2
#define SLAVE1_RX 15
#define myBaudRate 115200

#define debugSerial Serial
#define debugln(c)    debugSerial.println(c)
#define debug(c)      debugSerial.print(c)

extern bool canPlay;

#endif
