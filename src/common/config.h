#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

#define BLINKER_AUTH               "846631006679"
#define BLINKER_BUTTON_OPEN        "btn-open"

#define WIFI_SSID                  "xxxx"
#define WIFI_PASSWORD              "31415926"

#define ONBOARD_LED_PIN            2
#define LED_ACTIVE_LEVEL           0

#define SERVO_PIN                  0
#define SERVO_MIN_ANGLE            0
#define SERVO_MAX_ANGLE            90
#define SERVO_HOLD_MS              2000

#define IR_TX_PIN                  3

#define UART_BAUDRATE              115200

#define TASK_BLINKER_STACK_SIZE    1024
#define TASK_BLINKER_PRIORITY      3
#define TASK_DOOR_STACK_SIZE       512
#define TASK_DOOR_PRIORITY         2
#define TASK_LED_STACK_SIZE        256
#define TASK_LED_PRIORITY          1
#define TASK_UART_STACK_SIZE       512
#define TASK_UART_PRIORITY         1

#define LED_BLINK_WIFI_FAIL_MS     200
#define LED_BLINK_CONNECTED_MS     2000
#define LED_DOOR_ON_MS             100

typedef enum {
    SYS_INIT = 0,
    SYS_WIFI_CONNECTING,
    SYS_WIFI_CONNECTED,
    SYS_BLINKER_READY,
    SYS_DOOR_OPENING,
} system_state_t;

#endif
