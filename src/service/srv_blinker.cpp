#define BLINKER_WIFI
#include "srv_blinker.h"
#include "../common/log.h"
#include "../service/srv_wifi.h"
#include <Blinker.h>

typedef enum {
    BLINKER_STATE_WAIT_WIFI = 0,
    BLINKER_STATE_INIT,
    BLINKER_STATE_CONNECTING,
    BLINKER_STATE_READY,
    BLINKER_STATE_FAIL,
} blinker_state_t;

static blinker_btn_cb_t s_open_cb = NULL;
static blinker_state_t s_state = BLINKER_STATE_WAIT_WIFI;
static BlinkerButton *s_btn_open = NULL;
static uint32_t s_start_time = 0;
static uint32_t s_retry_count = 0;
static bool s_button_registered = false;

#define BLINKER_INIT_DELAY_MS      2000
#define BLINKER_CONNECT_TIMEOUT_MS 30000
#define BLINKER_RETRY_INTERVAL_MS  15000

static void btn_open_callback(const String & state)
{
    LOG_I("Blinker button %s: %s", BLINKER_BUTTON_OPEN, state.c_str());
    if (state == "tap" && s_open_cb != NULL) {
        s_open_cb();
    }
}

err_code_t srv_blinker_init(blinker_btn_cb_t open_cb)
{
    s_open_cb = open_cb;
    s_state = BLINKER_STATE_WAIT_WIFI;
    s_button_registered = false;
    s_retry_count = 0;
    LOG_I("Blinker service init, waiting for WiFi");
    return APP_OK;
}

static void blinker_do_connect(void)
{
    s_retry_count++;
    LOG_I("Blinker connecting ... (attempt #%lu)", (unsigned long)s_retry_count);
    Blinker.begin(BLINKER_AUTH, WIFI_SSID, WIFI_PASSWORD);

    if (!s_button_registered) {
        s_btn_open = new BlinkerButton(BLINKER_BUTTON_OPEN);
        s_btn_open->attach(btn_open_callback);
        s_button_registered = true;
        LOG_I("Blinker button registered: %s", BLINKER_BUTTON_OPEN);
    }

    s_state = BLINKER_STATE_CONNECTING;
    s_start_time = millis();
}

void srv_blinker_run(void)
{
    uint32_t now = millis();

    switch (s_state) {
        case BLINKER_STATE_WAIT_WIFI:
            if (srv_wifi_is_connected()) {
                s_state = BLINKER_STATE_INIT;
                s_start_time = millis();
                LOG_I("WiFi ready, Blinker will init in 2s");
            }
            break;

        case BLINKER_STATE_INIT:
            if (now - s_start_time >= BLINKER_INIT_DELAY_MS) {
                blinker_do_connect();
            }
            break;

        case BLINKER_STATE_CONNECTING:
            if (srv_wifi_is_connected()) {
                if (Blinker.connected()) {
                    s_state = BLINKER_STATE_READY;
                    LOG_I("Blinker connected! Server online, MQTT alive");
                }
            }
            if (now - s_start_time >= BLINKER_CONNECT_TIMEOUT_MS) {
                s_state = BLINKER_STATE_FAIL;
                s_start_time = now;
                LOG_W("Blinker connect timeout, retry in %ds",
                      BLINKER_RETRY_INTERVAL_MS / 1000);
            }
            Blinker.run();
            break;

        case BLINKER_STATE_READY:
            if (!srv_wifi_is_connected()) {
                s_state = BLINKER_STATE_FAIL;
                s_start_time = now;
                LOG_W("WiFi lost, Blinker paused");
            } else {
                Blinker.run();
            }
            break;

        case BLINKER_STATE_FAIL:
            if (srv_wifi_is_connected() &&
                now - s_start_time >= BLINKER_RETRY_INTERVAL_MS) {
                blinker_do_connect();
            }
            break;

        default:
            break;
    }
}

bool srv_blinker_is_ready(void)
{
    return s_state == BLINKER_STATE_READY;
}
