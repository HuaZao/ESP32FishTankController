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

template <typename T>
T getJsonValueOrDefault(JsonObject &root, const char *key, const T &defaultValue)
{
    if (root.containsKey(key))
    {
        return root[key].as<T>();
    }
    return defaultValue;
}

void reloadConfig(JsonObject &root)
{
    StoreDataStruct.ledChannelCount = getJsonValueOrDefault(root, "ledChannelCount", StoreDataStruct.ledChannelCount);
    StoreDataStruct.isEnableAddWather = getJsonValueOrDefault(root, "isEnableAddWather", StoreDataStruct.isEnableAddWather);
    StoreDataStruct.addWatherTimeout = getJsonValueOrDefault(root, "addWatherTimeout", StoreDataStruct.addWatherTimeout);
    StoreDataStruct.isEnableWaterChanges = getJsonValueOrDefault(root, "isEnableWaterChanges", StoreDataStruct.isEnableWaterChanges);
    StoreDataStruct.isEnableLedSunTime = getJsonValueOrDefault(root, "isEnableSunTime", StoreDataStruct.isEnableLedSunTime);
    StoreDataStruct.fanControlMode = getJsonValueOrDefault(root, "fanControlMode", StoreDataStruct.fanControlMode);
    StoreDataStruct.fanPowerValue = getJsonValueOrDefault(root, "fanPowerValue", StoreDataStruct.fanPowerValue);
    StoreDataStruct.changesWaterTiming = getJsonValueOrDefault(root, "changesWaterTiming", StoreDataStruct.changesWaterTiming);
    StoreDataStruct.pushKey = getJsonValueOrDefault(root, "pushKey", StoreDataStruct.pushKey); 

    if (root.containsKey("rgbwuv"))
    {
        JsonArray rgbwuValues = root["rgbwuv"];
        for (int i = 0; i < LED_Channel && i < rgbwuValues.size(); i++)
        {
            StoreDataStruct.rgbwu[i] = rgbwuValues[i];
        }
    }

    if (root.containsKey("pwmValuesPerHour"))
    {
        JsonArray pwmHourValues = root["pwmValuesPerHour"];
        for (int i = 0; i < 24; i++)
        {
            String key = String(i);
            JsonObject value = pwmHourValues[i];
            for (int j = 0; j < LED_Channel; j++)
            {
                StoreDataStruct.pwmValuesPerHour[i][j] = value[key][j];
            }
        }
        // ESP_LOGI("ConfigDataStruct", "index 12 value => %d  | %d | %d | %d | %d",StoreDataStruct.pwmValuesPerHour[12][0]
        // ,StoreDataStruct.pwmValuesPerHour[12][1],StoreDataStruct.pwmValuesPerHour[12][2],StoreDataStruct.pwmValuesPerHour[12][3]
        // ,StoreDataStruct.pwmValuesPerHour[12][4]);
    }
    serializeJsonPretty(root, Serial);
    ESP_LOGI("ConfigDataStruct", "Reload Config Success");
}

DynamicJsonDocument mapConfig()
{
    DynamicJsonDocument doc(4096);
    doc["ledChannelCount"] = StoreDataStruct.ledChannelCount;
    doc["isEnableAddWather"] = StoreDataStruct.isEnableAddWather;
    doc["addWatherTimeout"] = StoreDataStruct.addWatherTimeout;
    doc["isEnableWaterChanges"] = StoreDataStruct.isEnableWaterChanges;
    doc["isEnableSunTime"] = StoreDataStruct.isEnableLedSunTime;
    doc["fanControlMode"] = StoreDataStruct.fanControlMode;
    doc["fanPowerValue"] = StoreDataStruct.fanPowerValue;
    doc["changesWaterTiming"] = StoreDataStruct.changesWaterTiming;
    doc["changesWaterCount"] = StoreDataStruct.changesWaterCount;
    doc["temperature"] = SensorDataStruct.temperatureData;
    doc["pushKey"] = StoreDataStruct.pushKey;

    JsonArray rgbuvArray = doc.createNestedArray("rgbwuv");
    for (int i = 0; i < LED_Channel; i++)
    {
        rgbuvArray.add(StoreDataStruct.rgbwu[i]);
    }
    JsonArray pwmValuesArray = doc.createNestedArray("pwmValuesPerHour");
    for (int i = 0; i < 24; i++)
    {
        String key = String(i);
        JsonObject hourObj = pwmValuesArray.createNestedObject();
        JsonArray valuesArray = hourObj.createNestedArray(key);
        for (int j = 0; j < LED_Channel; j++)
        {
            valuesArray.add(StoreDataStruct.pwmValuesPerHour[i][j]);
        }
    }
    return doc;
}

void updateConfig(Config_DataStruct_t data)
{
    File file = LittleFS.open("/config.json", FILE_WRITE);
    if (!file)
    {
        ESP_LOGE("ConfigDataStruct", "Failed to open file for reading");
        return;
    }
    DynamicJsonDocument doc = mapConfig();
    String jsonString;
    serializeJson(doc, jsonString);
    file.print(jsonString);
    // serializeJsonPretty(doc, Serial);
    doc.clear();
    file.close();
    ESP_LOGI("ConfigDataStruct", "Config Save Success!");
}

String storeJsonString(){
    DynamicJsonDocument doc = mapConfig();
    String jsonString;
    serializeJson(doc, jsonString);
    doc.clear();
    return jsonString;
}

String sensorJsonString(){
    DynamicJsonDocument doc(4096);
    doc["temperatureData"] = SensorDataStruct.temperatureData;
    doc["waterLevelIn"] = SensorDataStruct.waterFinshTankIn;
    doc["waterLevelOut"] = SensorDataStruct.waterFinshTankOut;
    doc["ntpValue"] = SensorDataStruct.ntpValue;
    doc["mos_b1"] = StoreDataStruct.changesWaterTiming;
    doc["mos_b2"] = StoreDataStruct.changesWaterCount;
    doc["mos_b3"] = SensorDataStruct.temperatureData;
    doc["voltage"] = SensorDataStruct.voltage;
    doc["current"] = SensorDataStruct.current;
   JsonArray ledPwmValues = doc.createNestedArray("ledPwmValues");
    for (int i = 0; i < LED_Channel; i++)
    {
        ledPwmValues.add(SensorDataStruct.ledPwmValues[i]);
    }
    String jsonString;
    serializeJson(doc, jsonString);
    doc.clear();
    return jsonString;
}