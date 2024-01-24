#ifndef MONITOR_H
#define MONITOR_H

#include "main.h"

extern String alertMessage;
extern String sendMessage;

// 监听水位,低于水位进行自动补水
void monitorFishTankWaterLevel();
// 监听换水
void monitorWaterChange();
// 进入换水
void enterWaterChangeMode();
// 退出换水
void exitWaterChangeMode();
// 监听故障
void monitorAlert();

void monitorMainTask(void *arg);;

#endif