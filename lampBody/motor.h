#ifndef MOTOR_H
#define MOTOR_H

class Motor{
    uint8_t _pwmPin;
    uint8_t _enablePin;
    bool isEnabled;
  public:
    Motor(uint8_t pwmPin, uint8_t enablePin);
    void enable();
    void disable();
    void setPulse(int pulse);
};

class PowerLed{
  uint8_t _pin;
  uint8_t _channel;
  public:
    PowerLed(uint8_t pin, uint8_t channel);
    void setPulse(int pulse);
};

extern Motor motor;
extern PowerLed warmLed;
extern PowerLed coldLed;

#endif
