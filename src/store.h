#ifndef STORE_H
#define STORE_H

#include "main.h"

// typedef struct
// {
//  int ledChannelCount;           // 设置的通道数
//  int isEnableAddWather;         // 是否开启自动补水模式
//  int addWatherTimeout;          // 补水超时时间
//  int isEnableWaterChanges;      // 开启定时换水
//  int isEnableLedSunTime;        // LED运行模式 1-日出日落 0-手动控制
//  int fanControlMode;    // 散热默认 0-自动 1-手动
//  int fanPowerValue;             // 风扇功率
//  int pwmValuesPerHour[24][5];   // LED每个通道对应24小时的值
//  int rgbwu[5];                  // 手动模式的RGBWU
//  int changesWaterTiming;        // 定时换水 0-关闭 1~7对应星期一 ~ 星期天
//  int changesWaterCount;         // 换水量5L
//  String pushKey;
// } Config_DataStruct_t;

// 定义水更换的时钟
typedef struct {
    int hour;
    int minute;
    int week;
} Clock;

// 定义 "addWather" 配置
typedef struct {
    int isEnable;
    int timeout;
} AddWaterConfig;

// 定义 "waterChanges" 配置
typedef struct {
    int isEnable;
    int timeout;
    int count;
    Clock clock;
} WaterChangesConfig;

// 定义 LED 配置
typedef struct {
    int ledChannelCount;
    int isEnableSunTime;
    int fanPowerValue;
    int fanControlMode;
    int fanStartUpValue;
    int pwmValuesPerHour[24][LED_Channel];
    int rgbwuv[LED_Channel];
} LedConfig;

// 定义 WiFi 配置
typedef struct {
    char ssid[64];
    char psw[64];
} WifiConfig;

// 定义 MQTT 配置
typedef struct {
    int port;
    char ip[40];
} MqttConfig;

// 定义主配置
typedef struct {
    AddWaterConfig addWater;
    WaterChangesConfig waterChanges;
    WifiConfig wifi;
    MqttConfig mqtt;
    LedConfig led;
    char pushKey[64];
} Config_DataStruct_t;

extern Config_DataStruct_t StoreDataStruct; 

void initStore();
void updateConfig();
void reloadConfig(const char *json_str);
char * storeJsonString();
char * sensorJsonString();
#endif