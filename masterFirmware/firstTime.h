#ifndef FIRST_TIME_H
#define FIRST_TIME_H

#define AUTOCONNECT_NAME     "Novella Makers"
#define AUTOCONNECT_PASSWORD "novellaP"
#define CONFIG_FILENAME      "/myConfig.txt"

// Default device settings for config files
#define DEFAULT_BRIGHTNESS_CONTROL_MODE     0       // 0=>"MANUAL", 1->automatic
#define DEFAULT_BRIGHTNESS_VALUE            128
#define DEFAULT_IMAGE                       "defaultImg.jpg"    // default image to be displayed , when no image in memory

// Function definitions
void networkInit();
void saveConfigCallback();
void generateConfig();
void getConfig();
void saveConfig();

#endif
