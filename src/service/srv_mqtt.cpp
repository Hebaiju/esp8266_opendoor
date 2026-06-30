#include "srv_mqtt.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../service/srv_wifi.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>

typedef enum {
    MQTT_STATE_DISCONNECTED = 0,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
} mqtt_state_t;

static WiFiClient s_wifi_client;
static mqtt_state_t s_state = MQTT_STATE_DISCONNECTED;
static uint32_t s_last_try = 0;
static uint32_t s_last_ping = 0;
static mqtt_msg_cb_t s_msg_cb = NULL;

static char s_host[EEPROM_MQTT_HOST_MAX + 1] = {0};
static char s_port[EEPROM_MQTT_PORT_MAX + 1] = {0};
static char s_user[EEPROM_MQTT_USER_MAX + 1] = {0};
static char s_pass[EEPROM_MQTT_PASS_MAX + 1] = {0};
static char s_topic[EEPROM_MQTT_TOPIC_MAX + 1] = {0};
static char s_pub_topic[EEPROM_MQTT_TOPIC_MAX + 1] = {0};
static char s_client_id[EEPROM_MQTT_CLIENT_MAX + 1] = {0};
static bool s_has_config = false;

typedef enum {
    PKT_STATE_HDR = 0,
    PKT_STATE_RLEN,
    PKT_STATE_PAYLOAD,
} pkt_state_t;

static pkt_state_t s_pkt_state = PKT_STATE_HDR;
static uint8_t s_pkt_hdr = 0;
static uint8_t s_pkt_type = 0;
static uint32_t s_pkt_rlen = 0;
static uint32_t s_pkt_mult = 1;
static uint8_t *s_pkt_buf = NULL;
static uint32_t s_pkt_got = 0;

static uint16_t _mqtt_encode_len(uint8_t *buf, uint32_t len)
{
    uint16_t idx = 0;
    do {
        uint8_t b = len % 128;
        len = len / 128;
        if (len > 0) b |= 0x80;
        buf[idx++] = b;
    } while (len > 0);
    return idx;
}

static bool _mqtt_send_packet(uint8_t type, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t hdr[5];
    uint16_t hdr_len = 1 + _mqtt_encode_len(hdr + 1, payload_len);
    hdr[0] = type << 4;

    if (s_wifi_client.write(hdr, hdr_len) != hdr_len) return false;
    if (payload_len > 0) {
        if (s_wifi_client.write(payload, payload_len) != payload_len) return false;
    }
    s_wifi_client.flush();
    return true;
}

static bool _mqtt_connect(void)
{
    if (strlen(s_host) == 0) return false;

    int port_num = atoi(s_port);
    if (port_num <= 0 || port_num > 65535) port_num = 1883;

    if (!s_wifi_client.connect(s_host, port_num)) {
        LOG_W("MQTT连接失败: %s:%d", s_host, port_num);
        return false;
    }

    uint8_t pkt[256];
    uint16_t pos = 0;

    pkt[pos++] = 0x00;
    pkt[pos++] = 0x04;
    pkt[pos++] = 'M';
    pkt[pos++] = 'Q';
    pkt[pos++] = 'T';
    pkt[pos++] = 'T';
    pkt[pos++] = 0x04;

    uint8_t flags = 0x02;
    bool has_user = strlen(s_user) > 0;
    bool has_pass = strlen(s_pass) > 0;
    if (has_user) flags |= 0x80;
    if (has_pass) flags |= 0xC0;
    pkt[pos++] = flags;

    pkt[pos++] = (MQTT_KEEPALIVE_SEC >> 8) & 0xFF;
    pkt[pos++] = MQTT_KEEPALIVE_SEC & 0xFF;

    const char *cid = (strlen(s_client_id) > 0) ? s_client_id : "esp8266-opendoor";
    uint16_t cid_len = strlen(cid);
    pkt[pos++] = (cid_len >> 8) & 0xFF;
    pkt[pos++] = cid_len & 0xFF;
    memcpy(pkt + pos, cid, cid_len);
    pos += cid_len;

    if (has_user) {
        uint16_t ulen = strlen(s_user);
        pkt[pos++] = (ulen >> 8) & 0xFF;
        pkt[pos++] = ulen & 0xFF;
        memcpy(pkt + pos, s_user, ulen);
        pos += ulen;
    }
    if (has_pass) {
        uint16_t plen = strlen(s_pass);
        pkt[pos++] = (plen >> 8) & 0xFF;
        pkt[pos++] = plen & 0xFF;
        memcpy(pkt + pos, s_pass, plen);
        pos += plen;
    }

    if (!_mqtt_send_packet(1, pkt, pos)) {
        s_wifi_client.stop();
        return false;
    }

    uint32_t start = millis();
    while (s_wifi_client.available() < 4 && millis() - start < 3000) {
        delay(10);
    }

    if (s_wifi_client.available() < 4) {
        LOG_W("MQTT连接超时");
        s_wifi_client.stop();
        return false;
    }

    uint8_t resp[4];
    s_wifi_client.read(resp, 4);

    if (resp[0] != 0x20 || resp[3] != 0x00) {
        LOG_W("MQTT连接被拒绝: %d", resp[3]);
        s_wifi_client.stop();
        return false;
    }

    if (strlen(s_topic) > 0) {
        uint8_t sub_pkt[128];
        uint16_t sub_pos = 0;
        sub_pkt[sub_pos++] = 0x00;
        sub_pkt[sub_pos++] = 0x01;
        uint16_t tlen = strlen(s_topic);
        sub_pkt[sub_pos++] = (tlen >> 8) & 0xFF;
        sub_pkt[sub_pos++] = tlen & 0xFF;
        memcpy(sub_pkt + sub_pos, s_topic, tlen);
        sub_pos += tlen;
        sub_pkt[sub_pos++] = 0x00;

        _mqtt_send_packet(8, sub_pkt, sub_pos);
    }

    LOG_I("MQTT连接成功: %s:%d", s_host, port_num);
    return true;
}

static void _mqtt_disconnect(void)
{
    uint8_t pkt = 0;
    _mqtt_send_packet(14, &pkt, 0);
    s_wifi_client.stop();
    s_state = MQTT_STATE_DISCONNECTED;
    s_pkt_state = PKT_STATE_HDR;
    if (s_pkt_buf) {
        free(s_pkt_buf);
        s_pkt_buf = NULL;
    }
}

static void _mqtt_handle_packet(void)
{
    while (s_wifi_client.available() > 0) {
        switch (s_pkt_state) {
            case PKT_STATE_HDR: {
                s_pkt_hdr = s_wifi_client.read();
                s_pkt_type = (s_pkt_hdr >> 4) & 0x0F;
                s_pkt_rlen = 0;
                s_pkt_mult = 1;
                s_pkt_state = PKT_STATE_RLEN;
                break;
            }

            case PKT_STATE_RLEN: {
                uint8_t enc = s_wifi_client.read();
                s_pkt_rlen += (enc & 0x7F) * s_pkt_mult;
                s_pkt_mult *= 128;
                if ((enc & 0x80) == 0 || s_pkt_mult > 128 * 128 * 128) {
                    if (s_pkt_rlen == 0) {
                        if (s_pkt_type == 2) {
                            LOG_I("MQTT收到CONNACK");
                        } else if (s_pkt_type == 4) {
                            LOG_I("MQTT收到PUBACK");
                        } else if (s_pkt_type == 9) {
                            LOG_I("MQTT收到SUBACK");
                        } else if (s_pkt_type == 13) {
                        } else {
                            LOG_I("MQTT收到数据包: type=%d, rlen=0", s_pkt_type);
                        }
                        s_pkt_state = PKT_STATE_HDR;
                    } else {
                        if (s_pkt_rlen > 2048) {
                            LOG_W("MQTT数据包过大: %d，丢弃", s_pkt_rlen);
                            s_pkt_state = PKT_STATE_HDR;
                            return;
                        }
                        s_pkt_buf = (uint8_t *)malloc(s_pkt_rlen);
                        if (!s_pkt_buf) {
                            LOG_W("MQTT内存分配失败");
                            s_pkt_state = PKT_STATE_HDR;
                            return;
                        }
                        s_pkt_got = 0;
                        s_pkt_state = PKT_STATE_PAYLOAD;
                    }
                }
                break;
            }

            case PKT_STATE_PAYLOAD: {
                int avail = s_wifi_client.available();
                int need = s_pkt_rlen - s_pkt_got;
                int rd = s_wifi_client.read(s_pkt_buf + s_pkt_got, need > avail ? avail : need);
                if (rd > 0) s_pkt_got += rd;

                if (s_pkt_got >= s_pkt_rlen) {
                    if (s_pkt_type == 3 && s_msg_cb) {
                        uint16_t topic_len = (s_pkt_buf[0] << 8) | s_pkt_buf[1];
                        if (topic_len < 128 && topic_len <= s_pkt_rlen - 2) {
                            char topic[128];
                            memcpy(topic, s_pkt_buf + 2, topic_len);
                            topic[topic_len] = '\0';
                            int payload_len = s_pkt_rlen - 2 - topic_len;
                            if (payload_len < 0) payload_len = 0;
                            if (payload_len < 512) {
                                char *payload = (char *)malloc(payload_len + 1);
                                if (payload) {
                                    memcpy(payload, s_pkt_buf + 2 + topic_len, payload_len);
                                    payload[payload_len] = '\0';
                                    LOG_I("MQTT收到消息: topic=%s, payload=%s", topic, payload);
                                    s_msg_cb(topic, payload, payload_len);
                                    free(payload);
                                }
                            }
                        }
                    } else if (s_pkt_type == 2) {
                        uint8_t rc = s_pkt_buf[1];
                        if (rc == 0) {
                            LOG_I("MQTT CONNACK: 连接接受");
                        } else {
                            LOG_W("MQTT CONNACK: 拒绝, code=%d", rc);
                        }
                    } else if (s_pkt_type == 9) {
                        LOG_I("MQTT SUBACK: 订阅成功");
                    } else {
                        LOG_I("MQTT收到数据包: type=%d, rlen=%d", s_pkt_type, s_pkt_rlen);
                    }

                    free(s_pkt_buf);
                    s_pkt_buf = NULL;
                    s_pkt_state = PKT_STATE_HDR;
                }
                break;
            }

            default:
                s_pkt_state = PKT_STATE_HDR;
                break;
        }
    }
}

err_code_t srv_mqtt_init(void)
{
    char tmp_host[EEPROM_MQTT_HOST_MAX + 1] = {0};
    char tmp_port[EEPROM_MQTT_PORT_MAX + 1] = {0};
    char tmp_user[EEPROM_MQTT_USER_MAX + 1] = {0};
    char tmp_pass[EEPROM_MQTT_PASS_MAX + 1] = {0};
    char tmp_topic[EEPROM_MQTT_TOPIC_MAX + 1] = {0};
    char tmp_client[EEPROM_MQTT_CLIENT_MAX + 1] = {0};

    if (srv_mqtt_load_config(
        tmp_host, sizeof(tmp_host),
        tmp_port, sizeof(tmp_port),
        tmp_user, sizeof(tmp_user),
        tmp_pass, sizeof(tmp_pass),
        tmp_topic, sizeof(tmp_topic),
        tmp_client, sizeof(tmp_client)
    )) {
        strncpy(s_host, tmp_host, sizeof(s_host));
        strncpy(s_port, tmp_port, sizeof(s_port));
        strncpy(s_user, tmp_user, sizeof(s_user));
        strncpy(s_pass, tmp_pass, sizeof(s_pass));
        strncpy(s_topic, tmp_topic, sizeof(s_topic));
        strncpy(s_client_id, tmp_client, sizeof(s_client_id));
        s_has_config = true;
        LOG_I("MQTT配置: 已加载");
    } else {
        /* 使用默认配置 */
        strncpy(s_host, MQTT_DEFAULT_HOST, sizeof(s_host));
        strncpy(s_port, MQTT_DEFAULT_PORT, sizeof(s_port));
        s_user[0] = '\0';
        s_pass[0] = '\0';
        strncpy(s_topic, MQTT_DEFAULT_TOPIC, sizeof(s_topic));
        strncpy(s_pub_topic, MQTT_DEFAULT_PUB_TOPIC, sizeof(s_pub_topic));
        s_client_id[0] = '\0';
        s_has_config = true;
        LOG_I("MQTT配置: 使用默认配置");
    }

    LOG_I("  服务器: %s:%s", s_host, s_port);
    LOG_I("  用户: %s", strlen(s_user) > 0 ? s_user : "(空)");
    LOG_I("  订阅主题: %s", s_topic);
    LOG_I("  发布主题: %s", s_pub_topic);
    LOG_I("  客户端ID: %s", strlen(s_client_id) > 0 ? s_client_id : "(自动生成)");

    s_state = MQTT_STATE_DISCONNECTED;
    return APP_OK;
}

void srv_mqtt_run(void)
{
    if (!srv_wifi_is_connected() || !s_has_config) {
        if (s_state != MQTT_STATE_DISCONNECTED) {
            _mqtt_disconnect();
        }
        return;
    }

    switch (s_state) {
        case MQTT_STATE_DISCONNECTED:
            if (millis() - s_last_try > MQTT_RETRY_INTERVAL_MS) {
                s_last_try = millis();
                s_state = MQTT_STATE_CONNECTING;
                LOG_I("MQTT正在连接: %s:%s ...", s_host, s_port);
                if (_mqtt_connect()) {
                    s_state = MQTT_STATE_CONNECTED;
                    s_last_ping = millis();
                    LOG_I("MQTT连接成功! 订阅主题: %s", s_topic);
                } else {
                    s_state = MQTT_STATE_DISCONNECTED;
                    LOG_W("MQTT连接失败，10秒后重试");
                }
            }
            break;

        case MQTT_STATE_CONNECTED:
            if (!s_wifi_client.connected()) {
                LOG_W("MQTT连接断开");
                s_wifi_client.stop();
                s_state = MQTT_STATE_DISCONNECTED;
                s_last_try = millis();
                break;
            }

            _mqtt_handle_packet();

            if (millis() - s_last_ping > MQTT_PING_INTERVAL_MS) {
                _mqtt_send_packet(12, NULL, 0);
                s_last_ping = millis();
            }
            break;

        default:
            break;
    }
}

bool srv_mqtt_is_connected(void)
{
    return s_state == MQTT_STATE_CONNECTED && s_wifi_client.connected();
}

bool srv_mqtt_has_config(void)
{
    return s_has_config;
}

void srv_mqtt_set_msg_callback(mqtt_msg_cb_t cb)
{
    s_msg_cb = cb;
}

bool srv_mqtt_load_config(
    char *host_buf, int host_len,
    char *port_buf, int port_len,
    char *user_buf, int user_len,
    char *pass_buf, int pass_len,
    char *topic_buf, int topic_len,
    char *client_buf, int client_len
) {
    if (host_buf == NULL) return false;

    EEPROM.begin(EEPROM_TOTAL_SIZE);
    uint8_t magic = EEPROM.read(EEPROM_MQTT_ADDR);
    if (magic != EEPROM_MQTT_MAGIC) {
        EEPROM.end();
        return false;
    }

    int addr = EEPROM_MQTT_ADDR + 1;
    int i;

    for (i = 0; i < EEPROM_MQTT_HOST_MAX && i < host_len - 1; i++) {
        char c = EEPROM.read(addr++);
        if (c == '\0') break;
        host_buf[i] = c;
    }
    host_buf[i] = '\0';

    for (i = 0; i < EEPROM_MQTT_PORT_MAX && i < port_len - 1; i++) {
        char c = EEPROM.read(addr++);
        if (c == '\0') break;
        port_buf[i] = c;
    }
    port_buf[i] = '\0';

    for (i = 0; i < EEPROM_MQTT_USER_MAX && i < user_len - 1; i++) {
        char c = EEPROM.read(addr++);
        if (c == '\0') break;
        user_buf[i] = c;
    }
    user_buf[i] = '\0';

    for (i = 0; i < EEPROM_MQTT_PASS_MAX && i < pass_len - 1; i++) {
        char c = EEPROM.read(addr++);
        if (c == '\0') break;
        pass_buf[i] = c;
    }
    pass_buf[i] = '\0';

    for (i = 0; i < EEPROM_MQTT_TOPIC_MAX && i < topic_len - 1; i++) {
        char c = EEPROM.read(addr++);
        if (c == '\0') break;
        topic_buf[i] = c;
    }
    topic_buf[i] = '\0';

    for (i = 0; i < EEPROM_MQTT_CLIENT_MAX && i < client_len - 1; i++) {
        char c = EEPROM.read(addr++);
        if (c == '\0') break;
        client_buf[i] = c;
    }
    client_buf[i] = '\0';

    EEPROM.end();

    if (strlen(host_buf) == 0) return false;
    return true;
}

static void _eeprom_write_str(int *addr, const char *str, int max_len)
{
    int len = strlen(str);
    if (len > max_len) len = max_len;
    for (int i = 0; i < len; i++) {
        EEPROM.write((*addr)++, str[i]);
    }
    EEPROM.write((*addr)++, '\0');
}

bool srv_mqtt_save_config(
    const char *host,
    const char *port,
    const char *user,
    const char *pass,
    const char *topic,
    const char *client_id
) {
    if (host == NULL || strlen(host) == 0) return false;

    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.write(EEPROM_MQTT_ADDR, EEPROM_MQTT_MAGIC);
    int addr = EEPROM_MQTT_ADDR + 1;

    _eeprom_write_str(&addr, host, EEPROM_MQTT_HOST_MAX);
    _eeprom_write_str(&addr, port ? port : "1883", EEPROM_MQTT_PORT_MAX);
    _eeprom_write_str(&addr, user ? user : "", EEPROM_MQTT_USER_MAX);
    _eeprom_write_str(&addr, pass ? pass : "", EEPROM_MQTT_PASS_MAX);
    _eeprom_write_str(&addr, topic ? topic : "", EEPROM_MQTT_TOPIC_MAX);
    _eeprom_write_str(&addr, client_id ? client_id : "", EEPROM_MQTT_CLIENT_MAX);

    EEPROM.commit();
    EEPROM.end();

    strncpy(s_host, host, sizeof(s_host));
    if (port) strncpy(s_port, port, sizeof(s_port));
    if (user) strncpy(s_user, user, sizeof(s_user));
    if (pass) strncpy(s_pass, pass, sizeof(s_pass));
    if (topic) strncpy(s_topic, topic, sizeof(s_topic));
    if (client_id) strncpy(s_client_id, client_id, sizeof(s_client_id));
    /* 发布主题固定 */
    strncpy(s_pub_topic, MQTT_DEFAULT_PUB_TOPIC, sizeof(s_pub_topic));
    s_has_config = true;

    LOG_I("MQTT配置: 已保存 (%s:%s)", host, port ? port : "1883");
    LOG_I("  订阅主题: %s", s_topic);
    LOG_I("  发布主题: %s", s_pub_topic);
    return true;
}

void srv_mqtt_clear_config(void)
{
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.write(EEPROM_MQTT_ADDR, 0x00);
    EEPROM.commit();
    EEPROM.end();

    if (s_state != MQTT_STATE_DISCONNECTED) {
        _mqtt_disconnect();
    }

    /* 恢复默认配置 */
    strncpy(s_host, MQTT_DEFAULT_HOST, sizeof(s_host));
    strncpy(s_port, MQTT_DEFAULT_PORT, sizeof(s_port));
    s_user[0] = '\0';
    s_pass[0] = '\0';
    strncpy(s_topic, MQTT_DEFAULT_TOPIC, sizeof(s_topic));
    strncpy(s_pub_topic, MQTT_DEFAULT_PUB_TOPIC, sizeof(s_pub_topic));
    s_client_id[0] = '\0';
    s_has_config = true;

    LOG_I("MQTT配置: 已恢复默认");
}

const char *srv_mqtt_get_host(void)
{
    return s_host;
}

const char *srv_mqtt_get_port(void)
{
    return s_port;
}

const char *srv_mqtt_get_user(void)
{
    return s_user;
}

const char *srv_mqtt_get_topic(void)
{
    return s_topic;
}

const char *srv_mqtt_get_client_id(void)
{
    return s_client_id;
}

const char *srv_mqtt_get_pub_topic(void)
{
    return s_pub_topic;
}

bool srv_mqtt_publish(const char *topic, const char *payload)
{
    if (!srv_mqtt_is_connected() || !topic || !payload) return false;

    uint16_t topic_len = strlen(topic);
    uint16_t payload_len = strlen(payload);
    uint32_t total_len = 2 + topic_len + payload_len;

    uint8_t hdr[5];
    uint16_t hdr_len = 1 + _mqtt_encode_len(hdr + 1, total_len);
    hdr[0] = 0x30; /* PUBLISH, QoS 0 */

    if (s_wifi_client.write(hdr, hdr_len) != hdr_len) return false;

    uint8_t tl[2] = { (uint8_t)(topic_len >> 8), (uint8_t)(topic_len & 0xFF) };
    if (s_wifi_client.write(tl, 2) != 2) return false;
    if (s_wifi_client.write((const uint8_t *)topic, topic_len) != topic_len) return false;
    if (s_wifi_client.write((const uint8_t *)payload, payload_len) != payload_len) return false;
    s_wifi_client.flush();

    return true;
}
