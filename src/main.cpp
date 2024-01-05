#include <Arduino.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include "FS.h"
#include <ArduinoJson.h>
// 仅第一次需要初始化文件系统
#define FORMAT_FILESYSTEM false

TaskHandle_t LedTask;

AsyncWebServer server(80);

// NTP客户端配置
WiFiUDP udp;
NTPClient ntpClient(udp, "pool.ntp.org", 8 * 3600);

const float fixedResistor = 10000.0;  // 串联的固定电阻的阻值
const float referenceTemperature = 25.0;  // 参考温度（NTC的阻值在该温度下被标定）

// 引脚
const int motor_bi_pin = 21;             // 电机后退
const int motor_fi_pin = 19;             // 电机前进
const int fishTank_water_level_pin = 22; // 鱼缸水位
const int wastewater_level_pin = 23;     // 废水水位
const int seawater_level_pin = 18;       // 海水水位
const int ntc_pin = 34;                  // NTC
const int backupwater_pin = 16;          // 物理备用水位
const int solenoid_valve_pin = 4;        // 电磁阀
const int alert_pin = 5;                 // 故障指示灯
const int button_pin = 17;               // 故障指示灯

// MOS引脚
const int mos_b1_pin = 13;
const int mos_b2_pin = 12;
const int mos_b3_pin = 14;
const int mos_u_pin = 27; // U
const int mos_b_pin = 26; // B
const int mos_g_pin = 25; // G
const int mos_r_pin = 33; // R
const int mos_w_pin = 32; // W

// 备用引脚
const int backup_pin = 35;

// PWM频率
const int pwm_freq = 68125;
const int pwm_freq_resolution = 10;

unsigned long pumpStartTime = 0;                         // 记录水泵启动的时间
const unsigned long pumpTimeout = 30000;                 // 补水超时时间，单位：毫秒
const unsigned long waterChangePumpTimeout = 1000 * 120; // 换水泵超时时间，单位：毫秒
bool waterChangeMode = false;                            // 换水模式标志
bool isAddWater = false;                                 // 是否在补水中
unsigned long buttonPressStartTime = 0;                  // 记录按钮按下的开始时间
const int longPressDuration = 3000;                      // 长按时间，单位：毫秒
bool isAlert = 0;                                        // 是否有故障
String alertMessage = "";
String sendMessage = "";
float temperature = 0.00;

// 全局配置
int isEnableAddWather = 1;         // 是否开启自动补水模式
int isEnableWaterChanges = 1;      // 开启定时换水
int isEnableLedSunTime = 1;        // LED运行模式 1-日出日落 0-手动控制
int TemperatureControlMode = 1;    // 散热默认 1-自动 2-手动
int pwmValuesPerHour[24][5] = {0}; // LED每个通道对应24小时的值
int rgbwu[5] = {0};                // 手动模式的RGBWU
int changesWaterTiming = 0;        // 定时换水 0-关闭 1~7对应星期一 ~ 星期天
int changesWaterCount = 5;         //换水量5L
String pushKey = "";
String configString = "";
// 当前时间(小时)
int currentHour;
// 预览模式,1秒切一个时间
bool isPreModel;
int currentPreCount = -1;

void core1LedPwmTask(void *parameter);
// 监听水位,低于水位进行自动补水
void monitorFishTankWaterLevel();
// 监听换水
void monitorWaterChange();
//进入换水
void enterWaterChangeMode();
//退出换水
void exitWaterChangeMode();
// 监听故障
void monitorAlert();
// 设置pwm的百分比
void setPWMPercentage(int pin, int percentage);
// NTC温度
void readTemperature();
//发送通知
void sc_send(String message);
//加载配置
void loadConfig();
//API
void jsonGetApi();
void jsonUpdataApi();
void reloadConfig(JsonObject &root);
String getJsonConfig();
void indexPage();

void setup()
{
  Serial.begin(115200);
  // 设置电机引脚为输出
  pinMode(motor_bi_pin, OUTPUT);
  pinMode(motor_fi_pin, OUTPUT);
  // 默认关闭电机
  digitalWrite(motor_bi_pin, LOW);
  digitalWrite(motor_fi_pin, LOW);

  // 设置水位传感器引脚为输入
  pinMode(fishTank_water_level_pin, INPUT);
  pinMode(wastewater_level_pin, INPUT);
  pinMode(seawater_level_pin, INPUT);
  pinMode(backupwater_pin, INPUT);
  pinMode(mos_b1_pin, OUTPUT);

  // 设置NTC引脚为输入
  pinMode(ntc_pin, INPUT);
  // 设置电磁阀引脚为输出
  pinMode(solenoid_valve_pin, OUTPUT);

  // 设置故障指示灯和按钮
  pinMode(alert_pin, OUTPUT);
  pinMode(button_pin, OUTPUT);
  digitalWrite(button_pin, HIGH);

  digitalWrite(fishTank_water_level_pin, LOW);

  digitalWrite(mos_r_pin, LOW);
  digitalWrite(mos_g_pin, LOW);
  digitalWrite(mos_b_pin, LOW);
  digitalWrite(mos_w_pin, LOW);
  digitalWrite(mos_u_pin, LOW);
  digitalWrite(mos_b1_pin, LOW);
  digitalWrite(mos_b2_pin, LOW);
  digitalWrite(mos_b3_pin, LOW);
  // 设置PWM通道
  ledcSetup(0, pwm_freq, pwm_freq_resolution);
  ledcSetup(1, pwm_freq, pwm_freq_resolution);
  ledcSetup(2, pwm_freq, pwm_freq_resolution);
  ledcSetup(3, pwm_freq, pwm_freq_resolution);
  ledcSetup(4, pwm_freq, pwm_freq_resolution);
  ledcSetup(5, pwm_freq, pwm_freq_resolution);
  ledcSetup(6, pwm_freq, pwm_freq_resolution);

  // 将PWM通道连接到引脚
  ledcAttachPin(mos_r_pin, 0);
  ledcAttachPin(mos_g_pin, 1);
  ledcAttachPin(mos_b_pin, 2);
  ledcAttachPin(mos_w_pin, 3);
  ledcAttachPin(mos_u_pin, 4);
  ledcAttachPin(mos_b2_pin, 5);
  ledcAttachPin(mos_b3_pin, 6);
  // 默认关闭全部通道
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  ledcWrite(4, 0);
  ledcWrite(5, 0);
  ledcWrite(6, 0);

  // 点亮故障指示灯,代表系统正在启动
  digitalWrite(alert_pin, HIGH);
  WiFi.begin("Mesh_AP", "23333333");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  ntpClient.begin();
  ntpClient.update();
  digitalWrite(alert_pin, LOW);
  if (FORMAT_FILESYSTEM)
    LittleFS.format();
  // 初始化SPIFFS
  if (!LittleFS.begin())
  {
    isAlert = 1;
    alertMessage = "SPIFFS初始化失败!Web服务无法启动";
  }
  else
  {
    loadConfig();
    indexPage();
    jsonGetApi();
    jsonUpdataApi();
  }
  ElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  xTaskCreatePinnedToCore(
    core1LedPwmTask,   // 任务1的函数
    "LedTask",         // 任务名
    10000,            // 栈大小，单位字节
    NULL,            // 任务参数
    1,               // 任务优先级
    &LedTask,          // 任务句柄
    1                // 运行在核心1
  );
  sc_send("ESP鱼缸系统启动成功!");
}

void setPWMPercentage(int pwmChannel, int percentage)
{
  // 将百分比映射到PWM值范围
  int pwmValue = map(percentage, 0, 100, 0, 1023);
  // 设置PWM值
  ledcWrite(pwmChannel, pwmValue);
}

void indexPage(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
}

void jsonUpdataApi(){
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
       request->send(200, "application/json", "{\"msg\":\"success\"}"); 
  } });
}

String getJsonConfig(){
  DynamicJsonDocument doc(4096);
  doc["isEnableAddWather"] = isEnableAddWather;
  doc["isEnableWaterChanges"] = isEnableWaterChanges;
  doc["isEnableSunTime"] = isEnableLedSunTime;
  doc["TemperatureControlMode"] = TemperatureControlMode;
  doc["changesWaterTiming"] = changesWaterTiming;
  doc["temperature"] = temperature;
  doc["pushKey"] = pushKey;

  JsonArray rgbuvArray = doc.createNestedArray("rgbuv");
  for (int i = 0; i < 5; i++)
  {
    rgbuvArray.add(rgbwu[i]);
  }
  JsonArray pwmValuesArray = doc.createNestedArray("pwmValuesPerHour");
  for (int i = 0; i < 24; i++)
  {
    String key = String(i);
    JsonObject hourObj = pwmValuesArray.createNestedObject();
    JsonArray valuesArray = hourObj.createNestedArray(key);
    for (int j = 0; j < 5; j++)
    {
      valuesArray.add(pwmValuesPerHour[i][j]);
    }
  }
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

void jsonGetApi()
{
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              String jsonString = getJsonConfig();
  request->send(200, "application/json",jsonString); });
}

void loadConfig(){
  File file = LittleFS.open("/config.json", FILE_READ);
  String fileContent = "";
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return;
  }
  while (file.available())
  {
    char c = file.read();
    fileContent += c;
  }
  // 解析JSON
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, fileContent);
  // 检查解析错误
  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }
  JsonObject root = doc.as<JsonObject>();
  reloadConfig(root);
  file.close();
}

void reloadConfig(JsonObject &root){
  isEnableAddWather = root["isEnableAddWather"];           
  isEnableWaterChanges = root["isEnableWaterChanges"];     
  isEnableLedSunTime = root["isEnableSunTime"];               
  TemperatureControlMode = root["TemperatureControlMode"]; 
  changesWaterTiming = root["changesWaterTiming"];         
  pushKey = root["pushKey"].as<String>();

  JsonArray rgbwuValues = root["rgbwu"];
  for (int i = 0; i < 5; i++)
  {
    rgbwu[i] = rgbwuValues[i];
  }
  JsonArray pwmHourVlaues = root["pwmValuesPerHour"];
  for (int i = 0; i <= 23; i++)
  {
    String key = String(i);
    JsonObject value = pwmHourVlaues[i];
    for (int j = 0; j <= 4; j++)
    {
      pwmValuesPerHour[i][j] = value[key][j];
    }
  }
  Serial.print("刷新配置成功!");
  serializeJson(root, Serial);
}

void saveConfig(JsonObject &root){
  File file = LittleFS.open("/config.json", FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  String jsonString;
  serializeJson(root, jsonString);
  file.print(jsonString);
  Serial.print("刷新配置成功!");
  serializeJson(root, Serial);
  file.close();
}

void monitorAlert()
{
  if (isAlert)
  {
    digitalWrite(alert_pin, HIGH);
    Serial.print("当前故障:");
    Serial.println(alertMessage);
    if (sendMessage != alertMessage)
    {
      sc_send(alertMessage);
    }
  }
  else
  {
    digitalWrite(alert_pin, LOW);
  }
}

void monitorFishTankWaterLevel()
{
  if (waterChangeMode)
  {
    return;
  }
  if (isAlert)
  {
    return;
  }
  // Serial.println("监听补水中.......");
  // 低于水位，打开水泵补水(无水是高电平)
  if (digitalRead(fishTank_water_level_pin) == HIGH)
  {
    // 如果水泵尚未启动，记录启动时间
    if (pumpStartTime == 0)
    {
      pumpStartTime = millis();
    }
    // 设置PWM值
    digitalWrite(mos_b1_pin, HIGH);
    Serial.println("补水中.....");
    if (isAddWater == false)
    {
      isAddWater = true;
      sc_send("鱼缸开始补水.....");
    }
    // 如果水泵运行时间超过超时时间，关闭水泵
    if (millis() - pumpStartTime >= pumpTimeout)
    {
      digitalWrite(mos_b1_pin, LOW);
      isAlert = 1;
      alertMessage = "补水超时!请检查水位传感器是否异常";
      pumpStartTime = 0; // 重置启动时间
      isAddWater = false;
    }
  }
  else
  {
    if (isAddWater == true)
    {
      isAlert = 0;
      alertMessage = "";
      // 水位正常，关闭水泵
      digitalWrite(mos_b1_pin, LOW);
      pumpStartTime = 0; // 重置启动时间
      isAddWater = false;
      sc_send("水位恢复~结束补水.....");
      Serial.print("水位恢复~结束补水.....");
    }
  }
}

void monitorWaterChange()
{
  if (waterChangeMode == true)
  {
    enterWaterChangeMode();
  }
  int buttonState = digitalRead(button_pin);
  if (buttonState == LOW)
  {
    Serial.print("按钮被按下了");
    if (buttonPressStartTime == 0)
    {
      buttonPressStartTime = millis();
    }
    if (millis() - buttonPressStartTime >= longPressDuration)
    {
      if (waterChangeMode)
      {
        Serial.print("强制终止换水~!");
        exitWaterChangeMode();
      }
      else
      {
        sc_send("鱼缸手动进入换水模式!");
        enterWaterChangeMode();
      }
    }
  }
  else
  {
    buttonPressStartTime = 0;
  }
}

// 进入换水模式(齿轮泵在12V电压标称1分钟1L水)
void enterWaterChangeMode()
{
  Serial.print("进入换水模式.......");
  // 如果已经是低水位了,则不换水了
  if (digitalRead(fishTank_water_level_pin) == HIGH)
  {
    isAlert = true;
    alertMessage = "水位过低,换水操作终止!";
  }
  else
  {
    // 检查废水箱是不是满了
    if(digitalRead(wastewater_level_pin) == HIGH){
      isAlert = true;
      alertMessage = "废水箱已满!换水操作终止!";
      return;
    }
    // 检查是否接了海水传感器
    if (digitalRead(seawater_level_pin) == HIGH)
    {
      isAlert = true;
      alertMessage = "海水箱水量不足,停止换水操作,请及时补水!";
      return;
    }

    waterChangeMode = true;
    // 如果水泵尚未启动，记录启动时间
    if (pumpStartTime == 0)
    {
      pumpStartTime = millis();
    }
    digitalWrite(motor_bi_pin, HIGH);
    digitalWrite(motor_fi_pin, LOW);
    // 翻转水泵
    if (millis() - pumpStartTime >= waterChangePumpTimeout / 2)
    {
      digitalWrite(motor_bi_pin, LOW);
      digitalWrite(motor_fi_pin, HIGH);
      // 打开电磁阀
      digitalWrite(solenoid_valve_pin, HIGH);
      Serial.print("翻转水泵运行模式.......");
    }
    // 鱼缸水位恢复,停止换水
    if (digitalRead(fishTank_water_level_pin) == HIGH)
    {
      pumpStartTime = 0;
      exitWaterChangeMode();
    }
    // 如果水泵运行时间超过超时时间，关闭水泵
    if (millis() - pumpStartTime >= (waterChangePumpTimeout + 1000))
    {
      isAlert = 1;
      alertMessage = "换水超时!请检查水量是否充足,或者调整换水超时时间";
      pumpStartTime = 0; // 重置启动时间
      digitalWrite(motor_bi_pin, LOW);
      digitalWrite(motor_fi_pin, LOW);
      digitalWrite(solenoid_valve_pin, LOW);
      waterChangeMode = false;
    }
  }
}

void ledProgress()
{
  if(isEnableLedSunTime){
    return;
  }

  if (isPreModel)
  {
    currentPreCount++;
    currentHour = currentPreCount;
    if (currentPreCount >= 23)
    {
      isPreModel = false;
      currentPreCount = -1;
      Serial.print("预览结束.....");
    }
  }
  else
  {
    currentHour = ntpClient.getHours();
  }
  int nextHour = (currentHour + 1) % 24;
  Serial.println("当前Hour: " + String(currentHour));
  if (currentHour >= 0 && currentHour < 24 && nextHour >= 0 && nextHour < 24)
  {
    int startValues[5] = {
        pwmValuesPerHour[currentHour][0],
        pwmValuesPerHour[currentHour][1],
        pwmValuesPerHour[currentHour][2],
        pwmValuesPerHour[currentHour][3],
        pwmValuesPerHour[currentHour][4]
        };

    int endValues[5] = {
        pwmValuesPerHour[nextHour][0],
        pwmValuesPerHour[nextHour][1],
        pwmValuesPerHour[nextHour][2],
        pwmValuesPerHour[nextHour][3],
        pwmValuesPerHour[nextHour][4]
        };

    int elapsedMinutes = ntpClient.getMinutes();
    float progress = float(elapsedMinutes) / 60.00;
    if (isPreModel)
    {
      progress = 1;
    }
    Serial.println("当前区间进度: " + String(progress * 100) + "%");
    for (int i = 0; i < 5; i++)
    {
      int currentBrightness = 0;
      if (endValues[i] > startValues[i])
      {
        int value = endValues[i] - startValues[i];
        currentBrightness = int(progress * float(value)) + startValues[i];
        // Serial.println("++++");
      }
      else if (endValues[i] < startValues[i])
      {
        int value = startValues[i] - endValues[i];
        currentBrightness = startValues[i] - int(progress * float(value));
        // Serial.println("-------");
      }
      else
      {
        // Serial.println("======");
        currentBrightness = startValues[i];
      }
      currentBrightness = constrain(currentBrightness, 0, 100); 
      setPWMPercentage(i, currentBrightness);
      Serial.println("当前通道: " + String(i) + "PWM: " + String(currentBrightness));
    }
  }
}

// 退出换水模式
void exitWaterChangeMode()
{
  Serial.print("水位恢复~换水成功!");
  waterChangeMode = false;
  digitalWrite(motor_bi_pin, LOW);
  digitalWrite(motor_fi_pin, LOW);
  digitalWrite(solenoid_valve_pin, LOW);
  isAlert = 0;
  alertMessage = "";
}

void readTemperature(){
  int rawValue = analogRead(ntc_pin);
  // 电压分压公式：Vout = Vin * R2 / (R1 + R2)
  float voltage = rawValue * (3.3 / 1023.0);
  // 使用电压分压公式计算 NTC 的电阻值
  float ntcResistance = fixedResistor * voltage / (3.3 - voltage);
  // 使用热敏电阻温度计算公式计算温度
  float temperature = 1.0 / ((1.0 / (referenceTemperature + 273.15)) + (1.0 / 3950.0) * log(ntcResistance / 10000.0));
  temperature -= 273.15;  // 转换为摄氏温度
  Serial.print("Raw Value: ");
  Serial.print(rawValue);
  Serial.print(", Voltage: ");
  Serial.print(voltage);
  Serial.print("V, NTC Resistance: ");
  Serial.print(ntcResistance);
  Serial.print(" ohms, Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
}

void sc_send(String message)
{
  sendMessage = message;
  String serverUrl = "https://api2.pushdeer.com/message/push?pushkey=PDU23935TXd5sxyNlBZ9bvze0OSMTb9EKn8febs4P";
  serverUrl += "&text=鱼缸通知&desp=" + message + "&type=text";
  HTTPClient http;
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postData = String("title=鱼缸消息&desp=") + String(alertMessage);
  http.begin(serverUrl);
  int httpCode = http.POST(postData);
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    Serial.println("Response: " + payload);
  }
  else
  {
    Serial.println("HTTP request failed");
  }
  http.end();
}

void core1LedPwmTask(void *parameter)
{
  while (1)
  {
    ledProgress();
    delay(100);
  }
}

void loop()
{
  monitorFishTankWaterLevel();
  monitorWaterChange();
  readTemperature();
  monitorAlert();
  delay(100);
}
