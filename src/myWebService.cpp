#include "myWebService.h"
#include "led.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include <WiFi.h>
#include <AsyncMqttClient.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ElegantOTA.h>
#include <WiFiClient.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>

static const char* TAG = "service_Log";   

extern xSemaphoreHandle SemaphoreHandle;

char mqtt_server[40];
char mqtt_port[6] = "1883";

AsyncMqttClient mqttClient;
AsyncWebServer server(80);
DNSServer dns;
WiFiUDP udp;
NTPClient ntpClient(udp, "ntp1.aliyun.com", 8 * 3600); // 中国时间

bool shouldSaveConfig = false;

void initWebService();

void connectToMqtt()
{
  ESP_LOGI(TAG,"Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  ESP_LOGI(TAG,"Connected to MQTT Session present:%d",sessionPresent);
  mqttClient.subscribe(MQTT_UPDATE_STORE_TOPIC, 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  ESP_LOGI(TAG,"Disconnected from MQTT.");
  if (WiFi.isConnected())
  {
    mqttClient.connect();
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  ESP_LOGI(TAG,"Subscribe packetId:%d  qos:%d",packetId,qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
  ESP_LOGI(TAG,"Unsubscribe packetId:%d",packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String payloadStr = String(payload);
  ESP_LOGI(TAG,"Publish received payload:%s",payloadStr);
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, payloadStr);
  JsonObject obj = doc.as<JsonObject>();
  if (strcmp(topic, MQTT_UPDATE_STORE_TOPIC) == 0)
  {
    JsonObject obj = doc.as<JsonObject>();
    reloadConfig(obj);
    updateConfig(StoreDataStruct);
  }
}

void onMqttPublish(uint16_t packetId)
{
  ESP_LOGI(TAG,"Publish packetId:%d",packetId);
}

void publishMessage(const char *topic, const String &payload)
{
  mqttClient.publish(topic, 1, false, payload.c_str());
  ESP_LOGI(TAG,"Published message to topic:%s  Payload:%s",String(topic),payload.c_str());
}


void indexPage()
{
  // server.serveStatic("/", LittleFS,"/").setDefaultFile("index.html");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
}

void jsonUpdataApi()
{
  server.on(
      "/api/updata", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, (char *)data, len); //deserialize the Body I get
  if (error) {
      request->send(500, "application/json", "{\"msg\":\"json error\"}"); 
  }else{
       JsonObject obj = doc.as<JsonObject>();
       reloadConfig(obj);
       updateConfig(StoreDataStruct);
       request->send(200, "application/json", "{\"msg\":\"success\"}"); 
  } });
}


void jsonGetApi()
{
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              String jsonString = storeJsonString();
  request->send(200, "application/json",jsonString); });

  server.on("/api/demo", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    startPreModel();
       request->send(200, "application/json", "{\"msg\":\"success\"}"); });
}

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void setupService()
{
  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.setDebugOutput(false);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setTimeout(180);
  AsyncWiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  AsyncWiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  if(!wifiManager.autoConnect("FishController_AP")) {
    ESP_LOGI(TAG,"failed to connect and hit timeout");
    delay(3000);
    // ESP.restart();
  }
  // 获取自定义的数据
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  if (shouldSaveConfig) {

  }
  ESP_LOGI(TAG,"WiFi connected IP:%s",WiFi.localIP().toString().c_str());
  ntpClient.begin();
  ntpClient.update();
  if(ntpClient.isTimeSet())
  {
      ESP_LOGI(TAG,"获取网络时间成功:%s",ntpClient.getFormattedTime());
  }
  // connectToMqtt();
}

void sendMqttDataTask(void *arg)
{
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  xSemaphoreGive(SemaphoreHandle);
  for (;;)
  {
    if (xSemaphoreTake(SemaphoreHandle, portMAX_DELAY) == pdTRUE)
    {
      if (mqttClient.connected())
      {
        publishMessage(MQTT_SEND_STORE_TOPIC, storeJsonString());
        vTaskDelay(3000 / portTICK_PERIOD_MS);
      }
    }
  }
}

void initWebService()
{    
  indexPage();
  jsonGetApi();
  jsonUpdataApi();
  ElegantOTA.begin(&server);
  server.begin();
  ESP_LOGI(TAG,"HTTP 服务已经启动~");
  sc_send("ESP鱼缸启动成功");
}

void sc_send(String message)
{
  //PDU23935TXd5sxyNlBZ9bvze0OSMTb9EKn8febs4P
  // String serverUrl = "https://api2.pushdeer.com/message/push?pushkey=PDU23935TXd5sxyNlBZ9bvze0OSMTb9EKn8febs4P" + StoreDataStruct.pushKey;
  // serverUrl += "&text="+ message + "&type=text";
  // HTTPClient http;
  // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // http.begin(serverUrl);
  // int httpCode = http.GET();
  // if (httpCode == HTTP_CODE_OK)
  // {
  //   sendMessage = alertMessage;
  // }
  // http.end();
}