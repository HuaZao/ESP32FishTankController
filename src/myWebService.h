#ifndef MY_WEBSERVICE_H
#define MY_WEBSERVICE_H

#include "main.h"

void setupService();
void sendMqttDataTask(void *arg); 
void sc_send(String message);

#endif