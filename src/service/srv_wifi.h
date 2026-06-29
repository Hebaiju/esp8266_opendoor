#ifndef __SRV_WIFI_H__
#define __SRV_WIFI_H__

#include "../common/err_code.h"

typedef enum {
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
} wifi_state_t;

void srv_wifi_init(void);
void srv_wifi_run(void);
bool srv_wifi_is_connected(void);
wifi_state_t srv_wifi_get_state(void);
int srv_wifi_get_rssi(void);
const char *srv_wifi_get_ip(void);

#endif
