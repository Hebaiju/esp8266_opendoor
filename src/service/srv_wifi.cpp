#include "srv_wifi.h"
#include "../common/config.h"
#include "../common/log.h"
#include <ESP8266WiFi.h>

static wifi_state_t s_state = WIFI_STATE_IDLE;
static uint32_t s_start_time = 0;
static uint32_t s_last_retry = 0;
static uint32_t s_retry_count = 0;

#define WIFI_CONNECT_TIMEOUT_MS  15000
#define WIFI_RETRY_INTERVAL_MS   10000

void srv_wifi_init(void)
{
    s_state = WIFI_STATE_IDLE;
    s_start_time = millis();
    s_retry_count = 0;
    LOG_I("WiFi service init, SSID: %s", WIFI_SSID);
}

static void wifi_start_connect(void)
{
    LOG_I("WiFi connecting to %s ... (retry #%lu)",
          WIFI_SSID, (unsigned long)s_retry_count);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    s_state = WIFI_STATE_CONNECTING;
    s_start_time = millis();
    s_retry_count++;
}

void srv_wifi_run(void)
{
    uint32_t now = millis();

    switch (s_state) {
        case WIFI_STATE_IDLE:
            if (now - s_start_time >= 1000) {
                wifi_start_connect();
            }
            break;

        case WIFI_STATE_CONNECTING: {
            wl_status_t status = WiFi.status();
            if (status == WL_CONNECTED) {
                s_state = WIFI_STATE_CONNECTED;
                LOG_I("WiFi connected! IP: %s, RSSI: %d dBm",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
            } else if (now - s_start_time >= WIFI_CONNECT_TIMEOUT_MS) {
                s_state = WIFI_STATE_DISCONNECTED;
                s_last_retry = now;
                LOG_W("WiFi connect timeout, status=%d, will retry in %ds",
                      status, WIFI_RETRY_INTERVAL_MS / 1000);
            }
            break;
        }

        case WIFI_STATE_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                s_state = WIFI_STATE_DISCONNECTED;
                s_last_retry = now;
                LOG_W("WiFi disconnected, will retry in %ds",
                      WIFI_RETRY_INTERVAL_MS / 1000);
            }
            break;

        case WIFI_STATE_DISCONNECTED:
            if (now - s_last_retry >= WIFI_RETRY_INTERVAL_MS) {
                wifi_start_connect();
            }
            break;

        default:
            break;
    }
}

bool srv_wifi_is_connected(void)
{
    return s_state == WIFI_STATE_CONNECTED;
}

wifi_state_t srv_wifi_get_state(void)
{
    return s_state;
}
