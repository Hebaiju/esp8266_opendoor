#include <Arduino.h>
#include "common/config.h"
#include "common/log.h"
#include "driver/drv_led.h"
#include "driver/drv_servo.h"
#include "driver/drv_ir.h"
#include "service/srv_wifi.h"
#include "service/srv_ntp.h"
#include "service/srv_ir_ac.h"
#include "service/srv_web.h"
#include "app_task/task_headers.h"

void setup()
{
    Serial.begin(UART_BAUDRATE);
    delay(100);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP8266-01S 开门器 - 点灯科技");
    Serial.println("  版本: 0.0.2");
    Serial.println("========================================");
    Serial.println();

    LOG_I("硬件初始化...");

    if (drv_led_init() != APP_OK) {
        LOG_E("LED初始化失败");
    } else {
        LOG_I("LED初始化完成");
    }

    if (drv_servo_init() != APP_OK) {
        LOG_E("舵机初始化失败");
    } else {
        LOG_I("舵机初始化完成, 引脚=%d, 起始角度=%d",
              SERVO_PIN, SERVO_MIN_ANGLE);
    }

    if (srv_ir_ac_init() != APP_OK) {
        LOG_E("空调红外服务初始化失败");
    } else {
        LOG_I("空调红外服务初始化完成");
    }

    LOG_I("服务层初始化...");
    srv_wifi_init();
    srv_ntp_init();
    srv_web_init();

    LOG_I("任务层初始化...");

    task_led_init();
    task_uart_init();
    task_door_init();
    task_blinker_init();

    LOG_I("系统初始化完成，开始运行");
}

void loop()
{
    srv_wifi_run();
    srv_ntp_run();
    srv_ir_ac_run();
    srv_web_run();
    task_blinker_run();
    task_door_run();
    task_led_run();
    task_uart_run();
}
