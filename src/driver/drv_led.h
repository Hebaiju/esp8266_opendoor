#ifndef __DRV_LED_H__
#define __DRV_LED_H__

#include "../common/err_code.h"
#include "../common/config.h"

err_code_t drv_led_init(void);
void drv_led_on(void);
void drv_led_off(void);
void drv_led_toggle(void);

#endif
