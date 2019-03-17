#ifndef MYCONFIG_H
#define MYCONFIG_H

#include <Arduino.h>
#include <memory>
#include <cassert>
#include <stdio.h>


// Debug setup
#define myBaudRate    115200
#define debugSerial   Serial
#define debugln(c)    debugSerial.println(c)
#define debug(c)      debugSerial.print(c)

#define MOTOR_PWM_PIN         18
#define MOTOR_ENABLE_PIN      5
#define MOTOR_HALL_PIN        24

#define MOTOR_PWM_FREQ        2    // in Hz
#define MOTOR_PWM_CHANNEL     0

#define LED_FREQ              1000

#define WARM_LED_PIN          25
#define WARM_LED_PWM_CHANNEL  1

#define COLD_LED_PIN          26
#define COLD_LED_PWM_CHANNEL  2

#define CONSTANT_PWM_FREQUENCY  80000
#define CONSTANT_PWM_PIN        27
#define CONSTANT_PWM_CHANNEL    3
#define CONSTANT_PWM_ENABLE_PIN 14

// funtion definitions
void printArray(char* arr, int len);
void errorHandler(char* msg);

#endif
