#ifndef COMM_H
#define COMM_H

#include "motor.h"

#define MQTT_BASE_TOPIC "novella/devices"
#define MQTT_PING_INTERVAL 10000  //  in milliseconds

// Settings object
class Settings{
  public:
    uint8_t motorSpeed = 50;  // 0 = manual, 1, automatic
    uint8_t coldLedBrightness = 50;  // value of brightness in percentage
    uint8_t warmLedBrightness = 50;  

    void setMotorSpeed(uint8_t speed){
      motorSpeed = speed;
      motor.setPulse( speed );
    }

    void setColdLed(uint8_t brightness){
      coldLedBrightness = brightness;
      coldLed.setPulse( brightness );
    }

    void setWarmLed(uint8_t brightness){
      warmLedBrightness = brightness;
      warmLed.setPulse( brightness );
    }
};

extern Settings mySettings;


void mqttInit(void);
void mqttLoop();
void mqttPing();
void mqttReply(char* msg);
void mqttCommand(char* msg);
void mqttReconnect();

#endif
