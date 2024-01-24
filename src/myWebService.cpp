#include "myWebService.h"
#include "led.h"
// #include "sensor.h"

#include <WiFi.h>
#include <AsyncMqttClient.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ElegantOTA.h>
#include <WiFiClient.h>
#include <LittleFS.h>

xSemaphoreHandle WebServiceSemaphoreHandle;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

AsyncMqttClient mqttClient;
AsyncWebServer server(80);
void initWebService();

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    ntpClient.begin();
    ntpClient.update();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    connectToMqtt();
    initWebService();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    xTimerStop(mqttReconnectTimer, 0);
    xTimerStart(wifiReconnectTimer, 0);
    break;
  }
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttClient.subscribe(MQTT_UPDATE_STORE_TOPIC, 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  Serial.println("Publish received.");
  String payloadStr = String(payload);
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
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void publishMessage(const char *topic, const String &payload)
{
  // 使用 AsyncMqttClient 提供的 publish 函数发布消息
  // 第一个参数是主题，第二个参数是消息内容
  // 第三个参数是 QoS（服务质量），这里使用 QoS 1
  // 第四个参数是是否保留消息，这里不保留
  mqttClient.publish(topic, 1, false, payload.c_str());
  Serial.println("Published message to topic: " + String(topic) + ", Payload: " + payload);
}

void connectService()
{
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();
}

void indexPage()
{
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

void setupService()
{
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();
}

void sendMqttDataTask(void *arg)
{
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  xSemaphoreGive(WebServiceSemaphoreHandle);
  for (;;)
  {
    if (xSemaphoreTake(WebServiceSemaphoreHandle, portMAX_DELAY) == pdTRUE)
    {
      if (mqttClient.connected())
      {
        publishMessage(MQTT_SEND_STORE_TOPIC, storeJsonString());
      }
    }
    vTaskDelay(3000);
  }
}

void initWebService()
{
  indexPage();
  jsonGetApi();
  jsonUpdataApi();
  ElegantOTA.begin(&server);
  server.begin();
  sc_send("ESP鱼缸启动成功");
}

void sc_send(String message)
{
  //PDU23935TXd5sxyNlBZ9bvze0OSMTb9EKn8febs4P
  String serverUrl = "https://api2.pushdeer.com/message/push?pushkey=" + StoreDataStruct.pushKey;
  serverUrl += "&text="+ message + "&type=text";
  HTTPClient http;
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.begin(serverUrl);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    sendMessage = alertMessage;
  }
  http.end();
}