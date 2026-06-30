#include "srv_ir_ac.h"
#include "../common/log.h"
#include <EEPROM.h>

static bool s_power = false;
static ac_mode_t s_mode = AC_MODE_HEAT;
static uint8_t s_temp = 26;
static int s_fan_level = 4;

static bool s_send_pending = false;
static uint32_t s_send_timer = 0;

static bool s_season_manual_edit = false;

static void ac_save_state(void)
{
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.write(EEPROM_AC_ADDR + 0, EEPROM_AC_MAGIC);
    EEPROM.write(EEPROM_AC_ADDR + 1, s_power ? 1 : 0);
    EEPROM.write(EEPROM_AC_ADDR + 2, (uint8_t)s_mode);
    EEPROM.write(EEPROM_AC_ADDR + 3, s_temp);
    EEPROM.write(EEPROM_AC_ADDR + 4, (uint8_t)s_fan_level);
    EEPROM.commit();
    EEPROM.end();
    LOG_I("空调状态已保存到EEPROM");
}

static bool ac_load_state(void)
{
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    uint8_t magic = EEPROM.read(EEPROM_AC_ADDR + 0);
    if (magic != EEPROM_AC_MAGIC) {
        EEPROM.end();
        return false;
    }
    s_mode = (ac_mode_t)EEPROM.read(EEPROM_AC_ADDR + 2);
    s_temp = EEPROM.read(EEPROM_AC_ADDR + 3);
    s_fan_level = EEPROM.read(EEPROM_AC_ADDR + 4);
    s_power = false;
    if (s_temp < 16 || s_temp > 30) s_temp = AC_DEFAULT_TEMP;
    if (s_fan_level < 1 || s_fan_level > 5) s_fan_level = AC_DEFAULT_FAN_LEVEL;
    if (s_mode > AC_MODE_FAN) s_mode = (ac_mode_t)AC_DEFAULT_MODE;
    EEPROM.end();
    LOG_I("空调状态从EEPROM恢复: %s %d°C %s风 (电源:关)",
          srv_ir_ac_get_mode_text(), s_temp, srv_ir_ac_get_fan_text());
    return true;
}

static void send_now(void)
{
    if (s_season_manual_edit) {
        s_season_manual_edit = false;
        EEPROM.begin(EEPROM_TOTAL_SIZE);
        EEPROM.write(EEPROM_AC_ADDR + 5, 255);
        EEPROM.commit();
        EEPROM.end();
        LOG_I("季节: 用户手动编辑，当月锁定");
    }
    drv_ir_send_state(s_power, s_mode, s_temp, (ac_fan_level_t)s_fan_level);
    ac_save_state();
}

static void send_debounced(void)
{
    s_send_pending = true;
    s_send_timer = millis();
}

err_code_t srv_ir_ac_init(void)
{
    err_code_t ret = drv_ir_init();
    if (ret != APP_OK) {
        return ret;
    }
    if (!ac_load_state()) {
        LOG_I("空调: 无历史记录，使用默认设置");
        s_mode = (ac_mode_t)AC_DEFAULT_MODE;
        s_temp = AC_DEFAULT_TEMP;
        s_fan_level = AC_DEFAULT_FAN_LEVEL;
    }
    s_send_pending = false;
    s_season_manual_edit = false;
    LOG_I("空调服务初始化完成");
    return APP_OK;
}

void srv_ir_ac_run(void)
{
    if (s_send_pending && (millis() - s_send_timer >= AC_SEND_DEBOUNCE_MS)) {
        s_send_pending = false;
        send_now();
    }
}

bool srv_ir_ac_get_power(void) { return s_power; }

void srv_ir_ac_set_power(bool on)
{
    s_power = on;
    send_now();
}

ac_mode_t srv_ir_ac_get_mode(void) { return s_mode; }

void srv_ir_ac_set_mode(ac_mode_t mode)
{
    if (!s_power) {
        return;
    }
    s_mode = mode;
    s_season_manual_edit = true;
    send_debounced();
}

int srv_ir_ac_get_temp(void) { return (int)s_temp; }

void srv_ir_ac_set_temp(int temp)
{
    if (!s_power) {
        return;
    }
    if (temp < 16) temp = 16;
    if (temp > 30) temp = 30;
    s_temp = (uint8_t)temp;
    s_season_manual_edit = true;
    send_debounced();
}

int srv_ir_ac_get_fan_level(void) { return s_fan_level; }

void srv_ir_ac_set_fan_level(int level)
{
    if (!s_power) {
        return;
    }
    if (level < 1) level = 1;
    if (level > 5) level = 5;
    s_fan_level = level;
    send_debounced();
}

void srv_ir_ac_cycle_fan(void)
{
    if (!s_power) {
        return;
    }
    s_fan_level = (s_fan_level >= 5) ? 1 : (s_fan_level + 1);
    send_debounced();
}

const char *srv_ir_ac_get_mode_text(void)
{
    switch (s_mode) {
        case AC_MODE_AUTO: return "自动";
        case AC_MODE_COOL: return "制冷";
        case AC_MODE_DRY:  return "除湿";
        case AC_MODE_HEAT: return "制热";
        case AC_MODE_FAN:  return "送风";
        default: return "未知";
    }
}

const char *srv_ir_ac_get_fan_text(void)
{
    switch (s_fan_level) {
        case 1: return "静音";
        case 2: return "低速";
        case 3: return "中速";
        case 4: return "高速";
        case 5: return "自动";
        default: return "未知";
    }
}

void srv_ir_ac_season_check(int month)
{
    if (month < 1 || month > 12) {
        return;
    }
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    uint8_t season_flag = EEPROM.read(EEPROM_AC_ADDR + 5);
    EEPROM.end();
    if (season_flag == 255) {
        LOG_I("季节: %d月 用户已手动设置，保持当前值 %s %d°C",
              month, srv_ir_ac_get_mode_text(), s_temp);
        return;
    }
    if (season_flag == (uint8_t)month) {
        LOG_I("季节: %d月 已建议过，保持当前值 %s %d°C",
              month, srv_ir_ac_get_mode_text(), s_temp);
        return;
    }
    ac_mode_t new_mode;
    int new_temp;
    if (month >= 6 && month <= 9) {
        new_mode = AC_MODE_COOL;
        new_temp = 26;
    } else if (month == 12 || month == 1 || month == 2) {
        new_mode = AC_MODE_HEAT;
        new_temp = 24;
    } else {
        new_mode = AC_MODE_AUTO;
        new_temp = 26;
    }
    s_mode = new_mode;
    s_temp = (uint8_t)new_temp;
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    EEPROM.write(EEPROM_AC_ADDR + 5, (uint8_t)month);
    EEPROM.commit();
    EEPROM.end();
    const char *sname = (month >= 6 && month <= 9) ? "夏天-制冷26°C" :
                        (month == 12 || month == 1 || month == 2) ? "冬天-制热24°C" :
                        "春秋-自动26°C";
    LOG_I("季节: %d月 -> %s (新月份建议值)", month, sname);
}
