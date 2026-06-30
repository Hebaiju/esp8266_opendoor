#define BLINKER_WIFI
#include "srv_blinker.h"
#include "../common/log.h"
#include "../service/srv_wifi.h"
#include "../service/srv_ir_ac.h"
#include <Blinker.h>

typedef enum {
    BLINKER_STATE_WAIT_WIFI = 0,
    BLINKER_STATE_INIT,
    BLINKER_STATE_CONNECTING,
    BLINKER_STATE_READY,
    BLINKER_STATE_FAIL,
} blinker_state_t;

static blinker_btn_cb_t s_servo_cb = NULL;
static blinker_btn_cb_t s_light_cb = NULL;
static blinker_btn_cb_t s_ac_pwr_cb = NULL;
static blinker_btn_cb_t s_ac_fan_cb = NULL;
static blinker_btn_cb_t s_ac_cool_cb = NULL;
static blinker_btn_cb_t s_ac_dry_cb = NULL;
static blinker_btn_cb_t s_ac_hot_cb = NULL;
static blinker_btn_cb_t s_ac_auto_cb = NULL;
static blinker_slider_cb_t s_temp_slider_cb = NULL;

static blinker_state_t s_state = BLINKER_STATE_WAIT_WIFI;
static uint32_t s_start_time = 0;
static uint32_t s_retry_count = 0;
static bool s_widgets_registered = false;

// 应用状态（由外部设置，心跳回调同步到APP）
static bool s_light_state = false;
static bool s_door_opening = false;

static BlinkerButton *s_btn_servo = NULL;
static BlinkerButton *s_btn_light = NULL;
static BlinkerButton *s_btn_pwr = NULL;
static BlinkerButton *s_btn_fan = NULL;
static BlinkerButton *s_btn_cool = NULL;
static BlinkerButton *s_btn_dry = NULL;
static BlinkerButton *s_btn_hot = NULL;
static BlinkerButton *s_btn_auto = NULL;
static BlinkerSlider *s_slider_temp = NULL;
static BlinkerNumber *s_num_temp = NULL;

#define BLINKER_INIT_DELAY_MS      2000
#define BLINKER_CONNECT_TIMEOUT_MS 30000
#define BLINKER_RETRY_INTERVAL_MS  15000

// 统一状态同步函数 - 参照旧代码 updateAppState()
static void update_app_state(void)
{
    if (!srv_blinker_is_ready()) {
        return;
    }

    if (s_btn_light != NULL) {
        s_btn_light->print(s_light_state ? "on" : "off");
    }

    if (s_door_opening) {
        Blinker.print("门", "开");
        Blinker.vibrate();
    }

    bool ac_power = srv_ir_ac_get_power();
    ac_mode_t ac_mode = srv_ir_ac_get_mode();
    int ac_temp = srv_ir_ac_get_temp();

    if (s_btn_pwr != NULL) {
        if (ac_power) {
            s_btn_pwr->color("#00FF00");
            s_btn_pwr->text("已开机");
            s_btn_pwr->print("on");
        } else {
            s_btn_pwr->color("#FF0000");
            s_btn_pwr->text("已关机");
            s_btn_pwr->print("off");
        }
    }

    if (s_num_temp != NULL) {
        s_num_temp->print(ac_temp);
    }
    if (s_slider_temp != NULL) {
        s_slider_temp->print(ac_temp);
    }

    const char *c = "#CCCCCC", *d = "#CCCCCC", *h = "#CCCCCC", *a = "#CCCCCC";
    if (ac_power) {
        if (ac_mode == AC_MODE_COOL) c = "#00B0FF";
        if (ac_mode == AC_MODE_DRY)  d = "#FFC107";
        if (ac_mode == AC_MODE_HEAT) h = "#FF5722";
        if (ac_mode == AC_MODE_AUTO) a = "#4CAF50";
    }
    if (s_btn_cool) { s_btn_cool->color(c); s_btn_cool->print(); }
    if (s_btn_dry)  { s_btn_dry->color(d);  s_btn_dry->print(); }
    if (s_btn_hot)  { s_btn_hot->color(h);  s_btn_hot->print(); }
    if (s_btn_auto) { s_btn_auto->color(a); s_btn_auto->print(); }

    if (s_btn_fan) {
        s_btn_fan->text(srv_ir_ac_get_fan_text());
        s_btn_fan->print();
    }
}

static void btn_servo_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_KEY_SERVO, state.c_str());
    if ((state == "tap" || state == "on") && s_servo_cb != NULL) {
        s_servo_cb();
    }
}

static void btn_light_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_KEY_LIGHT, state.c_str());
    if (s_light_cb == NULL) {
        return;
    }
    if (state == "on" || state == "tap" || state == "off") {
        s_light_cb();
    }
    update_app_state();
}

static void btn_ac_pwr_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_BTN_PWR, state.c_str());
    if (s_ac_pwr_cb != NULL) {
        s_ac_pwr_cb();
    }
    update_app_state();
}

static void btn_ac_fan_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_BTN_FAN, state.c_str());
    if (state == "tap" || state == "on") {
        if (s_ac_fan_cb != NULL) {
            s_ac_fan_cb();
        }
    }
    update_app_state();
}

static void btn_ac_cool_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_BTN_COOL, state.c_str());
    if (state == "tap" || state == "on") {
        if (s_ac_cool_cb != NULL) {
            s_ac_cool_cb();
        }
    }
    update_app_state();
}

static void btn_ac_dry_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_BTN_DRY, state.c_str());
    if (state == "tap" || state == "on") {
        if (s_ac_dry_cb != NULL) {
            s_ac_dry_cb();
        }
    }
    update_app_state();
}

static void btn_ac_hot_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_BTN_HOT, state.c_str());
    if (state == "tap" || state == "on") {
        if (s_ac_hot_cb != NULL) {
            s_ac_hot_cb();
        }
    }
    update_app_state();
}

static void btn_ac_auto_callback(const String & state)
{
    LOG_I("点灯科技按钮 %s: %s", BLINKER_BTN_AUTO, state.c_str());
    if (state == "tap" || state == "on") {
        if (s_ac_auto_cb != NULL) {
            s_ac_auto_cb();
        }
    }
    update_app_state();
}

static void slider_temp_callback(int32_t value)
{
    LOG_I("点灯科技滑块 %s: %d", BLINKER_SLIDER_TEMP, (int)value);
    if (s_temp_slider_cb != NULL) {
        s_temp_slider_cb(value);
    }
    update_app_state();
}

// 心跳回调 - 定期同步状态到APP
static void heartbeat_callback(void)
{
    update_app_state();
}

static void register_widgets(void)
{
    if (s_widgets_registered) {
        return;
    }

    s_btn_servo = new BlinkerButton(BLINKER_KEY_SERVO);
    s_btn_servo->attach(btn_servo_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_KEY_SERVO);

    s_btn_light = new BlinkerButton(BLINKER_KEY_LIGHT);
    s_btn_light->attach(btn_light_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_KEY_LIGHT);

    s_btn_pwr = new BlinkerButton(BLINKER_BTN_PWR);
    s_btn_pwr->attach(btn_ac_pwr_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_BTN_PWR);

    s_btn_fan = new BlinkerButton(BLINKER_BTN_FAN);
    s_btn_fan->attach(btn_ac_fan_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_BTN_FAN);

    s_btn_cool = new BlinkerButton(BLINKER_BTN_COOL);
    s_btn_cool->attach(btn_ac_cool_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_BTN_COOL);

    s_btn_dry = new BlinkerButton(BLINKER_BTN_DRY);
    s_btn_dry->attach(btn_ac_dry_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_BTN_DRY);

    s_btn_hot = new BlinkerButton(BLINKER_BTN_HOT);
    s_btn_hot->attach(btn_ac_hot_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_BTN_HOT);

    s_btn_auto = new BlinkerButton(BLINKER_BTN_AUTO);
    s_btn_auto->attach(btn_ac_auto_callback);
    LOG_I("点灯科技按钮注册: %s", BLINKER_BTN_AUTO);

    s_slider_temp = new BlinkerSlider(BLINKER_SLIDER_TEMP);
    s_slider_temp->attach(slider_temp_callback);
    LOG_I("点灯科技滑块注册: %s", BLINKER_SLIDER_TEMP);

    s_num_temp = new BlinkerNumber(BLINKER_NUMBER_TEMP);
    LOG_I("点灯科技数值注册: %s", BLINKER_NUMBER_TEMP);

    // 注册心跳回调 - 定期同步状态到APP
    Blinker.attachHeartbeat(heartbeat_callback);
    LOG_I("点灯科技心跳回调已注册");

    s_widgets_registered = true;
}

err_code_t srv_blinker_init(blinker_btn_cb_t servo_cb,
                            blinker_btn_cb_t light_cb,
                            blinker_btn_cb_t ac_pwr_cb,
                            blinker_btn_cb_t ac_fan_cb,
                            blinker_btn_cb_t ac_cool_cb,
                            blinker_btn_cb_t ac_dry_cb,
                            blinker_btn_cb_t ac_hot_cb,
                            blinker_btn_cb_t ac_auto_cb,
                            blinker_slider_cb_t temp_slider_cb)
{
    s_servo_cb = servo_cb;
    s_light_cb = light_cb;
    s_ac_pwr_cb = ac_pwr_cb;
    s_ac_fan_cb = ac_fan_cb;
    s_ac_cool_cb = ac_cool_cb;
    s_ac_dry_cb = ac_dry_cb;
    s_ac_hot_cb = ac_hot_cb;
    s_ac_auto_cb = ac_auto_cb;
    s_temp_slider_cb = temp_slider_cb;

    s_state = BLINKER_STATE_WAIT_WIFI;
    s_widgets_registered = false;
    s_retry_count = 0;
    LOG_I("点灯科技服务初始化，等待WiFi连接");
    return APP_OK;
}

static void blinker_do_connect(void)
{
    s_retry_count++;
    LOG_I("点灯科技连接中... (第%lu次)", (unsigned long)s_retry_count);
    Blinker.begin(BLINKER_AUTH, WIFI_SSID, WIFI_PASSWORD);
    register_widgets();
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
                LOG_I("WiFi已就绪，点灯科技将在2秒后初始化");
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
                    LOG_I("点灯科技连接成功! 服务器在线，MQTT存活");
                }
            }
            if (now - s_start_time >= BLINKER_CONNECT_TIMEOUT_MS) {
                s_state = BLINKER_STATE_FAIL;
                s_start_time = now;
                LOG_W("点灯科技连接超时，%ds后重试",
                      BLINKER_RETRY_INTERVAL_MS / 1000);
            }
            Blinker.run();
            break;

        case BLINKER_STATE_READY:
            if (!srv_wifi_is_connected()) {
                s_state = BLINKER_STATE_FAIL;
                s_start_time = now;
                LOG_W("WiFi断开，点灯科技暂停");
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

void srv_blinker_send_light_state(bool on)
{
    if (s_btn_light != NULL && srv_blinker_is_ready()) {
        s_btn_light->print(on ? "on" : "off");
    }
}

void srv_blinker_send_temp(int temp)
{
    if (s_num_temp != NULL && srv_blinker_is_ready()) {
        s_num_temp->print(temp);
    }
}

void srv_blinker_set_light_state(bool on)
{
    s_light_state = on;
    update_app_state();
}

void srv_blinker_set_door_opening(bool is_opening)
{
    s_door_opening = is_opening;
}

void srv_blinker_notify_action(const char *action)
{
    if (srv_blinker_is_ready()) {
        Blinker.print(action);
        Blinker.vibrate();
    }
}

void srv_blinker_set_ac_power(bool on)
{
    srv_ir_ac_set_power(on);
    update_app_state();
}

bool srv_blinker_get_ac_power(void)
{
    return srv_ir_ac_get_power();
}

void srv_blinker_set_ac_temp(int temp)
{
    srv_ir_ac_set_temp(temp);
    update_app_state();
}

int srv_blinker_get_ac_temp(void)
{
    return srv_ir_ac_get_temp();
}

void srv_blinker_set_ac_mode_cool(void)
{
    srv_ir_ac_set_mode(AC_MODE_COOL);
    update_app_state();
}

void srv_blinker_set_ac_mode_dry(void)
{
    srv_ir_ac_set_mode(AC_MODE_DRY);
    update_app_state();
}

void srv_blinker_set_ac_mode_hot(void)
{
    srv_ir_ac_set_mode(AC_MODE_HEAT);
    update_app_state();
}

void srv_blinker_set_ac_mode_auto(void)
{
    srv_ir_ac_set_mode(AC_MODE_AUTO);
    update_app_state();
}

bool srv_blinker_is_ac_cool(void) { return srv_ir_ac_get_mode() == AC_MODE_COOL; }
bool srv_blinker_is_ac_dry(void)  { return srv_ir_ac_get_mode() == AC_MODE_DRY; }
bool srv_blinker_is_ac_hot(void)  { return srv_ir_ac_get_mode() == AC_MODE_HEAT; }
bool srv_blinker_is_ac_auto(void) { return srv_ir_ac_get_mode() == AC_MODE_AUTO; }

void srv_blinker_update_app_state(void)
{
    update_app_state();
}

void srv_blinker_ac_fan_cycle(void)
{
    srv_ir_ac_cycle_fan();
    update_app_state();
}

const char *srv_blinker_get_ac_fan_text(void)
{
    return srv_ir_ac_get_fan_text();
}
