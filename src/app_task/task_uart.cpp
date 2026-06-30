#include "task_headers.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../driver/drv_led.h"
#include "../driver/drv_servo.h"
#include "../service/srv_ntp.h"
#include "../service/srv_blinker.h"
#include <ESP8266WiFi.h>
#include <time.h>

#define UART_BUF_SIZE  64

static uint32_t s_last_tick = 0;
static uint32_t s_heartbeat_count = 0;
static char     s_cmd_buf[UART_BUF_SIZE];
static uint8_t  s_cmd_len = 0;

static const char *led_mode_str(uint8_t mode)
{
    switch (mode) {
        case 0: return "关闭";
        case 1: return "常亮";
        case 2: return "闪烁";
        case 3: return "呼吸";
        case 4: return "自动";
        default: return "未知";
    }
}

static void uart_print_help(void)
{
    Serial.println();
    Serial.println("===== 串口命令列表 =====");
    Serial.println("  help           - 显示此帮助");
    Serial.println("  wifi           - 显示WiFi状态");
    Serial.println("  time           - 显示当前时间(NTP)");
    Serial.println("  led on         - LED开启");
    Serial.println("  led off        - LED关闭");
    Serial.println("  led blink      - LED闪烁模式(500ms)");
    Serial.println("  led fast       - LED快速闪烁(100ms)");
    Serial.println("  led breath     - LED呼吸灯模式");
    Serial.println("  led auto       - LED自动模式(时间控制)");
    Serial.println("  led toggle     - 手动切换灯光");
    Serial.println("  servo [n]      - 设置舵机角度(0-180)");
    Serial.println("  status         - 显示系统状态");
    Serial.println("  open           - 触发开门");
    Serial.println("=======================");
    Serial.println();
}

static void uart_print_wifi(void)
{
    Serial.println();
    Serial.println("===== WiFi状态 =====");
    Serial.printf("  状态      : %s\r\n",
                  WiFi.status() == WL_CONNECTED ? "已连接" : "未连接");
    Serial.printf("  热点名称  : %s\r\n", WiFi.SSID().c_str());
    Serial.printf("  IP地址    : %s\r\n", WiFi.localIP().toString().c_str());
    Serial.printf("  MAC地址   : %s\r\n", WiFi.macAddress().c_str());
    Serial.printf("  信号强度  : %d dBm\r\n", WiFi.RSSI());
    Serial.printf("  信道      : %d\r\n", WiFi.channel());
    Serial.println("===================");
    Serial.println();
}

static void uart_print_time(void)
{
    Serial.println();
    Serial.println("===== 时间状态 =====");
    if (!srv_ntp_is_synced()) {
        Serial.println("  NTP       : 未同步");
    } else {
        struct tm t;
        if (srv_ntp_get_time(&t)) {
            Serial.printf("  NTP       : 已同步\r\n");
            Serial.printf("  当前时间  : %04d-%02d-%02d %02d:%02d:%02d\r\n",
                          t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                          t.tm_hour, t.tm_min, t.tm_sec);
            Serial.printf("  小时      : %d\r\n", t.tm_hour);
        }
    }
    Serial.printf("  自动开启时段: %d:00-%d:00, %d:00-%d:00\r\n",
                  LED_AUTO_ON_START_HOUR, LED_AUTO_ON_END_HOUR,
                  LED_AUTO_ON_START_HOUR2, LED_AUTO_ON_END_HOUR2);
    Serial.println("===================");
    Serial.println();
}

static void uart_print_status(void)
{
    Serial.println();
    Serial.println("===== 系统状态 =====");
    Serial.printf("  运行时间  : %lu ms\r\n", (unsigned long)millis());
    Serial.printf("  心跳计数  : %lu\r\n", (unsigned long)s_heartbeat_count);
    Serial.printf("  LED引脚   : %d (低电平点亮)\r\n", ONBOARD_LED_PIN);
    Serial.printf("  LED模式   : %s\r\n", led_mode_str(task_led_get_mode()));
    Serial.printf("  LED亮度   : %d / %d\r\n",
                  drv_led_get_brightness(), LED_PWM_MAX);
    Serial.printf("  舵机引脚  : %d, 角度: %d度\r\n",
                  SERVO_PIN, drv_servo_get_angle());
    Serial.printf("  WiFi      : %s\r\n",
                  WiFi.status() == WL_CONNECTED ? "已连接" : "未连接");
    Serial.printf("  NTP       : %s\r\n",
                  srv_ntp_is_synced() ? "已同步" : "未同步");
    Serial.printf("  点灯科技  : %s\r\n",
                  srv_blinker_is_ready() ? "已就绪" : "连接中");
    Serial.println("===================");
    Serial.println();
}

static void uart_handle_cmd(const char *cmd)
{
    if (strlen(cmd) == 0) {
        return;
    }

    Serial.printf(">> 指令: %s\r\n", cmd);

    if (strcmp(cmd, "help") == 0) {
        uart_print_help();
    } else if (strcmp(cmd, "wifi") == 0) {
        uart_print_wifi();
    } else if (strcmp(cmd, "time") == 0) {
        uart_print_time();
    } else if (strcmp(cmd, "status") == 0) {
        uart_print_status();
    } else if (strcmp(cmd, "open") == 0) {
        door_send_cmd(DOOR_CMD_OPEN);
        Serial.println("开门指令已发送");
    } else if (strncmp(cmd, "led ", 4) == 0) {
        const char *arg = cmd + 4;
        if (strcmp(arg, "on") == 0) {
            task_led_set_mode(1);
            Serial.println("LED模式: 常亮");
        } else if (strcmp(arg, "off") == 0) {
            task_led_set_mode(0);
            Serial.println("LED模式: 关闭");
        } else if (strcmp(arg, "blink") == 0) {
            task_led_set_interval(500);
            task_led_set_mode(2);
            Serial.println("LED模式: 闪烁(500ms)");
        } else if (strcmp(arg, "fast") == 0) {
            task_led_set_interval(100);
            task_led_set_mode(2);
            Serial.println("LED模式: 快速闪烁(100ms)");
        } else if (strcmp(arg, "breath") == 0) {
            task_led_set_mode(3);
            Serial.println("LED模式: 呼吸灯");
        } else if (strcmp(arg, "auto") == 0) {
            task_led_set_mode(4);
            Serial.println("LED模式: 自动(时间控制)");
        } else if (strcmp(arg, "toggle") == 0) {
            task_led_manual_toggle();
            Serial.println("LED手动切换");
        } else {
            Serial.printf("未知LED参数: %s\r\n", arg);
        }
    } else if (strncmp(cmd, "servo ", 6) == 0) {
        int angle = atoi(cmd + 6);
        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;
        drv_servo_set_angle(angle);
        Serial.printf("舵机设置为 %d 度\r\n", angle);
    } else {
        Serial.printf("未知指令: %s (输入 'help' 查看列表)\r\n", cmd);
    }
}

static void uart_poll_input(void)
{
    while (Serial.available() > 0) {
        char c = Serial.read();

        if (c == '\r' || c == '\n') {
            if (s_cmd_len > 0) {
                s_cmd_buf[s_cmd_len] = '\0';
                uart_handle_cmd(s_cmd_buf);
                s_cmd_len = 0;
                s_cmd_buf[0] = '\0';
            }
        } else if (c == '\b' || c == 0x7F) {
            if (s_cmd_len > 0) {
                s_cmd_len--;
                s_cmd_buf[s_cmd_len] = '\0';
            }
        } else if (s_cmd_len < UART_BUF_SIZE - 1) {
            s_cmd_buf[s_cmd_len++] = c;
            s_cmd_buf[s_cmd_len] = '\0';
        }
    }
}

void task_uart_init(void)
{
    s_last_tick = millis();
    s_cmd_len = 0;
    s_cmd_buf[0] = '\0';
    LOG_I("串口任务初始化");
    Serial.println();
    Serial.println("输入 'help' 查看命令列表");
    Serial.println();
}

void task_uart_run(void)
{
    uart_poll_input();

    uint32_t now = millis();
    if (now - s_last_tick >= 5000) {
        s_last_tick = now;
        s_heartbeat_count++;

        const char *wifi_str = WiFi.status() == WL_CONNECTED ? "已连接" : "未连接";
        const char *ntp_str = srv_ntp_is_synced() ? "已同步" : "未同步";
        const char *blink_str = srv_blinker_is_ready() ? "已就绪" : "连接中";
        const char *led_str = led_mode_str(task_led_get_mode());

        LOG_I("心跳 #%lu | WiFi:%s | NTP:%s | 点灯:%s | LED:%s",
              (unsigned long)s_heartbeat_count,
              wifi_str, ntp_str, blink_str, led_str);
    }
}
