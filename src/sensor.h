#ifndef __SENSOR_H
#define __SENSOR_H
#include "main.h"

typedef struct
{
    int waterFinshTankIn;          // 鱼缸水位
    int waterFinshTankOut;         // 废水水位
    int waterFinshTankSalt;        // 海水水位
    int waterFinshTankAlert;       // 鱼缸内部物理传感器
    double temperatureData;         // 水温  
    int ledPwmValues[5];             //当前pwm
    char ntpValue[10];                 //ntp时钟
    int mos_b1;                      //mos1当前pwm
    int mos_b2;                      //mos2当前pwm
    int mos_b3;                      //mos3当前pwm
    double voltage;
    double current;
} DataStruct_t;
extern DataStruct_t SensorDataStruct;       // 鱼缸传感器结构体

void gatherSensorTask(void *arg);
#endif