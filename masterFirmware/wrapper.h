#ifndef _WRAPPER_H
#define _WRAPPER_H
#include "arm.h"

void isr1();
void setupIsr();
void Arm1Task(void *pvparameters);
bool createTask(arm* myarm);
void resetArms();

#endif

