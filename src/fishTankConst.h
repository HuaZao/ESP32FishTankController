#ifndef FINSH_TANK_CONST_H
#define FINSH_TANK_CONST_H

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

// 仅第一次需要初始化文件系统
#define FORMAT_FILESYSTEM false

//LED通道数 R,G,B,W,U 
#define LED_Channel 5
//风扇启动阈值(LED PWM 相加),如RGB 最大PWM为300,
#define fan_start_count 100

//I2C
// #define SDA_PIN 21
// #define SCL_PIN 22

//WIFI
#define WIFI_SSID "Mesh_AP"
#define WIFI_PASSWORD "23333333"
#define MQTT_HOST IPAddress(192, 168, 10, 203)
#define MQTT_PORT 1883

// NTC
#define  ntcResistance 10000.00    //  校准参数 - NTC 温度传感器的电阻。如果您使用 10k NTC，请更改为 10000.00
#define  referenceTemperature  25.0 // 参考温度（NTC的阻值在该温度下被标定）
#define  ntc_pin 34                 // NTC

// 传感器引脚
#define  motor_bi_pin 21             // 电机后退
#define  motor_fi_pin 19             // 电机前进
#define  fishTank_water_level_pin 22 // 鱼缸水位
#define  wastewater_level_pin 23     // 废水水位
#define  seawater_level_pin 18       // 海水水位
#define  backupwater_pin 16          // 物理备用水位
#define  solenoid_valve_pin 4        // 电磁阀
#define  alert_pin 5                 // 故障指示灯
#define  button_pin 17               // 换水按钮

// MOS引脚
#define  mos_b1_pin 13 //B1
#define  mos_b2_pin 12 //B2
#define  mos_b3_pin 14 //B3
#define  mos_u_pin 27 // U
#define  mos_b_pin 26 // B
#define  mos_g_pin 25 // G
#define  mos_r_pin 33 // R
#define  mos_w_pin 32 // W

// 备用引脚
#define  backup_pin 35

// PWM频率
#define  pwm_freq 68000
#define  pwm_freq_resolution 10

//MQTT Topic
#define MQTT_UPDATE_STORE_TOPIC "finsh_tank/store/update"        //更新配置参数
#define MQTT_SEND_STORE_TOPIC "finsh_tank/store/value"            //获取配置参数
#define MQTT_SEND_TEMP_TOPIC "finsh_tank/sensor/temperature"     //发送温度数据
#define MQTT_SEND_LED_TOPIC "finsh_tank/led/value"               //发送当前LED数据
#define MQTT_UPDATE_LED_TOPIC "finsh_tank/led/update"            //更新当前LED数据
#define MQTT_REBOOT_TOPIC "finsh_tank/task/reboot"               //重启
#define MQTT_PREVIEW_TOPIC "finsh_tank/task/preview"             //预览


#endif