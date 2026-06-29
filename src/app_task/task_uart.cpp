#include "task_headers.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../driver/drv_led.h"
#include "../driver/drv_servo.h"
#include <ESP8266WiFi.h>

#define UART_BUF_SIZE  64

static uint32_t s_last_tick = 0;
static uint32_t s_heartbeat_count = 0;
static char     s_cmd_buf[UART_BUF_SIZE];
static uint8_t  s_cmd_len = 0;

static void uart_print_help(void)
{
    Serial.println();
    Serial.println("===== UART Commands =====");
    Serial.println("  help       - Show this help");
    Serial.println("  wifi       - Show WiFi status");
    Serial.println("  led on     - LED on");
    Serial.println("  led off    - LED off");
    Serial.println("  led blink  - LED blink (500ms)");
    Serial.println("  led fast   - LED fast blink (100ms)");
    Serial.println("  servo [n]  - Set servo angle (0-180)");
    Serial.println("  status     - Show system status");
    Serial.println("  open       - Trigger door open");
    Serial.println("=========================");
    Serial.println();
}

static void uart_print_wifi(void)
{
    Serial.println();
    Serial.println("===== WiFi Status =====");
    Serial.printf("  Status    : %s\r\n",
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.printf("  SSID      : %s\r\n", WiFi.SSID().c_str());
    Serial.printf("  IP        : %s\r\n", WiFi.localIP().toString().c_str());
    Serial.printf("  MAC       : %s\r\n", WiFi.macAddress().c_str());
    Serial.printf("  RSSI      : %d dBm\r\n", WiFi.RSSI());
    Serial.printf("  Channel   : %d\r\n", WiFi.channel());
    Serial.println("=======================");
    Serial.println();
}

static void uart_print_status(void)
{
    Serial.println();
    Serial.println("===== System Status =====");
    Serial.printf("  Uptime    : %lu ms\r\n", (unsigned long)millis());
    Serial.printf("  Heartbeat : %lu\r\n", (unsigned long)s_heartbeat_count);
    Serial.printf("  LED pin   : %d (active %s)\r\n",
                  ONBOARD_LED_PIN, LED_ACTIVE_LEVEL ? "HIGH" : "LOW");
    Serial.printf("  Servo pin : %d, angle: %d deg\r\n",
                  SERVO_PIN, drv_servo_get_angle());
    Serial.printf("  WiFi      : %s\r\n",
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.printf("  Blinker   : %s\r\n", "init done");
    Serial.println("=========================");
    Serial.println();
}

static void uart_handle_cmd(const char *cmd)
{
    if (strlen(cmd) == 0) {
        return;
    }

    Serial.printf(">> CMD: %s\r\n", cmd);

    if (strcmp(cmd, "help") == 0) {
        uart_print_help();
    } else if (strcmp(cmd, "wifi") == 0) {
        uart_print_wifi();
    } else if (strcmp(cmd, "status") == 0) {
        uart_print_status();
    } else if (strcmp(cmd, "open") == 0) {
        door_send_cmd(DOOR_CMD_OPEN);
        Serial.println("Door open command sent");
    } else if (strncmp(cmd, "led ", 4) == 0) {
        const char *arg = cmd + 4;
        if (strcmp(arg, "on") == 0) {
            drv_led_on();
            Serial.println("LED on");
        } else if (strcmp(arg, "off") == 0) {
            drv_led_off();
            Serial.println("LED off");
        } else if (strcmp(arg, "blink") == 0) {
            task_led_set_interval(500);
            Serial.println("LED blink (500ms)");
        } else if (strcmp(arg, "fast") == 0) {
            task_led_set_interval(100);
            Serial.println("LED fast blink (100ms)");
        } else {
            Serial.printf("Unknown led arg: %s\r\n", arg);
        }
    } else if (strncmp(cmd, "servo ", 6) == 0) {
        int angle = atoi(cmd + 6);
        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;
        drv_servo_set_angle(angle);
        Serial.printf("Servo set to %d deg\r\n", angle);
    } else {
        Serial.printf("Unknown command: %s (type 'help' for list)\r\n", cmd);
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
    LOG_I("UART task init");
    Serial.println();
    Serial.println("Type 'help' for command list");
    Serial.println();
}

void task_uart_run(void)
{
    uart_poll_input();

    uint32_t now = millis();
    if (now - s_last_tick >= 5000) {
        s_last_tick = now;
        s_heartbeat_count++;
        LOG_I("Heartbeat #%lu - WiFi: %s",
              (unsigned long)s_heartbeat_count,
              WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
    }
}
