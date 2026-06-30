#include "drv_ir.h"
#include "../common/log.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>

/* IR发送器和空调协议对象 */
static IRsend *s_irsend = NULL;
static IRCoolixAC *s_ac = NULL;

/**
 * @brief 内部函数：将应用层模式转换为Coolix协议模式值
 */
static uint8_t mode_to_coolix(ac_mode_t mode)
{
    switch (mode) {
        case AC_MODE_AUTO: return kCoolixAuto;
        case AC_MODE_COOL: return kCoolixCool;
        case AC_MODE_DRY:  return kCoolixDry;
        case AC_MODE_HEAT: return kCoolixHeat;
        case AC_MODE_FAN:  return kCoolixFan;
        default: return kCoolixAuto;
    }
}

/**
 * @brief 内部函数：将应用层风速等级转换为Coolix协议风速值
 */
static uint8_t fan_to_coolix(ac_fan_level_t fan)
{
    switch (fan) {
        case AC_FAN_1: return kCoolixFanFixed;
        case AC_FAN_2: return kCoolixFanMin;
        case AC_FAN_3: return kCoolixFanMed;
        case AC_FAN_4: return kCoolixFanMax;
        case AC_FAN_5: return kCoolixFanAuto0;
        default: return kCoolixFanAuto0;
    }
}

/**
 * @brief 初始化红外驱动
 */
err_code_t drv_ir_init(void)
{
    s_irsend = new IRsend(IR_TX_PIN);
    s_ac = new IRCoolixAC(IR_TX_PIN);
    if (s_irsend == NULL || s_ac == NULL) {
        LOG_E("红外驱动初始化失败，内存不足");
        return APP_FAIL;
    }
    s_irsend->begin();
    s_ac->begin();
    LOG_I("红外驱动初始化完成，引脚=%d", IR_TX_PIN);
    return APP_OK;
}

/**
 * @brief 发送空调状态红外码
 * 根据电源状态发送开/关机或完整状态码
 */
void drv_ir_send_state(bool power, ac_mode_t mode, uint8_t temp, ac_fan_level_t fan)
{
    if (s_ac == NULL) {
        return;
    }
    if (!power) {
        s_ac->off();
        s_ac->send();
        LOG_I("红外发送: 空调关机");
        return;
    }
    s_ac->on();
    s_ac->setMode(mode_to_coolix(mode));
    s_ac->setTemp(temp);
    s_ac->setFan(fan_to_coolix(fan));
    s_ac->send();
    LOG_I("红外发送: 开机 模式=%d 温度=%d 风速=%d", (int)mode, temp, (int)fan);
}
