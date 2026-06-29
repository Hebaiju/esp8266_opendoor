#include <Arduino.h>
#include "common/config.h"
#include "common/log.h"
#include "driver/drv_led.h"
#include "driver/drv_servo.h"
#include "service/srv_wifi.h"
#include "app_task/task_headers.h"

void setup()
{
    Serial.begin(UART_BAUDRATE);
    delay(100);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP8266-01S OpenDoor - Blinker");
    Serial.println("  Version: 0.0.2");
    Serial.println("========================================");
    Serial.println();

    LOG_I("Hardware init...");

    if (drv_led_init() != APP_OK) {
        LOG_E("LED init failed");
    } else {
        LOG_I("LED init ok");
    }

    if (drv_servo_init() != APP_OK) {
        LOG_E("Servo init failed");
    } else {
        LOG_I("Servo init ok, pin=%d, start angle=%d",
              SERVO_PIN, SERVO_MIN_ANGLE);
    }

    LOG_I("Service init...");
    srv_wifi_init();

    LOG_I("Task init...");

    task_led_init();
    task_uart_init();
    task_door_init();
    task_blinker_init();

    LOG_I("System init complete, running");
}

void loop()
{
    srv_wifi_run();
    task_blinker_run();
    task_door_run();
    task_led_run();
    task_uart_run();
}
