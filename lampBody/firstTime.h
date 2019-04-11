#ifndef FIRST_TIME_H
#define FIRST_TIME_H

#define AUTOCONNECT_NAME     "POVLAMP Lampbody"
#define AUTOCONNECT_PASSWORD "novellaP"
#define CONFIG_FILENAME      "/myConfig.txt"

// Function definitions
void networkInit();
void saveConfigCallback();
void generateConfig();
void getConfig();
void saveConfig();

#endif
