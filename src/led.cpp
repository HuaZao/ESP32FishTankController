#include "led.h"
extern xSemaphoreHandle SemaphoreHandle;
TaskHandle_t LedTask;


// 当前时间(小时)
int currentHour;
// 下一个小时的PWM数据
int currentPreCount;
// 预览模式,1秒切一个时间
bool isPreModel;


void setPWMPercentage(int pwmChannel, int percentage)
{
    // 将百分比映射到PWM值范围
    int pwmValue = map(percentage, 0, 100, 0, 1024);
    ESP_LOGI("LedProgress", "当前通道: %d%-PWM: %d", pwmChannel, pwmValue);
    // 设置PWM值
    ledcWrite(pwmChannel, pwmValue);
}

void startPreModel()
{
    ESP_LOGI("LedProgress","进入预览模式!");
    currentPreCount = -1;
    isPreModel = true;
}

void ledProgress()
{
    int pwmCount = 0;
    if (StoreDataStruct.isEnableLedSunTime == 0){
        for (int i = 0; i < LED_Channel; i++)
        {
            setPWMPercentage(i, StoreDataStruct.rgbwu[i]);
            pwmCount = pwmCount + StoreDataStruct.rgbwu[i];
        }
    }else{
        if (isPreModel){
            currentPreCount++;
            currentHour = currentPreCount;
            if (currentPreCount >= 23)
            {
                isPreModel = false;
                currentPreCount = -1;
                ESP_LOGI("LedProgress", "预览结束.....");
            }
        }else{
            if (!ntpClient.isTimeSet()){
                ntpClient.update();
                ESP_LOGI("LedProgress", "等待 NTP 时钟....");
                return;
            }
            currentHour = ntpClient.getHours();
        }
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
        int elapsedMinutes = ntpClient.getMinutes();
        int elapsedSeconds =  ntpClient.getSeconds();
        // 将小时、分钟和秒都转换为秒，然后加在一起
        int totalSeconds =  elapsedMinutes * 60 + elapsedSeconds;
        // 计算总秒在一小时中的百分比
        float progress = (totalSeconds / 3600.00);
        if (isPreModel){
            progress = 1;
        }
        ESP_LOGI("LedProgress", "当前区间进度: %s %", String(progress * 100.00));
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
    vTaskDelay(3000 / portTICK_PERIOD_MS); 
    xSemaphoreGive(SemaphoreHandle);
    for (;;)
    {
        if (xSemaphoreTake(SemaphoreHandle, portMAX_DELAY) == pdTRUE) 
        {
            ledProgress();
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}