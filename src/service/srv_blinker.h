#ifndef __SRV_BLINKER_H__
#define __SRV_BLINKER_H__

#include "../common/err_code.h"
#include "../common/config.h"

typedef void (*blinker_btn_cb_t)(void);
typedef void (*blinker_slider_cb_t)(int32_t value);

err_code_t srv_blinker_init(blinker_btn_cb_t servo_cb,
                            blinker_btn_cb_t light_cb,
                            blinker_btn_cb_t ac_pwr_cb,
                            blinker_btn_cb_t ac_fan_cb,
                            blinker_btn_cb_t ac_cool_cb,
                            blinker_btn_cb_t ac_dry_cb,
                            blinker_btn_cb_t ac_hot_cb,
                            blinker_btn_cb_t ac_auto_cb,
                            blinker_slider_cb_t temp_slider_cb);
void srv_blinker_run(void);
bool srv_blinker_is_ready(void);
void srv_blinker_send_light_state(bool on);
void srv_blinker_send_temp(int temp);

// 状态同步API - 供应用层调用，更新状态后立即同步到APP
void srv_blinker_set_light_state(bool on);
void srv_blinker_set_door_opening(bool is_opening);
void srv_blinker_notify_action(const char *action);

// 空调状态API
void srv_blinker_set_ac_power(bool on);
bool srv_blinker_get_ac_power(void);
void srv_blinker_set_ac_temp(int temp);
int  srv_blinker_get_ac_temp(void);
void srv_blinker_set_ac_mode_cool(void);
void srv_blinker_set_ac_mode_dry(void);
void srv_blinker_set_ac_mode_hot(void);
void srv_blinker_set_ac_mode_auto(void);
bool srv_blinker_is_ac_cool(void);
bool srv_blinker_is_ac_dry(void);
bool srv_blinker_is_ac_hot(void);
bool srv_blinker_is_ac_auto(void);
void srv_blinker_update_app_state(void);
void srv_blinker_ac_fan_cycle(void);
const char *srv_blinker_get_ac_fan_text(void);

#endif
