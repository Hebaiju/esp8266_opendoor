#ifndef __DRV_SERVO_H__
#define __DRV_SERVO_H__

#include "../common/err_code.h"
#include "../common/config.h"

err_code_t drv_servo_init(void);
void drv_servo_set_angle(uint8_t angle);
uint8_t drv_servo_get_angle(void);

#endif
