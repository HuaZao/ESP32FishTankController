#ifndef LED_H
#define LED_H

#include "main.h"


extern TaskHandle_t LedTask;

void setPWMPercentage(int pwmChannel, int percentage);
void core1LedPwmTask(void *parameter);
void startPreModel();

#endif