#ifndef __SRV_BLINKER_H__
#define __SRV_BLINKER_H__

#include "../common/err_code.h"
#include "../common/config.h"

/* 按钮回调函数类型 */
typedef void (*blinker_btn_cb_t)(void);
/* 滑块回调函数类型 */
typedef void (*blinker_slider_cb_t)(int32_t value);

/**
 * @brief 初始化点灯科技服务
 * @param servo_cb 舵机按钮回调
 * @param light_cb 灯光按钮回调
 * @param ac_pwr_cb 空调电源按钮回调
 * @param ac_fan_cb 空调风速按钮回调
 * @param ac_cool_cb 制冷模式按钮回调
 * @param ac_dry_cb 除湿模式按钮回调
 * @param ac_hot_cb 制热模式按钮回调
 * @param ac_auto_cb 自动模式按钮回调
 * @param temp_slider_cb 温度滑块回调
 * @return APP_OK 成功，APP_FAIL 失败
 */
err_code_t srv_blinker_init(blinker_btn_cb_t servo_cb,
                            blinker_btn_cb_t light_cb,
                            blinker_btn_cb_t ac_pwr_cb,
                            blinker_btn_cb_t ac_fan_cb,
                            blinker_btn_cb_t ac_cool_cb,
                            blinker_btn_cb_t ac_dry_cb,
                            blinker_btn_cb_t ac_hot_cb,
                            blinker_btn_cb_t ac_auto_cb,
                            blinker_slider_cb_t temp_slider_cb);

/**
 * @brief 运行点灯科技服务主循环
 * 处理连接状态机和Blinker.run()调用
 */
void srv_blinker_run(void);

/**
 * @brief 检查点灯科技连接状态
 * @return true 已连接就绪，false 未就绪
 */
bool srv_blinker_is_ready(void);

/**
 * @brief 设置灯光状态并同步到APP
 * @param on true开，false关
 */
void srv_blinker_set_light_state(bool on);

/**
 * @brief 设置门打开状态
 * @param is_opening true正在开门，false已关闭
 */
void srv_blinker_set_door_opening(bool is_opening);

/**
 * @brief 发送操作通知到APP（含震动）
 * @param action 操作描述字符串
 */
void srv_blinker_notify_action(const char *action);

/* ========== 空调控制API ========== */

/**
 * @brief 设置空调电源状态
 * @param on true开机，false关机
 */
void srv_blinker_set_ac_power(bool on);

/**
 * @brief 获取空调电源状态
 * @return true开机，false关机
 */
bool srv_blinker_get_ac_power(void);

/**
 * @brief 设置空调温度
 * @param temp 目标温度（16-30°C）
 */
void srv_blinker_set_ac_temp(int temp);

/**
 * @brief 获取空调当前温度
 * @return 当前设定温度
 */
int  srv_blinker_get_ac_temp(void);

/**
 * @brief 设置空调制冷模式
 */
void srv_blinker_set_ac_mode_cool(void);

/**
 * @brief 设置空调除湿模式
 */
void srv_blinker_set_ac_mode_dry(void);

/**
 * @brief 设置空调制热模式
 */
void srv_blinker_set_ac_mode_hot(void);

/**
 * @brief 设置空调自动模式
 */
void srv_blinker_set_ac_mode_auto(void);

/**
 * @brief 循环切换空调风速等级
 */
void srv_blinker_ac_fan_cycle(void);

/**
 * @brief 获取风速等级中文描述
 * @return 风速名称字符串指针
 */
const char *srv_blinker_get_ac_fan_text(void);

/**
 * @brief 主动同步所有状态到APP
 * 用于状态变更后立即刷新UI
 */
void srv_blinker_update_app_state(void);

/* ========== Blinker配置持久化 ========== */

bool srv_blinker_load_config(char *auth_buf, int buf_len);
bool srv_blinker_save_config(const char *auth);
bool srv_blinker_has_config(void);
void srv_blinker_clear_config(void);
const char *srv_blinker_get_auth(void);

#endif
