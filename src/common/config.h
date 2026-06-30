#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

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

/* WiFi配置 - 默认配置（首次使用或配网失败时） */
#define WIFI_SSID                  "open-dog"
#define WIFI_PASSWORD              "31415926"

/* AP热点配置 */
#define AP_SSID_PREFIX             "ESP_8266"
#define AP_PASSWORD                ""              /* 空密码 = 开放热点 */

/* MQTT默认配置 */
#define MQTT_DEFAULT_HOST           "broker.emqx.io"
#define MQTT_DEFAULT_PORT           "1883"
#define MQTT_DEFAULT_TOPIC          "esp8266/opendoor/door"
#define MQTT_DEFAULT_PUB_TOPIC      "esp8266/opendoor/dooo"
#define MQTT_RETRY_INTERVAL_MS      10000
#define MQTT_KEEPALIVE_SEC          60
#define MQTT_PING_INTERVAL_MS        30000

/* MQTT指令关键词 - 收到包含以下任一关键词的消息时执行开门 */
#define MQTT_CMD_OPEN_1             "opendoor"

/* MQTT灯光指令 - 收到包含以下关键词时反转灯光状态 */
#define MQTT_CMD_LIGHT              "light"

/* MQTT回复消息 */
#define MQTT_REPLY_OPENED           "ok"
#define MQTT_REPLY_LIGHT_ON         "light_on"
#define MQTT_REPLY_LIGHT_OFF        "light_off"

/* WiFi配置持久化存储区 */
#define EEPROM_WIFI_ADDR           400
#define EEPROM_WIFI_MAGIC          0xEE
#define EEPROM_WIFI_SSID_MAX       32
#define EEPROM_WIFI_PASS_MAX       64

/* Blinker密钥持久化存储区 */
#define EEPROM_BLINKER_ADDR        600
#define EEPROM_BLINKER_MAGIC       0xBB
#define EEPROM_BLINKER_AUTH_MAX    32

/* MQTT配置持久化存储区 */
#define EEPROM_MQTT_ADDR           700
#define EEPROM_MQTT_MAGIC          0xEF
#define EEPROM_MQTT_HOST_MAX       64
#define EEPROM_MQTT_PORT_MAX       6
#define EEPROM_MQTT_USER_MAX       32
#define EEPROM_MQTT_PASS_MAX       32
#define EEPROM_MQTT_TOPIC_MAX      64
#define EEPROM_MQTT_CLIENT_MAX     32

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
#define MAX_SCAN_RESULTS           15

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
