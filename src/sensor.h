#ifndef __SENSOR_H
#define __SENSOR_H
#include "main.h"

typedef struct
{
    int Water_FinshTank_In;          // 鱼缸水位
    int Water_FinshTank_Out;         // 废水水位
    int Water_FinshTank_Salt;        // 海水水位
    int Water_FinshTank_Alert;       // 鱼缸内部物理传感器
    double  TemperatureData;         // 水温  
} DataStruct_t;
extern DataStruct_t SensorDataStruct;       // 鱼缸传感器结构体

void gatherSensorTask(void *arg);
#endif