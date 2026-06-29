#include "bsp_gpio.h"

err_code_t bsp_gpio_init(uint8_t pin, gpio_mode_t mode)
{
    switch (mode) {
        case GPIO_MODE_INPUT:
            pinMode(pin, INPUT);
            break;
        case GPIO_MODE_OUTPUT:
            pinMode(pin, OUTPUT);
            break;
        case GPIO_MODE_INPUT_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;
        case GPIO_MODE_INPUT_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN_16);
            break;
        default:
            return APP_ERR_PARAM;
    }
    return APP_OK;
}

void bsp_gpio_write(uint8_t pin, uint8_t level)
{
    digitalWrite(pin, level ? HIGH : LOW);
}

uint8_t bsp_gpio_read(uint8_t pin)
{
    return digitalRead(pin);
}

void bsp_gpio_toggle(uint8_t pin)
{
    digitalWrite(pin, !digitalRead(pin));
}
