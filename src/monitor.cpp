#include "monitor.h"

extern xSemaphoreHandle SemaphoreHandle;

unsigned long pumpStartTime = 0;                   // 记录水泵启动的时间
unsigned long pumpTimeout = 10000;                 // 补水超时时间，单位：毫秒
unsigned long waterChangePumpTimeout = 1000 * 120; // 换水泵超时时间，单位：毫秒
bool waterChangeMode = false;                      // 换水模式标志
bool isAddWater = false;                           // 是否在补水中
bool isAlert = false;                              
String alertMessage = "";
String sendMessage = "";

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
    //打开B1
    digitalWrite(mos_b1_pin, HIGH);
    SensorDataStruct.mos_b1 = 100;
    Serial.println("补水中.....");
    if (isAddWater == false)
    {
      isAddWater = true;
      //   sc_send("鱼缸开始补水.....");
    }
    // 如果水泵运行时间超过超时时间，关闭水泵
    if (millis() - pumpStartTime >= pumpTimeout)
    {
      digitalWrite(mos_b1_pin, LOW);
      SensorDataStruct.mos_b1 = 0;
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
  // if (waterChangeMode == true)
  // {
  //   enterWaterChangeMode();
  // }
  // int buttonState = digitalRead(button_pin);
  // if (buttonState == LOW)
  // {
  //   Serial.print("按钮被按下了");
  //   if (buttonPressStartTime == 0)
  //   {
  //     buttonPressStartTime = millis();
  //   }
  //   if (millis() - buttonPressStartTime >= longPressDuration)
  //   {
  //     if (waterChangeMode)
  //     {
  //       Serial.print("强制终止换水~!");
  //       exitWaterChangeMode();
  //     }
  //     else
  //     {
  //       sc_send("鱼缸手动进入换水模式!");
  //       enterWaterChangeMode();
  //     }
  //   }
  // }
  // else
  // {
  //   buttonPressStartTime = 0;
  // }
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
    if (digitalRead(wastewater_level_pin) == HIGH)
    {
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

void monitorMainTask(void *arg)
{
  vTaskDelay(3000 / portTICK_PERIOD_MS); // 设置完成后要有一段延迟
  xSemaphoreGive(SemaphoreHandle);       // 发送一个信号量更新数据
  for (;;)
  {
    if (xSemaphoreTake(SemaphoreHandle, portMAX_DELAY) == pdTRUE) // 等待信号量
    {
      // 监听水位
      monitorFishTankWaterLevel();
      // 监听换水
      monitorWaterChange();
      // 监听故障
      monitorAlert();
      vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
  }
}