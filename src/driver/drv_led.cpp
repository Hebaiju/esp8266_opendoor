#include "drv_led.h"
#include "../bsp/bsp_gpio.h"
#include "../common/log.h"

static uint16_t s_current_level = 0;

err_code_t drv_led_init(void)
{
    err_code_t ret = bsp_gpio_init(ONBOARD_LED_PIN, GPIO_MODE_OUTPUT);
    if (ret != APP_OK) {
        return ret;
    }
    analogWriteRange(LED_PWM_MAX);
    analogWriteFreq(1000);
    drv_led_off();
    return APP_OK;
}

void drv_led_set_brightness(uint16_t level)
{
    if (level > LED_PWM_MAX) {
        level = LED_PWM_MAX;
    }
    s_current_level = level;

#if LED_ACTIVE_LEVEL == 0
    uint16_t pwm_val = LED_PWM_MAX - level;
#else
    uint16_t pwm_val = level;
#endif
    analogWrite(ONBOARD_LED_PIN, pwm_val);
}

uint16_t drv_led_get_brightness(void)
{
    return s_current_level;
}

void drv_led_on(void)
{
    drv_led_set_brightness(LED_PWM_MAX);
}

void drv_led_off(void)
{
    drv_led_set_brightness(LED_PWM_MIN);
}

void drv_led_toggle(void)
{
    if (s_current_level > 0) {
        drv_led_off();
    } else {
        drv_led_on();
    }
}
