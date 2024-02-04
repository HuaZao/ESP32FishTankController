#include "store.h"
#include <FS.h>
#include <LittleFS.h>
#include "cJSON.h"


Config_DataStruct_t StoreDataStruct;

void initStore()
{
    if (FORMAT_FILESYSTEM)
    {
        LittleFS.format();
    }
    if (LittleFS.begin())
    {
        File file = LittleFS.open("/config.json", FILE_READ);
        String fileContent = "";
        if (!file || file.isDirectory())
        {
            ESP_LOGE("ConfigDataStruct", "Failed to open file for reading");
            return;
        }
        while (file.available())
        {
            char c = file.read();
            fileContent += c;
        }
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, fileContent);
        if (error)
        {
            ESP_LOGE("ConfigDataStruct", "Failed to parse JSON:%s", error.c_str());
            return;
        }
        JsonObject root = doc.as<JsonObject>();
        reloadConfig(root);
        root.clear();
        file.close();
    }
    else
    {
        ESP_LOGE("ConfigDataStruct", "开始格式化LittleFS......");
        // LittleFS.format();
        // ESP.restart();
    }
}

void reloadConfig(JsonObject &root)
{
    StoreDataStruct.isEnableAddWather = root["isEnableAddWather"];
    StoreDataStruct.addWatherTimeout= root["addWatherTimeout"];
    StoreDataStruct.isEnableWaterChanges = root["isEnableWaterChanges"];
    StoreDataStruct.isEnableLedSunTime = root["isEnableSunTime"];
    StoreDataStruct.temperatureControlMode = root["temperatureControlMode"];
    StoreDataStruct.changesWaterTiming = root["changesWaterTiming"];
    StoreDataStruct.pushKey = root["pushKey"].as<String>();
    JsonArray rgbwuValues = root["rgbwu"];
    for (int i = 0; i < LED_Channel; i++)
    {
        StoreDataStruct.rgbwu[i] = rgbwuValues[i];
    }
    JsonArray pwmHourVlaues = root["pwmValuesPerHour"];
    for (int i = 0; i < 24; i++)
    {
        String key = String(i);
        JsonObject value = pwmHourVlaues[i];
        for (int j = 0; j < LED_Channel; j++)
        {
            StoreDataStruct.pwmValuesPerHour[i][j] = value[key][j];
        }
    }
    ESP_LOGI("ConfigDataStruct", "Reload Config Success");
    // serializeJsonPretty(root, Serial);
}

void updateConfig(Config_DataStruct_t data)
{
    File file = LittleFS.open("/config.json", FILE_WRITE);
    if (!file)
    {
        ESP_LOGE("ConfigDataStruct", "Failed to open file for reading");
        return;
    }
    DynamicJsonDocument doc(4096);
    doc["isEnableAddWather"] = data.isEnableAddWather;
    doc["addWatherTimeout"] = data.addWatherTimeout;
    doc["isEnableWaterChanges"] = data.isEnableWaterChanges;
    doc["isEnableSunTime"] = data.isEnableLedSunTime;
    doc["temperatureControlMode"] = data.temperatureControlMode;
    doc["changesWaterTiming"] = data.changesWaterTiming;
    doc["pushKey"] = data.pushKey;

    JsonArray rgbuvArray = doc.createNestedArray("rgbuv");
    for (int i = 0; i < LED_Channel; i++)
    {
        rgbuvArray.add(data.rgbwu[i]);
    }
    JsonArray pwmValuesArray = doc.createNestedArray("pwmValuesPerHour");
    for (int i = 0; i < 24; i++)
    {
        String key = String(i);
        JsonObject hourObj = pwmValuesArray.createNestedObject();
        JsonArray valuesArray = hourObj.createNestedArray(key);
        for (int j = 0; j < LED_Channel; j++)
        {
            valuesArray.add(data.pwmValuesPerHour[i][j]);
        }
    }
    String jsonString;
    serializeJson(doc, jsonString);
    file.print(jsonString);
    // JsonObject root = doc.as<JsonObject>();
    // reloadConfig(root);
    serializeJsonPretty(doc, Serial);
    doc.clear();
    file.close();
    ESP_LOGI("ConfigDataStruct", "Config Save Success!");
}

String storeJsonString(){
    DynamicJsonDocument doc(4096);
    doc["isEnableAddWather"] = StoreDataStruct.isEnableAddWather;
    doc["addWatherTimeout"] = StoreDataStruct.addWatherTimeout;
    doc["isEnableWaterChanges"] = StoreDataStruct.isEnableWaterChanges;
    doc["isEnableSunTime"] = StoreDataStruct.isEnableLedSunTime;
    doc["temperatureControlMode"] = StoreDataStruct.temperatureControlMode;
    doc["changesWaterTiming"] = StoreDataStruct.changesWaterTiming;
    doc["changesWaterCount"] = StoreDataStruct.changesWaterCount;
    doc["temperature"] = SensorDataStruct.TemperatureData;
    doc["pushKey"] = StoreDataStruct.pushKey;


    JsonArray rgbuvArray = doc.createNestedArray("rgbuv");
    for (int i = 0; i < 5; i++)
    {
        rgbuvArray.add(StoreDataStruct.rgbwu[i]);
    }
    JsonArray pwmValuesArray = doc.createNestedArray("pwmValuesPerHour");
    for (int i = 0; i < 24; i++)
    {
        String key = String(i);
        JsonObject hourObj = pwmValuesArray.createNestedObject();
        JsonArray valuesArray = hourObj.createNestedArray(key);
        for (int j = 0; j < 5; j++)
        {
        valuesArray.add(StoreDataStruct.pwmValuesPerHour[i][j]);
        }
    }
    String jsonString;
    serializeJson(doc, jsonString);
    doc.clear();
    return jsonString;
}