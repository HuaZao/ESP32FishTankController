#include "main.h"
xSemaphoreHandle SemaphoreHandle;
static const char* TAG = "main_App_Log";   

void ESP_GPIO_init(void);

void setup(){
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);        // set all components to ERROR level
  ESP_LOGI(TAG,"初始化中......");
  //初始化IO
  // ESP_GPIO_init();
  //初始化储存
  initStore();
  //连接Wi-Fi
  setupService();
  initWebService();
  //初始化信号
  SemaphoreHandle = xSemaphoreCreateBinary();
  // //监听任务
  xTaskCreate(monitorMainTask, "monitor_Main_Task", 2048, NULL, 1, NULL); 
  //传感器采集
  xTaskCreate(gatherSensorTask, "gather_Sensor_Task", 2048, NULL, 1, NULL); 
  //LED任务
  xTaskCreatePinnedToCore(core1LedPwmTask, "led_Task", 2048 , NULL, 1, &LedTask, 1);
  // //发送MQTT数据
  // xTaskCreate(sendMqttDataTask, "send_Mqtt_Data_Task", 2048 , NULL, 3, NULL); 
  ESP_LOGI(TAG,"初始化完成!");
}

void ESP_GPIO_init(void)
{ 
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
  digitalWrite(backupwater_pin, LOW);
  pinMode(mos_b1_pin, OUTPUT);

  // 设置NTC引脚为输入
  pinMode(ntc_pin, INPUT);
  // 设置电磁阀引脚为输出
  pinMode(solenoid_valve_pin, OUTPUT);

  // 设置故障指示灯和按钮
  pinMode(alert_pin, OUTPUT);
  pinMode(button_pin, INPUT);
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
}

void loop()
{
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  //持续发送更新信号
  xSemaphoreGive(SemaphoreHandle);
}
