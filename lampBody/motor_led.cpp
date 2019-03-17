#include "myconfig.h"
#include "motor.h"

Motor motor(MOTOR_PWM_PIN, MOTOR_ENABLE_PIN);
PowerLed warmLed(WARM_LED_PIN, WARM_LED_PWM_CHANNEL);
PowerLed coldLed(COLD_LED_PIN, COLD_LED_PWM_CHANNEL);

/*
  Motor object initialization
  */
Motor::Motor(uint8_t pwmPin, uint8_t enablePin){
  debugln("Motor: Initializing Motor.");
  _pwmPin = pwmPin;
  _enablePin = enablePin;

  //pinMode(_pwmPin, OUTPUT);
  pinMode(_enablePin, OUTPUT);

  ledcSetup(MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ, 8);    // Initialize PWM for motor
  ledcAttachPin(MOTOR_PWM_PIN, MOTOR_PWM_CHANNEL);
}

// Function to enable motor
void Motor::enable(){
  debugln("Motor: Enabling motor.");
  digitalWrite(_enablePin, HIGH);
}

// Function to disable motor
void Motor::disable(){
  debugln("Motor: Disabling motor.");
  digitalWrite(_enablePin, LOW);
}

// Set the pulse of motor
void Motor::setPulse(int val){
  int pulse = map(val, 0, 100, 0, 255);
  debugln("Motor: Setting motor pulse");
  ledcWrite(MOTOR_PWM_CHANNEL, pulse);
}


/*
  PowerLed object intialization
  */
PowerLed::PowerLed(uint8_t pin, uint8_t channel){
  _channel = channel;
  ledcSetup(channel, LED_FREQ, 8);
  ledcAttachPin(pin, channel);
}

// Set the pulse of led
void PowerLed::setPulse(int val){
  int pulse = map(val, 0, 100, 0, 255);
  debugln("Motor: Setting Led pulse");
  ledcWrite(_channel, pulse);
}
