#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include "c_types.h"
#include "gpio.h"
#include "../common/err_code.h"

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
} gpio_mode_t;

err_code_t bsp_gpio_init(uint8_t pin, gpio_mode_t mode);
void bsp_gpio_write(uint8_t pin, uint8_t level);
uint8_t bsp_gpio_read(uint8_t pin);
void bsp_gpio_toggle(uint8_t pin);

#endif
