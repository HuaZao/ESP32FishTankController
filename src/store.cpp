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
        size_t fileSize = file.size();
        char *fileContent = (char *)malloc(fileSize + 1);
        size_t bytesRead = file.readBytes(fileContent, fileSize);
        fileContent[bytesRead] = '\0';
        // ESP_LOGD("ConfigDataStruct","====>%s",fileContent);
        reloadConfig(fileContent);
        file.close();
        free(fileContent);
    }
    else
    {
        ESP_LOGE("ConfigDataStruct", "请初始化LittleFS......");
    }
}

void parseJsonToStruct(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root != NULL) {
        cJSON *addWater = cJSON_GetObjectItem(root, "addWather");
        if (addWater != NULL) {
            ESP_LOGE("ConfigDataStruct", "更新addWather");
            StoreDataStruct.addWater.isEnable = cJSON_GetObjectItem(addWater, "isEnable")->valueint;
            StoreDataStruct.addWater.timeout = cJSON_GetObjectItem(addWater, "timeout")->valueint;
        }

        cJSON *waterChanges = cJSON_GetObjectItem(root, "waterChanges");
        if (waterChanges != NULL) {
            ESP_LOGE("ConfigDataStruct", "更新waterChanges");
            StoreDataStruct.waterChanges.isEnable = cJSON_GetObjectItem(waterChanges, "isEnable")->valueint;
            StoreDataStruct.waterChanges.timeout = cJSON_GetObjectItem(waterChanges, "timeout")->valueint;
            StoreDataStruct.waterChanges.count = cJSON_GetObjectItem(waterChanges, "count")->valueint;

            cJSON *clock = cJSON_GetObjectItem(waterChanges, "clock");
            if (clock != NULL) {
                StoreDataStruct.waterChanges.clock.hour = cJSON_GetObjectItem(clock, "hour")->valueint;
                StoreDataStruct.waterChanges.clock.minute = cJSON_GetObjectItem(clock, "minute")->valueint;
                StoreDataStruct.waterChanges.clock.week = cJSON_GetObjectItem(clock, "week")->valueint;
            }
        }

        cJSON *wifi = cJSON_GetObjectItem(root, "wifi");
        if (wifi != NULL) {
            ESP_LOGE("ConfigDataStruct", "更新Wi-Fi");
            strcpy(StoreDataStruct.wifi.ssid, cJSON_GetObjectItem(wifi, "ssid")->valuestring);
            strcpy(StoreDataStruct.wifi.psw, cJSON_GetObjectItem(wifi, "psw")->valuestring);
        }

        cJSON *mqtt = cJSON_GetObjectItem(root, "mqtt");
        if (mqtt != NULL) {
            ESP_LOGE("ConfigDataStruct", "更新mqtt");
            StoreDataStruct.mqtt.port = cJSON_GetObjectItem(mqtt, "port")->valueint;
            strcpy(StoreDataStruct.mqtt.ip, cJSON_GetObjectItem(mqtt, "ip")->valuestring);
        }

        cJSON *led = cJSON_GetObjectItem(root, "led");
        if (led != NULL) {
            ESP_LOGE("ConfigDataStruct", "更新led");
            StoreDataStruct.led.ledChannelCount = cJSON_GetObjectItem(led, "ledChannelCount")->valueint;
            StoreDataStruct.led.isEnableSunTime = cJSON_GetObjectItem(led, "isEnableSunTime")->valueint;
            StoreDataStruct.led.fanPowerValue = cJSON_GetObjectItem(led, "fanPowerValue")->valueint;
            StoreDataStruct.led.fanControlMode = cJSON_GetObjectItem(led, "fanControlMode")->valueint;
            StoreDataStruct.led.fanStartUpValue = cJSON_GetObjectItem(led, "fanStartUpValue")->valueint;

            //解析pwmValuesPerHour数组
            cJSON *pwmArray = cJSON_GetObjectItem(led, "pwmValuesPerHour");
            for (int i = 0; i < 24; i++)
            {
                char key[10];
                snprintf(key, sizeof(key), "%d", i);
                cJSON *hoursObj = cJSON_GetArrayItem(pwmArray, i);
                cJSON *current_item = hoursObj->child;
                while (current_item != NULL) {
                    cJSON *object = cJSON_GetObjectItemCaseSensitive(hoursObj, current_item->string);
                    for (int j = 0; j < LED_Channel; j++)
                    {
                        cJSON *item = cJSON_GetArrayItem(object, j);
                        StoreDataStruct.led.pwmValuesPerHour[i][j] = item->valueint;
                    }
                    current_item = current_item->next;
                }
            }

            // 解析rgbwuv数组
            cJSON *rgbwuvArray = cJSON_GetObjectItem(led, "rgbwuv");
            for (int i = 0; i < LED_Channel; i++)
            {
                cJSON *item = cJSON_GetArrayItem(rgbwuvArray, i);
                StoreDataStruct.led.rgbwuv[i] = item->valueint;
            }
        }
        cJSON *pushKey = cJSON_GetObjectItem(root, "pushKey");
        if(pushKey != NULL){
            ESP_LOGE("ConfigDataStruct", "更新pushkey");
            strcpy(StoreDataStruct.pushKey, pushKey->valuestring);
        }
        ESP_LOGI("ConfigDataStruct", "%s",cJSON_Print(root));
        cJSON_Delete(root);
    }
}

void reloadConfig(const char *json_str){
    ESP_LOGI("ConfigDataStruct", "收到配置 %s",json_str);
    parseJsonToStruct(json_str);
}

char *structToJson(Config_DataStruct_t config) {
    cJSON *root = cJSON_CreateObject();
    cJSON *addWater = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "addWather", addWater);
    cJSON_AddNumberToObject(addWater, "isEnable", config.addWater.isEnable);
    cJSON_AddNumberToObject(addWater, "timeout", config.addWater.timeout);

    cJSON *waterChanges = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "waterChanges", waterChanges);
    cJSON_AddNumberToObject(waterChanges, "isEnable", config.waterChanges.isEnable);
    cJSON_AddNumberToObject(waterChanges, "timeout", config.waterChanges.timeout);
    cJSON_AddNumberToObject(waterChanges, "count", config.waterChanges.count);
    cJSON *clock = cJSON_CreateObject();
    cJSON_AddItemToObject(waterChanges, "clock", clock);
    cJSON_AddNumberToObject(clock, "hour", config.waterChanges.clock.hour);
    cJSON_AddNumberToObject(clock, "minute", config.waterChanges.clock.minute);
    cJSON_AddNumberToObject(clock, "week", config.waterChanges.clock.week);

    cJSON *wifi = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "wifi", wifi);
    cJSON_AddStringToObject(wifi, "ssid", config.wifi.ssid);
    cJSON_AddStringToObject(wifi, "psw", config.wifi.psw);

    cJSON *mqtt = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "mqtt", mqtt);
    cJSON_AddNumberToObject(mqtt, "port", config.mqtt.port);
    cJSON_AddStringToObject(mqtt, "ip", config.mqtt.ip);

    cJSON *led = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "led", led);
    cJSON_AddNumberToObject(led, "ledChannelCount", config.led.ledChannelCount);
    cJSON_AddNumberToObject(led, "isEnableSunTime", config.led.isEnableSunTime);
    cJSON_AddNumberToObject(led, "fanPowerValue", config.led.fanPowerValue);
    cJSON_AddNumberToObject(led, "fanControlMode", config.led.fanControlMode);
    cJSON_AddNumberToObject(led, "fanStartUpValue", config.led.fanStartUpValue);
    
    cJSON *pwmValuesPerHourArray = cJSON_CreateArray();
     for (int i = 0; i < 24; ++i) {
        cJSON *object = cJSON_CreateObject();
        cJSON *keyArray = cJSON_CreateIntArray(config.led.pwmValuesPerHour[i], LED_Channel);
        char key[10];
        snprintf(key, sizeof(key), "%d", i);
        cJSON_AddItemToObject(object, key, keyArray);
        cJSON_AddItemToArray(pwmValuesPerHourArray, object);
    }
    cJSON_AddItemToObject(led, "pwmValuesPerHour", pwmValuesPerHourArray);

    cJSON *rgbwuv = cJSON_CreateArray();
    cJSON_AddItemToObject(led, "rgbwuv", rgbwuv);
    for (int i = 0; i < LED_Channel; i++) {
        cJSON_AddItemToArray(rgbwuv, cJSON_CreateNumber(config.led.rgbwuv[i]));
    }
    cJSON_AddStringToObject(root, "pushKey", config.pushKey);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

void updateConfig()
{
    File file = LittleFS.open("/config.json", FILE_WRITE);
    if (!file)
    {
        ESP_LOGE("ConfigDataStruct", "Failed to open file for reading");
        return;
    }
    char *jsonString = structToJson(StoreDataStruct);
    file.print(jsonString);
    file.close();
    ESP_LOGI("ConfigDataStruct", "Config Save Success!");
}

char *storeJsonString(){
    return structToJson(StoreDataStruct);
}

char *sensorJsonString(){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperatureData", SensorDataStruct.temperatureData);
    cJSON_AddNumberToObject(root, "waterLevelIn", SensorDataStruct.waterFinshTankIn);
    cJSON_AddNumberToObject(root, "waterLevelOut", SensorDataStruct.waterFinshTankOut);
    cJSON_AddStringToObject(root, "ntpValue", SensorDataStruct.ntpValue);
    cJSON_AddNumberToObject(root, "mos_b1", SensorDataStruct.mos_b1);
    cJSON_AddNumberToObject(root, "mos_b2", SensorDataStruct.mos_b2);
    cJSON_AddNumberToObject(root, "mos_b3", SensorDataStruct.mos_b3);
    cJSON_AddNumberToObject(root, "voltage", SensorDataStruct.voltage);
    cJSON_AddNumberToObject(root, "current", SensorDataStruct.current);
    // ledPwmValues
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}