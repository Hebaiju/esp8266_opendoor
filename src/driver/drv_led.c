#include "drv_led.h"
#include "../bsp/bsp_gpio.h"

err_code_t drv_led_init(void)
{
    err_code_t ret = bsp_gpio_init(ONBOARD_LED_PIN, GPIO_MODE_OUTPUT);
    if (ret != ERR_OK) {
        return ret;
    }
    drv_led_off();
    return ERR_OK;
}

void drv_led_on(void)
{
    bsp_gpio_write(ONBOARD_LED_PIN, LED_ACTIVE_LEVEL);
}

void drv_led_off(void)
{
    bsp_gpio_write(ONBOARD_LED_PIN, !LED_ACTIVE_LEVEL);
}

void drv_led_toggle(void)
{
    bsp_gpio_toggle(ONBOARD_LED_PIN);
}
