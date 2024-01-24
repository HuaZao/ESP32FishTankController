#include "led.h"

TaskHandle_t LedTask;
WiFiUDP udp;
NTPClient ntpClient(udp, "pool.ntp.org", 8 * 3600); // 中国时间

// 当前时间(小时)
int currentHour;
// 预览模式,1秒切一个时间
bool isPreModel;
// 下一个小时的PWM数据
int currentPreCount;

void setPWMPercentage(int pwmChannel, int percentage)
{
    // 将百分比映射到PWM值范围
    int pwmValue = map(percentage, 0, 100, 0, 2 ^ pwm_freq_resolution);
    // 设置PWM值
    ledcWrite(pwmChannel, pwmValue);
}

void startPreModel()
{
    Serial.println("进入预览模式!");
    currentPreCount = -1;
    isPreModel = true;
}

void ledProgress()
{
    int pwmCount = 0;
    if (StoreDataStruct.isEnableLedSunTime == 0){
        for (int i = 0; i < LED_Channel; i++)
        {
            Serial.println("当前通道: " + String(i) + "PWM: " + String(StoreDataStruct.rgbwu[i]));
            setPWMPercentage(i, StoreDataStruct.rgbwu[i]);
            pwmCount = pwmCount + StoreDataStruct.rgbwu[i];
        }
    }
    else{
        if (isPreModel){
            currentPreCount++;
            currentHour = currentPreCount;
            if (currentPreCount >= 23)
            {
                isPreModel = false;
                currentPreCount = -1;
                Serial.print("预览结束.....");
            }
        }else{
            if (!ntpClient.isTimeSet()){
                ntpClient.update();
                ESP_LOGI("LedProgress", "等待 NTP 时钟....");
                return;
            }
            currentHour = ntpClient.getHours();
            int nextHour = (currentHour + 1) % 24;
            ESP_LOGI("LedProgress", "当前Hour:%s", String(currentHour));
            //获取LED 开始和结束的亮度
            int startValues[LED_Channel];
            int endValues[LED_Channel];
            for (int i = 0; i < LED_Channel; i++)
            {
                startValues[i] = StoreDataStruct.pwmValuesPerHour[currentHour][i];
                endValues[i] = StoreDataStruct.pwmValuesPerHour[nextHour][i];
            }
            // 获取一个小时的区间进度,如当前为30分,就是50%
            int elapsedMinutes = ntpClient.getMinutes();
            float progress = float(elapsedMinutes) / 60.00;
            if (isPreModel){
                progress = 1;
            }
            ESP_LOGI("LedProgress", "当前区间进度: %s%", String(progress * 100));
            //根据 当前时间进度,开始,结束 计数出当前的PWM亮度
            for (int i = 0; i < LED_Channel; i++){
                int currentBrightness = 0;
                if (endValues[i] > startValues[i])
                {
                    int value = endValues[i] - startValues[i];
                    currentBrightness = int(progress * float(value)) + startValues[i];
                }
                else if (endValues[i] < startValues[i])
                {
                    int value = startValues[i] - endValues[i];
                    currentBrightness = startValues[i] - int(progress * float(value));
                }
                else
                {
                    currentBrightness = startValues[i];
                }
                currentBrightness = constrain(currentBrightness, 0, 100);
                setPWMPercentage(i, currentBrightness);
                pwmCount = pwmCount + currentBrightness;
                ESP_LOGI("LedProgress", "当前通道: %d% PWM: %d", i, currentBrightness);
            }
        }
    }
    if (pwmCount > fan_start_count){
        setPWMPercentage(5, pwmCount / LED_Channel);
    }
    else{
        setPWMPercentage(5, 0);
    }
}

void core1LedPwmTask(void *parameter)
{
  while (1)
  {
    ledProgress();
    delay(500);
  }
}