#ifndef _WRAPPER_H
#define _WRAPPER_H
#include "arm.h"

void isr1();
void isr2();
void setupIsr();
void Arm1Task(void *pvparameters);
void Arm2Task(void *pvparameters);
bool createTask(arm* myarm);
void resetArms();

#endif
