
#include "sensor.h"
#include <LittleFS.h>
#include <INA226.h>
int avgStoreTS = 0;
int sampleStoreTS = 0;
float TS = 0.0000;
float TSlog = 0.0000;

extern xSemaphoreHandle SemaphoreHandle;
DataStruct_t SensorDataStruct;
INA226 ina;

void printData(DataStruct_t data)
{
  ESP_LOGI("SensorData", "****************************************************");
  ESP_LOGI("DataStruct", "温度: %f °C", data.temperatureData);
  ESP_LOGI("DataStruct", "鱼缸水位状态: %d", data.waterFinshTankIn);
  ESP_LOGI("DataStruct", "废水水位状态: %d", data.waterFinshTankOut);
  ESP_LOGI("DataStruct", "海水水位状态: %d", data.waterFinshTankSalt);
  ESP_LOGI("DataStruct", "内部浮阀状态: %d", data.waterFinshTankAlert);
}

void readWaterSensor()
{
  SensorDataStruct.waterFinshTankIn = digitalRead(fishTank_water_level_pin);
  SensorDataStruct.waterFinshTankOut = digitalRead(wastewater_level_pin);
  SensorDataStruct.waterFinshTankSalt = digitalRead(seawater_level_pin);
  SensorDataStruct.waterFinshTankAlert = digitalRead(backupwater_pin);
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
    SensorDataStruct.temperatureData = (1.0 / (1.009249522e-03 + 2.378405444e-04 * TSlog + 2.019202697e-07 * TSlog * TSlog * TSlog)) - 273.15;
    sampleStoreTS = 0;
    TS = 0;
  }
}

void gatherSensorTask(void *arg)
{
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  xSemaphoreGive(SemaphoreHandle);
  for (;;)
  {
    if (xSemaphoreTake(SemaphoreHandle, portMAX_DELAY) == pdTRUE)
    {
      readTemperatureSensor();
      readWaterSensor();
      printData(SensorDataStruct);
      vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
  }
}
