#include "srv_wifi.h"
#include "../common/config.h"
#include "../common/log.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>

static wifi_state_t s_state = WIFI_STATE_IDLE;
static uint32_t s_start_time = 0;
static uint32_t s_last_retry = 0;
static uint32_t s_retry_count = 0;
static uint32_t s_last_check_ms = 0;
static bool s_is_connected = false;
static bool s_ap_active = false;
static wifi_config_t s_wifi_cfg = {0};

/* WiFi扫描结果缓存 */
static int s_scan_count = 0;

/* 是否有有效的WiFi配置 */
static bool s_has_wifi_config = false;

/* WiFi连接是否已停止（重试过多） */
static bool s_wifi_stopped = false;

#define WIFI_CONNECT_TIMEOUT_MS    15000
#define WIFI_RETRY_MIN_INTERVAL_MS 5000
#define WIFI_RETRY_MAX_INTERVAL_MS 120000
#define WIFI_CHECK_INTERVAL_MS     5000
#define WIFI_AP_CHECK_INTERVAL_MS  10000
#define WIFI_MAX_RETRY_COUNT       10

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
            break;
    }
}

/* ========== WiFi配置持久化 ========== */

bool srv_wifi_load_config(wifi_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.get(EEPROM_WIFI_ADDR, *cfg);
    EEPROM.end();

    if (cfg->magic != EEPROM_WIFI_MAGIC) {
        return false;
    }
    if (cfg->ssid[0] == '\0' || cfg->ssid[0] == 0xFF) {
        return false;
    }
    return true;
}

void srv_wifi_save_config(const wifi_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.put(EEPROM_WIFI_ADDR, *cfg);
    EEPROM.commit();
    EEPROM.end();
    LOG_I("WiFi配置已保存: SSID=%s", cfg->ssid);
}

bool srv_wifi_has_config(void)
{
    wifi_config_t cfg;
    return srv_wifi_load_config(&cfg);
}

void srv_wifi_clear_config(void)
{
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.write(EEPROM_WIFI_ADDR, 0);
    EEPROM.commit();
    EEPROM.end();
    s_has_wifi_config = false;
    s_wifi_stopped = true;
    WiFi.disconnect();
    LOG_I("WiFi配置已清除，进入纯AP模式");
}

/* ========== AP模式 ========== */

void srv_wifi_start_ap(void)
{
    String apName = String(AP_SSID_PREFIX) + "_" + String(ESP.getChipId(), HEX);
    WiFi.softAP(apName.c_str(), AP_PASSWORD);
    s_ap_active = true;
    LOG_I("AP热点已启动: %s, IP: %s", apName.c_str(),
          WiFi.softAPIP().toString().c_str());
}

static void wifi_check_ap(void)
{
    if (!s_ap_active) {
        return;
    }
    /* 检查AP热点是否还在运行，如果被意外关闭则重新启动 */
    if (WiFi.softAPgetStationNum() == 0 && !WiFi.softAPIP().isSet()) {
        LOG_W("AP热点意外关闭，正在重新启动...");
        String apName = String(AP_SSID_PREFIX) + "_" + String(ESP.getChipId(), HEX);
        WiFi.softAP(apName.c_str(), AP_PASSWORD);
        LOG_I("AP热点已重启: %s", apName.c_str());
    }
}

bool srv_wifi_is_ap_active(void)
{
    return s_ap_active;
}

const char *srv_wifi_get_ap_ip(void)
{
    static char ip_str[16];
    if (s_ap_active) {
        strncpy(ip_str, WiFi.softAPIP().toString().c_str(), sizeof(ip_str) - 1);
        ip_str[sizeof(ip_str) - 1] = '\0';
        return ip_str;
    }
    return "0.0.0.0";
}

const char *srv_wifi_get_ssid(void)
{
    return s_wifi_cfg.ssid;
}

const char *srv_wifi_get_password(void)
{
    return s_wifi_cfg.pass;
}

/* ========== WiFi扫描 ========== */

/* 扫描缓存：避免5秒内重复扫描 */
static String s_scan_cache = "";
static uint32_t s_scan_cache_time = 0;

int srv_wifi_scan_start(void)
{
    /* 有缓存且未过期（5秒内），直接返回 */
    if (s_scan_cache.length() > 0 && millis() - s_scan_cache_time < 5000) {
        LOG_I("WiFi扫描: 使用缓存 (剩余%lus)", (5000 - (millis() - s_scan_cache_time)) / 1000);
        return s_scan_count;
    }

    LOG_I("WiFi扫描开始...");
    int n = WiFi.scanNetworks();
    if (n < 0) {
        s_scan_count = 0;
        s_scan_cache = "[]";
        return 0;
    }

    /* 限制结果数量，防止栈溢出 */
    int cnt = (n > MAX_SCAN_RESULTS) ? MAX_SCAN_RESULTS : n;

    /* 信号强度排序（从强到弱） */
    int indices[MAX_SCAN_RESULTS];
    for (int i = 0; i < cnt; i++) indices[i] = i;
    for (int i = 0; i < cnt - 1; i++) {
        for (int j = 0; j < cnt - i - 1; j++) {
            if (WiFi.RSSI(indices[j]) < WiFi.RSSI(indices[j + 1])) {
                int tmp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = tmp;
            }
        }
    }

    /* 构建JSON */
    String json = "[";
    for (int i = 0; i < cnt; i++) {
        int id = indices[i];
        String ssid = WiFi.SSID(id);
        if (ssid.length() > 0) {
            if (i > 0) json += ",";
            json += "{\"s\":\"" + ssid + "\",\"r\":\"" + String(WiFi.RSSI(id)) + "\"}";
        }
    }
    json += "]";

    s_scan_cache = json;
    s_scan_cache_time = millis();
    s_scan_count = cnt;

    LOG_I("WiFi扫描完成, 发现 %d 个网络", s_scan_count);
    return s_scan_count;
}

const char *srv_wifi_scan_ssid(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return "";
    }
    return WiFi.SSID(index).c_str();
}

int srv_wifi_scan_rssi(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return -100;
    }
    return WiFi.RSSI(index);
}

void srv_wifi_scan_free(void)
{
    WiFi.scanDelete();
    s_scan_count = 0;
}

const char *srv_wifi_get_scan_cache(void)
{
    return s_scan_cache.c_str();
}

/* ========== STA连接 ========== */

static void wifi_start_connect(void)
{
    if (s_wifi_stopped) {
        return;
    }

    if (s_retry_count >= WIFI_MAX_RETRY_COUNT) {
        LOG_W("WiFi连接失败超过%d次，停止连接，保持纯AP模式", WIFI_MAX_RETRY_COUNT);
        s_wifi_stopped = true;
        WiFi.disconnect();
        return;
    }

    uint32_t interval = wifi_calc_retry_interval();
    LOG_I("正在连接WiFi %s ... (重试 #%lu/%d, 间隔=%lus)",
          s_wifi_cfg.ssid, (unsigned long)s_retry_count, WIFI_MAX_RETRY_COUNT,
          (unsigned long)interval / 1000);

    WiFi.setAutoReconnect(false);
    WiFi.begin(s_wifi_cfg.ssid, s_wifi_cfg.pass);

    s_state = WIFI_STATE_CONNECTING;
    s_start_time = millis();
    s_retry_count++;
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

/* ========== 公共接口 ========== */

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
    s_ap_active = false;
    s_has_wifi_config = false;
    s_wifi_stopped = false;
    memset(&s_wifi_cfg, 0, sizeof(s_wifi_cfg));

    /* 首先设置STA+AP并存模式 */
    WiFi.mode(WIFI_AP_STA);
    WiFi.hostname("Open-Door-Cat");

    /* 启动AP热点（长期运行） */
    srv_wifi_start_ap();

    /* 加载WiFi配置，只有有效配置才尝试连接 */
    if (srv_wifi_load_config(&s_wifi_cfg)) {
        s_has_wifi_config = true;
        LOG_I("WiFi配置已加载: %s", s_wifi_cfg.ssid);
        LOG_I("WiFi服务初始化完成, STA+AP并存模式");
    } else {
        s_has_wifi_config = false;
        s_state = WIFI_STATE_IDLE;  /* 保持空闲，不尝试连接 */
        LOG_I("WiFi配置无效，进入纯AP模式等待配网");
        LOG_I("WiFi服务初始化完成, 纯AP模式");
    }
}

void srv_wifi_run(void)
{
    uint32_t now = millis();

    /* 定期检查AP热点状态，确保一直保持活跃 */
    static uint32_t s_last_ap_check = 0;
    if (now - s_last_ap_check >= WIFI_AP_CHECK_INTERVAL_MS) {
        s_last_ap_check = now;
        wifi_check_ap();
    }

    /* 只有有WiFi配置且未停止才尝试连接STA */
    if (!s_has_wifi_config || s_wifi_stopped) {
        return;
    }

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
