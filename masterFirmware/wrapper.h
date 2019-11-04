#ifndef _WRAPPER_H
#define _WRAPPER_H
#include "arm.h"

void IRAM_ATTR isr1();
void setupIsr();
void Arm1Task(void *pvparameters);
bool createTask(arm* myarm);
void resetArms();

void disableIsr(void);
void enableIsr(void);

#endif

