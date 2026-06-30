#ifndef __SRV_WIFI_H__
#define __SRV_WIFI_H__

#include "../common/err_code.h"
#include <stdint.h>

typedef enum {
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
} wifi_state_t;

/* WiFi配置结构体 */
typedef struct {
    char ssid[32];
    char pass[64];
    uint8_t magic;
} wifi_config_t;

void srv_wifi_init(void);
void srv_wifi_run(void);
bool srv_wifi_is_connected(void);
wifi_state_t srv_wifi_get_state(void);
int srv_wifi_get_rssi(void);
const char *srv_wifi_get_ip(void);
const char *srv_wifi_get_ap_ip(void);
const char *srv_wifi_get_ssid(void);
const char *srv_wifi_get_password(void);

/* WiFi配置持久化 */
bool srv_wifi_load_config(wifi_config_t *cfg);
void srv_wifi_save_config(const wifi_config_t *cfg);
bool srv_wifi_has_config(void);
void srv_wifi_clear_config(void);

/* AP模式 */
void srv_wifi_start_ap(void);
bool srv_wifi_is_ap_active(void);

/* WiFi扫描 */
int srv_wifi_scan_start(void);
const char *srv_wifi_scan_ssid(int index);
int srv_wifi_scan_rssi(int index);
void srv_wifi_scan_free(void);
const char *srv_wifi_get_scan_cache(void);

#endif
