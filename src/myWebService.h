#ifndef MY_WEBSERVICE_H
#define MY_WEBSERVICE_H

#include "main.h"
#include <WiFiUdp.h>
#include "NTPClient.h"
#include <ESPAsyncWebServer.h>

extern WiFiUDP udp;
extern NTPClient ntpClient;
extern AsyncWebServer server;

void setupService();
void sendMqttDataTask(void *arg); 
void sc_send(String message);

#endif