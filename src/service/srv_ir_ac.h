#ifndef __SRV_IR_AC_H__
#define __SRV_IR_AC_H__

#include "../common/err_code.h"
#include "../common/config.h"
#include "../driver/drv_ir.h"
#include <stdint.h>

err_code_t srv_ir_ac_init(void);
void srv_ir_ac_run(void);

bool srv_ir_ac_get_power(void);
void srv_ir_ac_set_power(bool on);

ac_mode_t srv_ir_ac_get_mode(void);
void srv_ir_ac_set_mode(ac_mode_t mode);

int srv_ir_ac_get_temp(void);
void srv_ir_ac_set_temp(int temp);

int srv_ir_ac_get_fan_level(void);
void srv_ir_ac_set_fan_level(int level);
void srv_ir_ac_cycle_fan(void);

const char *srv_ir_ac_get_mode_text(void);
const char *srv_ir_ac_get_fan_text(void);

void srv_ir_ac_season_check(int month);

#endif
