#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

#define BLINKER_AUTH               "846631006679"

#define BLINKER_KEY_SERVO          "key-servo"
#define BLINKER_KEY_LIGHT          "key-light"
#define BLINKER_BTN_PWR            "btn-pwr"
#define BLINKER_BTN_FAN            "btn-fan"
#define BLINKER_BTN_COOL           "btn-cool"
#define BLINKER_BTN_DRY            "btn-dry"
#define BLINKER_BTN_HOT            "btn-hot"
#define BLINKER_BTN_AUTO           "btn-auto"
#define BLINKER_SLIDER_TEMP        "ran-wen"
#define BLINKER_NUMBER_TEMP        "settemp"

#define WIFI_SSID                  "xxxx"
#define WIFI_PASSWORD              "31415926"

#define ONBOARD_LED_PIN            2
#define LED_ACTIVE_LEVEL           0

#define LED_PWM_MIN                0
#define LED_PWM_MAX                1023
#define LED_BREATH_STEP            20
#define LED_BREATH_INTERVAL_MS     8

#define LED_AUTO_ON_START_HOUR     8
#define LED_AUTO_ON_END_HOUR       12
#define LED_AUTO_ON_START_HOUR2    14
#define LED_AUTO_ON_END_HOUR2      23

#define SERVO_PIN                  0
#define SERVO_MIN_ANGLE            0
#define SERVO_MAX_ANGLE            120
#define SERVO_HOLD_MS              3000

#define IR_TX_PIN                  3

#define AC_DEFAULT_MODE            1
#define AC_DEFAULT_TEMP            26
#define AC_DEFAULT_FAN_LEVEL       4
#define AC_SEND_DEBOUNCE_MS        500

#define EEPROM_TOTAL_SIZE          1024
#define EEPROM_AC_ADDR             500
#define EEPROM_AC_MAGIC            0xAC

#define NTP_SERVER                 "ntp.aliyun.com"
#define NTP_TIMEZONE               8
#define NTP_SYNC_INTERVAL_MS       3600000UL
#define NTP_RETRY_INTERVAL_MS      10000UL

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
