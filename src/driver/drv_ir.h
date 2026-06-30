#ifndef __DRV_IR_H__
#define __DRV_IR_H__

#include "../common/err_code.h"
#include "../common/config.h"
#include <stdint.h>

typedef enum {
    AC_MODE_AUTO = 0,
    AC_MODE_COOL = 1,
    AC_MODE_DRY  = 2,
    AC_MODE_HEAT = 3,
    AC_MODE_FAN  = 4,
} ac_mode_t;

typedef enum {
    AC_FAN_1 = 1,
    AC_FAN_2 = 2,
    AC_FAN_3 = 3,
    AC_FAN_4 = 4,
    AC_FAN_5 = 5,
} ac_fan_level_t;

err_code_t drv_ir_init(void);
void drv_ir_send_state(bool power, ac_mode_t mode, uint8_t temp, ac_fan_level_t fan);

#endif
