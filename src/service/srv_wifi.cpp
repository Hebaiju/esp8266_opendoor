#include "srv_wifi.h"
#include "../common/config.h"
#include "../common/log.h"
#include <ESP8266WiFi.h>

static wifi_state_t s_state = WIFI_STATE_IDLE;
static uint32_t s_start_time = 0;
static uint32_t s_last_retry = 0;
static uint32_t s_retry_count = 0;
static uint32_t s_last_check_ms = 0;
static bool s_is_connected = false;

#define WIFI_CONNECT_TIMEOUT_MS    15000
#define WIFI_RETRY_MIN_INTERVAL_MS 5000
#define WIFI_RETRY_MAX_INTERVAL_MS 120000
#define WIFI_CHECK_INTERVAL_MS     5000

static uint32_t wifi_calc_retry_interval(void)
{
    uint32_t interval = WIFI_RETRY_MIN_INTERVAL_MS * (1 << s_retry_count);
    if (interval > WIFI_RETRY_MAX_INTERVAL_MS) {
        interval = WIFI_RETRY_MAX_INTERVAL_MS;
    }
    return interval;
}

static void wifi_event_handler(WiFiEvent_t event)
{
    switch (event) {
        case WIFI_EVENT_STAMODE_CONNECTED:
            LOG_I("WiFi事件: 已连接到热点");
            break;

        case WIFI_EVENT_STAMODE_DISCONNECTED:
            s_state = WIFI_STATE_DISCONNECTED;
            s_last_retry = millis();
            LOG_W("WiFi事件: 与热点断开连接");
            break;

        case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
            LOG_I("WiFi事件: 认证模式变更");
            break;

        case WIFI_EVENT_STAMODE_GOT_IP:
            s_state = WIFI_STATE_CONNECTED;
            s_is_connected = true;
            s_retry_count = 0;
            LOG_I("WiFi连接成功! IP: %s, 信号强度: %d dBm",
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
            break;

        case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
            s_state = WIFI_STATE_DISCONNECTED;
            s_last_retry = millis();
            LOG_W("WiFi事件: DHCP超时");
            break;

        default:
            LOG_I("WiFi事件: %d", (int)event);
            break;
    }
}

static void wifi_start_connect(void)
{
    uint32_t interval = wifi_calc_retry_interval();
    LOG_I("正在连接WiFi %s ... (重试 #%lu, 间隔=%lus)",
          WIFI_SSID, (unsigned long)s_retry_count, (unsigned long)interval / 1000);

    WiFi.setAutoReconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    s_state = WIFI_STATE_CONNECTING;
    s_start_time = millis();
    s_retry_count++;
}

void srv_wifi_init(void)
{
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.onEvent(wifi_event_handler);

    s_state = WIFI_STATE_IDLE;
    s_start_time = millis();
    s_retry_count = 0;
    s_last_check_ms = 0;
    s_is_connected = false;

    LOG_I("WiFi服务初始化, SSID: %s", WIFI_SSID);
}

static void wifi_check_connection(void)
{
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED) {
        if (!s_is_connected) {
            s_state = WIFI_STATE_CONNECTED;
            s_is_connected = true;
            s_retry_count = 0;
            LOG_I("WiFi连接成功! IP: %s, 信号强度: %d dBm",
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
        }
    } else {
        if (s_is_connected) {
            s_is_connected = false;
            s_state = WIFI_STATE_DISCONNECTED;
            s_last_retry = millis();
            LOG_W("WiFi断开连接, 状态=%d, 将自动重试", (int)status);
        }
    }
}

void srv_wifi_run(void)
{
    uint32_t now = millis();

    if (now - s_last_check_ms >= WIFI_CHECK_INTERVAL_MS) {
        s_last_check_ms = now;
        wifi_check_connection();
    }

    switch (s_state) {
        case WIFI_STATE_IDLE:
            if (now - s_start_time >= 1000) {
                wifi_start_connect();
            }
            break;

        case WIFI_STATE_CONNECTING: {
            if (s_is_connected) {
                s_state = WIFI_STATE_CONNECTED;
            } else if (now - s_start_time >= WIFI_CONNECT_TIMEOUT_MS) {
                s_state = WIFI_STATE_DISCONNECTED;
                s_last_retry = now;
                uint32_t interval = wifi_calc_retry_interval();
                LOG_W("WiFi连接超时, %lus后重试",
                      (unsigned long)interval / 1000);
            }
            break;
        }

        case WIFI_STATE_CONNECTED:
            break;

        case WIFI_STATE_DISCONNECTED: {
            uint32_t interval = wifi_calc_retry_interval();
            if (now - s_last_retry >= interval) {
                wifi_start_connect();
            }
            break;
        }

        default:
            break;
    }
}

bool srv_wifi_is_connected(void)
{
    return s_is_connected && WiFi.status() == WL_CONNECTED;
}

wifi_state_t srv_wifi_get_state(void)
{
    return s_state;
}

int srv_wifi_get_rssi(void)
{
    return WiFi.RSSI();
}

const char *srv_wifi_get_ip(void)
{
    static char ip_str[16];
    if (s_is_connected) {
        strncpy(ip_str, WiFi.localIP().toString().c_str(), sizeof(ip_str) - 1);
        ip_str[sizeof(ip_str) - 1] = '\0';
        return ip_str;
    }
    return "0.0.0.0";
}
