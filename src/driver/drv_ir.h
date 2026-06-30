#ifndef __DRV_IR_H__
#define __DRV_IR_H__

#include "../common/err_code.h"
#include "../common/config.h"
#include <stdint.h>

/**
 * @brief 空调运行模式枚举
 */
typedef enum {
    AC_MODE_AUTO = 0,  /* 自动模式 */
    AC_MODE_COOL = 1,  /* 制冷模式 */
    AC_MODE_DRY  = 2,  /* 除湿模式 */
    AC_MODE_HEAT = 3,  /* 制热模式 */
    AC_MODE_FAN  = 4,  /* 送风模式 */
} ac_mode_t;

/**
 * @brief 空调风速等级枚举
 */
typedef enum {
    AC_FAN_1 = 1,  /* 静音/1档 */
    AC_FAN_2 = 2,  /* 低速/2档 */
    AC_FAN_3 = 3,  /* 中速/3档 */
    AC_FAN_4 = 4,  /* 高速/4档 */
    AC_FAN_5 = 5,  /* 自动/5档 */
} ac_fan_level_t;

/**
 * @brief 初始化红外驱动
 * @return APP_OK 成功，APP_FAIL 失败
 */
err_code_t drv_ir_init(void);

/**
 * @brief 发送空调状态红外码
 * @param power 电源状态（true开/false关）
 * @param mode 运行模式
 * @param temp 设定温度（16-30°C）
 * @param fan 风速等级
 */
void drv_ir_send_state(bool power, ac_mode_t mode, uint8_t temp, ac_fan_level_t fan);

#endif
