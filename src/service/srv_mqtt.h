#ifndef __SRV_MQTT_H__
#define __SRV_MQTT_H__

#include <Arduino.h>
#include "../common/err_code.h"
#include "../common/config.h"

typedef void (*mqtt_msg_cb_t)(const char *topic, const char *payload, int len);

err_code_t srv_mqtt_init(void);
void srv_mqtt_run(void);
bool srv_mqtt_is_connected(void);
bool srv_mqtt_has_config(void);
void srv_mqtt_set_msg_callback(mqtt_msg_cb_t cb);

bool srv_mqtt_load_config(
    char *host_buf, int host_len,
    char *port_buf, int port_len,
    char *user_buf, int user_len,
    char *pass_buf, int pass_len,
    char *topic_buf, int topic_len,
    char *client_buf, int client_len
);
bool srv_mqtt_save_config(
    const char *host,
    const char *port,
    const char *user,
    const char *pass,
    const char *topic,
    const char *client_id
);
void srv_mqtt_clear_config(void);

const char *srv_mqtt_get_host(void);
const char *srv_mqtt_get_port(void);
const char *srv_mqtt_get_user(void);
const char *srv_mqtt_get_topic(void);
const char *srv_mqtt_get_client_id(void);
const char *srv_mqtt_get_pub_topic(void);

bool srv_mqtt_publish(const char *topic, const char *payload);

#endif
