#ifndef STORE_H
#define STORE_H

#include "main.h"

typedef struct
{
 int isEnableAddWather;         // 是否开启自动补水模式
 int isEnableWaterChanges;      // 开启定时换水
 int isEnableLedSunTime;        // LED运行模式 1-日出日落 0-手动控制
 int TemperatureControlMode;    // 散热默认 1-自动 2-手动
 int pwmValuesPerHour[24][5];   // LED每个通道对应24小时的值
 int rgbwu[5];                  // 手动模式的RGBWU
 int changesWaterTiming;        // 定时换水 0-关闭 1~7对应星期一 ~ 星期天
 int changesWaterCount;         // 换水量5L
 String pushKey;
} Config_DataStruct_t;

extern Config_DataStruct_t StoreDataStruct; 

void initStore();
void reloadConfig(JsonObject &root);
void updateConfig(Config_DataStruct_t data);
String storeJsonString();
#endif