#include "srv_ntp.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../service/srv_wifi.h"
#include <ESP8266WiFi.h>

typedef enum {
    NTP_STATE_IDLE = 0,
    NTP_STATE_WAIT_WIFI,
    NTP_STATE_SYNCING,
    NTP_STATE_SYNCED,
    NTP_STATE_FAIL,
} ntp_state_t;

static ntp_state_t s_state = NTP_STATE_IDLE;
static uint32_t s_last_sync = 0;
static uint32_t s_sync_start = 0;

#define NTP_SYNC_TIMEOUT_MS    15000

static void ntp_do_sync(void)
{
    LOG_I("正在同步NTP时间, 服务器: %s, 时区: %d", NTP_SERVER, NTP_TIMEZONE);
    configTime(NTP_TIMEZONE * 3600, 0, NTP_SERVER, "pool.ntp.org", "time.nist.gov");
    s_state = NTP_STATE_SYNCING;
    s_sync_start = millis();
}

err_code_t srv_ntp_init(void)
{
    s_state = NTP_STATE_WAIT_WIFI;
    s_last_sync = 0;
    LOG_I("NTP服务初始化");
    return APP_OK;
}

void srv_ntp_run(void)
{
    uint32_t now = millis();
    time_t t_now;
    struct tm t_tm;

    switch (s_state) {
        case NTP_STATE_WAIT_WIFI:
            if (srv_wifi_is_connected()) {
                ntp_do_sync();
            }
            break;

        case NTP_STATE_SYNCING:
            t_now = time(NULL);
            if (t_now > 100000) {
                s_state = NTP_STATE_SYNCED;
                s_last_sync = now;
                localtime_r(&t_now, &t_tm);
                LOG_I("NTP时间同步成功! %04d-%02d-%02d %02d:%02d:%02d",
                      t_tm.tm_year + 1900, t_tm.tm_mon + 1, t_tm.tm_mday,
                      t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec);
            } else if (now - s_sync_start >= NTP_SYNC_TIMEOUT_MS) {
                s_state = NTP_STATE_FAIL;
                s_last_sync = now;
                LOG_W("NTP同步超时, %ds后重试", NTP_RETRY_INTERVAL_MS / 1000);
            }
            break;

        case NTP_STATE_SYNCED:
            if (!srv_wifi_is_connected()) {
                s_state = NTP_STATE_WAIT_WIFI;
                LOG_W("WiFi断开，NTP暂停");
            } else if (now - s_last_sync >= NTP_SYNC_INTERVAL_MS) {
                ntp_do_sync();
            }
            break;

        case NTP_STATE_FAIL:
            if (srv_wifi_is_connected() &&
                now - s_last_sync >= NTP_RETRY_INTERVAL_MS) {
                ntp_do_sync();
            }
            break;

        default:
            break;
    }
}

bool srv_ntp_is_synced(void)
{
    return s_state == NTP_STATE_SYNCED;
}

bool srv_ntp_get_time(struct tm *out_tm)
{
    if (s_state != NTP_STATE_SYNCED || out_tm == NULL) {
        return false;
    }
    time_t t_now = time(NULL);
    localtime_r(&t_now, out_tm);
    return true;
}

uint8_t srv_ntp_get_hour(void)
{
    if (s_state != NTP_STATE_SYNCED) {
        return 255;
    }
    time_t t_now = time(NULL);
    struct tm t_tm;
    localtime_r(&t_now, &t_tm);
    return (uint8_t)t_tm.tm_hour;
}
