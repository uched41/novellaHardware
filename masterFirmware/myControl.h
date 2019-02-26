#ifndef CONTROL_H
#define CONTROL_H

#define MQTT_BASE_TOPIC "novella/devices"
#define MQTT_PING_INTERVAL 10000  //  in milliseconds

extern uint8_t brightnessMode;
extern uint8_t brightnessVal;
extern uint8_t **dataStore;

#define STATUS_FLASH_INTERVAL   200   // interval in millis when flashing status led

class StatusLed{
  uint8_t r, g, b;
  public:
    StatusLed(uint8_t _r, uint8_t _g, uint8_t _b){
      r=_r; g=_g; b=_b;
      pinMode(r, OUTPUT);
      pinMode(g, OUTPUT);
      pinMode(b, OUTPUT);
    }

    void flashRed() {
      digitalWrite(r, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(r, LOW);
      delay(STATUS_FLASH_INTERVAL);
    }

    void flashGreen() {
      digitalWrite(g, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(g, LOW);
      delay(STATUS_FLASH_INTERVAL);
    }

    void flashBlue()  {
      digitalWrite(b, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(b, LOW);
      delay(STATUS_FLASH_INTERVAL);
    }

    void flashYellow(){
      digitalWrite(r, HIGH);
      digitalWrite(g, HIGH);
      delay(STATUS_FLASH_INTERVAL);
      digitalWrite(r, LOW);
      digitalWrite(g, LOW);
    }

    void off(){   // function to put off all eds
      digitalWrite(b, LOW);
      digitalWrite(g, LOW);
      digitalWrite(r, LOW);
    }

};

extern StatusLed statusLed;

void mqttInit(void);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
void mqttLoop();
void mqttReply(char* msg);
void mqttPing();
void splitData(char* filename);

#endif
