#include "led_driver.h"
#include "myconfig.h"
#include "comm.h"
#include "arm.h"
#include "wrapper.h"
#include "esp_task_wdt.h"
/*
 * NOTE: Please edit ESP32 core to increase I2C Buffer Size
 */
arm arm1(HSPI, CLOCK_PIN1, MISO_PIN1, DATA_PIN1, SS_PIN1, IMAGE_HEIGHT, 1);
arm arm2(VSPI, CLOCK_PIN2, MISO_PIN2, DATA_PIN2, SS_PIN2, IMAGE_HEIGHT, 2);

void setup()
{
  debugSerial.begin(115200);

  // initialize arms
  arm1.setCore(0);
  arm2.setCore(0);
  setupIsr();

  initComm();
  Serial.println("Initialization complete");
}


void loop(){
  commParser();
  delay(5);
  //esp_task_wdt_reset();
}


