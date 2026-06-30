#ifndef __SRV_IR_AC_H__
#define __SRV_IR_AC_H__

#include "../common/err_code.h"
#include "../common/config.h"
#include "../driver/drv_ir.h"
#include <stdint.h>

/**
 * @brief 初始化空调服务（含红外驱动）
 * @return APP_OK 成功，APP_FAIL 失败
 */
err_code_t srv_ir_ac_init(void);

/**
 * @brief 运行空调服务主循环
 * 处理防抖发送定时器
 */
void srv_ir_ac_run(void);

/**
 * @brief 获取空调电源状态
 * @return true 开机，false 关机
 */
bool srv_ir_ac_get_power(void);

/**
 * @brief 设置空调电源状态
 * @param on true开机，false关机
 */
void srv_ir_ac_set_power(bool on);

/**
 * @brief 获取空调运行模式
 * @return 当前模式枚举值
 */
ac_mode_t srv_ir_ac_get_mode(void);

/**
 * @brief 设置空调运行模式
 * @param mode 目标模式（仅开机状态下生效）
 */
void srv_ir_ac_set_mode(ac_mode_t mode);

/**
 * @brief 获取空调设定温度
 * @return 设定温度（16-30°C）
 */
int srv_ir_ac_get_temp(void);

/**
 * @brief 设置空调设定温度
 * @param temp 目标温度（16-30°C，仅开机状态下生效）
 */
void srv_ir_ac_set_temp(int temp);

/**
 * @brief 获取空调风速等级
 * @return 当前风速等级（1-5）
 */
int srv_ir_ac_get_fan_level(void);

/**
 * @brief 设置空调风速等级
 * @param level 目标风速（1-5，仅开机状态下生效）
 */
void srv_ir_ac_set_fan_level(int level);

/**
 * @brief 循环切换风速等级（1→2→3→4→5→1）
 */
void srv_ir_ac_cycle_fan(void);

/**
 * @brief 获取当前模式的中文描述
 * @return 模式名称字符串指针
 */
const char *srv_ir_ac_get_mode_text(void);

/**
 * @brief 获取当前风速的中文描述
 * @return 风速名称字符串指针
 */
const char *srv_ir_ac_get_fan_text(void);

/**
 * @brief 根据月份进行季节自动调整
 * 夏季(6-9月): 制冷26°C
 * 冬季(12-2月): 制热24°C
 * 春秋: 自动26°C
 * 用户手动编辑后当月锁定，不再自动调整
 * @param month 当前月份（1-12）
 */
void srv_ir_ac_season_check(int month);

#endif
