#include "task_headers.h"
#include "../driver/drv_led.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../service/srv_ntp.h"

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_BREATH,
    LED_MODE_AUTO,
} led_mode_t;

static led_mode_t s_mode = LED_MODE_BLINK;
static uint32_t s_last_tick = 0;
static uint32_t s_blink_interval = LED_BLINK_WIFI_FAIL_MS;

static int16_t s_breath_level = LED_PWM_MIN;
static int8_t s_breath_dir = 1;

static uint8_t s_last_auto_hour = 255;
static bool s_manual_override = false;
static uint8_t s_override_hour = 255;

static bool s_light_is_on = false;

extern void task_blinker_report_light(bool on);

static bool auto_is_on(void);

static void led_report_state_if_needed(void)
{
    if (s_mode == LED_MODE_BLINK) {
        return;
    }

    bool now_on = false;
    switch (s_mode) {
        case LED_MODE_ON:
        case LED_MODE_BREATH:
            now_on = true;
            break;
        case LED_MODE_OFF:
            now_on = false;
            break;
        case LED_MODE_AUTO:
            if (s_manual_override) {
                now_on = !auto_is_on();
            } else {
                now_on = auto_is_on();
            }
            break;
        default:
            break;
    }

    if (now_on != s_light_is_on) {
        s_light_is_on = now_on;
        task_blinker_report_light(now_on);
    }
}

static bool is_auto_on_hour(uint8_t hour)
{
    if (hour >= LED_AUTO_ON_START_HOUR && hour < LED_AUTO_ON_END_HOUR) {
        return true;
    }
    if (hour >= LED_AUTO_ON_START_HOUR2 && hour < LED_AUTO_ON_END_HOUR2) {
        return true;
    }
    return false;
}

static void auto_check_and_apply(void)
{
    if (!srv_ntp_is_synced()) {
        return;
    }
    uint8_t hour = srv_ntp_get_hour();
    if (hour == s_last_auto_hour) {
        return;
    }
    s_last_auto_hour = hour;

    if (s_manual_override) {
        if (hour != s_override_hour) {
            s_manual_override = false;
            LOG_I("LED自动模式: 手动覆盖已过期，恢复自动模式");
        } else {
            return;
        }
    }

    if (is_auto_on_hour(hour)) {
        s_breath_level = LED_PWM_MIN;
        s_breath_dir = 1;
        s_last_tick = millis();
        LOG_I("LED自动模式: 开启 (当前%d点), 呼吸灯模式", hour);
    } else {
        drv_led_off();
        LOG_I("LED自动模式: 关闭 (当前%d点)", hour);
    }
}

static bool auto_is_on(void)
{
    if (!srv_ntp_is_synced()) {
        return false;
    }
    uint8_t hour = srv_ntp_get_hour();
    return is_auto_on_hour(hour);
}

void task_led_init(void)
{
    s_last_tick = millis();
    LOG_I("LED任务初始化, 模式=闪烁");
}

void task_led_set_mode(uint8_t mode)
{
    if (mode > LED_MODE_AUTO) {
        return;
    }
    s_mode = (led_mode_t)mode;
    switch (s_mode) {
        case LED_MODE_OFF:
            drv_led_off();
            LOG_I("LED模式: 关闭");
            break;
        case LED_MODE_ON:
            drv_led_on();
            LOG_I("LED模式: 常亮");
            break;
        case LED_MODE_BLINK:
            s_last_tick = millis();
            LOG_I("LED模式: 闪烁 (%lums)", (unsigned long)s_blink_interval);
            break;
        case LED_MODE_BREATH:
            s_breath_level = LED_PWM_MIN;
            s_breath_dir = 1;
            s_last_tick = millis();
            LOG_I("LED模式: 呼吸灯");
            break;
        case LED_MODE_AUTO:
            s_last_auto_hour = 255;
            s_manual_override = false;
            LOG_I("LED模式: 自动 (时间控制)");
            break;
    }
}

void task_led_set_interval(uint32_t ms)
{
    s_blink_interval = ms;
}

uint8_t task_led_get_mode(void)
{
    return (uint8_t)s_mode;
}

void task_led_manual_toggle(void)
{
    // 非AUTO模式下，直接切换灯光状态
    if (s_mode == LED_MODE_OFF) {
        s_breath_level = LED_PWM_MIN;
        s_breath_dir = 1;
        s_last_tick = millis();
        s_mode = LED_MODE_BREATH;
        LOG_I("LED手动切换: 关闭 -> 开启 (呼吸灯)");
        task_blinker_report_light(true);
    } else if (s_mode == LED_MODE_ON || s_mode == LED_MODE_BREATH) {
        s_mode = LED_MODE_OFF;
        drv_led_off();
        LOG_I("LED手动切换: 开启 -> 关闭");
        task_blinker_report_light(false);
    } else if (s_mode == LED_MODE_BLINK) {
        // 从闪烁模式切换到呼吸灯（开启）
        s_breath_level = LED_PWM_MIN;
        s_breath_dir = 1;
        s_last_tick = millis();
        s_mode = LED_MODE_BREATH;
        LOG_I("LED手动切换: 闪烁 -> 开启 (呼吸灯)");
        task_blinker_report_light(true);
    } else if (s_mode == LED_MODE_AUTO) {
        // AUTO模式下的原有逻辑
        if (!srv_ntp_is_synced()) {
            LOG_W("LED自动切换失败: NTP未同步");
            return;
        }
        uint8_t hour = srv_ntp_get_hour();
        s_override_hour = hour;
        s_manual_override = true;

        if (auto_is_on()) {
            drv_led_off();
            LOG_I("LED自动模式: 手动覆盖 -> 关闭 (当前%d点)", hour);
            task_blinker_report_light(false);
        } else {
            s_breath_level = LED_PWM_MIN;
            s_breath_dir = 1;
            s_last_tick = millis();
            LOG_I("LED自动模式: 手动覆盖 -> 开启 (当前%d点)", hour);
            task_blinker_report_light(true);
        }
    }
}

static void task_led_run_blink(uint32_t now)
{
    if (now - s_last_tick >= s_blink_interval / 2) {
        s_last_tick = now;
        drv_led_toggle();
    }
}

static void task_led_run_breath(uint32_t now)
{
    if (now - s_last_tick >= LED_BREATH_INTERVAL_MS) {
        s_last_tick = now;
        s_breath_level += s_breath_dir * LED_BREATH_STEP;
        if (s_breath_level >= LED_PWM_MAX) {
            s_breath_level = LED_PWM_MAX;
            s_breath_dir = -1;
        } else if (s_breath_level <= LED_PWM_MIN) {
            s_breath_level = LED_PWM_MIN;
            s_breath_dir = 1;
        }
        drv_led_set_brightness((uint16_t)s_breath_level);
    }
}

static void task_led_run_auto(uint32_t now)
{
    auto_check_and_apply();

    if (s_manual_override) {
        if (auto_is_on()) {
            drv_led_off();
        } else {
            task_led_run_breath(now);
        }
    } else {
        if (auto_is_on()) {
            task_led_run_breath(now);
        }
    }
}

void task_led_run(void)
{
    uint32_t now = millis();

    // WiFi连接成功后且NTP同步后，自动切换到AUTO模式
    if (s_mode == LED_MODE_BLINK && srv_ntp_is_synced()) {
        s_mode = LED_MODE_AUTO;
        s_last_auto_hour = 255;
        s_manual_override = false;
        s_last_tick = now;
        LOG_I("LED自动模式: WiFi+NTP就绪，切换到自动模式");
    }

    switch (s_mode) {
        case LED_MODE_BLINK:
            task_led_run_blink(now);
            break;
        case LED_MODE_BREATH:
            task_led_run_breath(now);
            break;
        case LED_MODE_AUTO:
            task_led_run_auto(now);
            break;
        default:
            break;
    }
    led_report_state_if_needed();
}
