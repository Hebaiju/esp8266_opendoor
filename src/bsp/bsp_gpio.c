#include "bsp_gpio.h"

err_code_t bsp_gpio_init(uint8_t pin, gpio_mode_t mode)
{
    if (pin >= 16) {
        return ERR_PARAM;
    }

    switch (mode) {
        case GPIO_MODE_INPUT:
            GPIO_DIS_OUTPUT(pin);
            break;
        case GPIO_MODE_OUTPUT:
            GPIO_AS_OUTPUT(pin);
            break;
        case GPIO_MODE_INPUT_PULLUP:
            GPIO_DIS_OUTPUT(pin);
            break;
        default:
            return ERR_PARAM;
    }
    return ERR_OK;
}

void bsp_gpio_write(uint8_t pin, uint8_t level)
{
    GPIO_OUTPUT_SET(pin, level ? 1 : 0);
}

uint8_t bsp_gpio_read(uint8_t pin)
{
    return GPIO_INPUT_GET(pin);
}

void bsp_gpio_toggle(uint8_t pin)
{
    if (GPIO_INPUT_GET(pin)) {
        GPIO_OUTPUT_SET(pin, 0);
    } else {
        GPIO_OUTPUT_SET(pin, 1);
    }
}
