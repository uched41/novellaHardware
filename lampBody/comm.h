#ifndef COMM_H
#define COMM_H

#define MQTT_BASE_TOPIC "novella/devices"
#define MQTT_PING_INTERVAL 10000  //  in milliseconds

void mqttInit(void);
void mqttLoop();
void mqttPing();
void mqttReply(char* msg);


#endif
