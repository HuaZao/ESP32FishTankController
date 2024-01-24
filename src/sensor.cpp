
#include "sensor.h"
#include <LittleFS.h>

int avgStoreTS = 0;    
int sampleStoreTS = 0; 
float TS = 0.0000;    
float TSlog = 0.0000;  

xSemaphoreHandle SensorSemaphoreHandle;
DataStruct_t SensorDataStruct; 

void printData(DataStruct_t data)
{
    ESP_LOGI("SensorData", "****************************************************");
    ESP_LOGI("DataStruct", "温度: %f °C", data.TemperatureData);
    ESP_LOGI("DataStruct", "鱼缸水位状态: %f °C", data.Water_FinshTank_In);
    ESP_LOGI("DataStruct", "废水水位状态: %f °C", data.Water_FinshTank_Out);
    ESP_LOGI("DataStruct", "海水水位状态: %f °C", data.Water_FinshTank_Salt);
    ESP_LOGI("DataStruct", "内部浮阀状态: %f °C", data.Water_FinshTank_Alert);
}


void readWaterSensor()
{
  SensorDataStruct.Water_FinshTank_In = digitalRead(fishTank_water_level_pin);
  SensorDataStruct.Water_FinshTank_Out = digitalRead(wastewater_level_pin);
  SensorDataStruct.Water_FinshTank_Salt = digitalRead(seawater_level_pin);
  SensorDataStruct.Water_FinshTank_Alert = digitalRead(backupwater_pin);
}

void readTemperatureSensor()
{
  if (avgStoreTS <= 5)
  { 
    TS = TS + analogRead(ntc_pin);
    sampleStoreTS++;
  }
  else
  {
    TS = TS / sampleStoreTS;
    TSlog = log(ntcResistance * (4095.00 / TS - 1.00));
    SensorDataStruct.TemperatureData = (1.0 / (1.009249522e-03 + 2.378405444e-04 * TSlog + 2.019202697e-07 * TSlog * TSlog * TSlog)) - 273.15;
    sampleStoreTS = 0;
    TS = 0;
  }
}

void gatherSensorTask(void *arg)
{
    vTaskDelay(3000 / portTICK_PERIOD_MS); 
    xSemaphoreGive(SensorSemaphoreHandle);
    for (;;)
    {
        if (xSemaphoreTake(SensorSemaphoreHandle, portMAX_DELAY) == pdTRUE)
        {
            readTemperatureSensor();
            readWaterSensor();
            printData(SensorDataStruct);
        }
        vTaskDelay(1000);
    }
}
