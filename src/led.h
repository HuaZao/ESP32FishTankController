#ifndef LED_H
#define LED_H

#include "main.h"
#include <WiFiUdp.h>
#include "NTPClient.h"

extern TaskHandle_t LedTask;

extern WiFiUDP udp;
extern NTPClient ntpClient;

void setPWMPercentage(int pwmChannel, int percentage);
void core1LedPwmTask(void *parameter);
void startPreModel();

#endif