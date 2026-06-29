#include "task_headers.h"
#include "../driver/drv_led.h"
#include "../common/config.h"
#include "../common/log.h"

static uint32_t s_last_tick = 0;
static uint32_t s_blink_interval = LED_BLINK_WIFI_FAIL_MS;

void task_led_init(void)
{
    s_last_tick = millis();
    LOG_I("LED task init");
}

void task_led_set_interval(uint32_t ms)
{
    s_blink_interval = ms;
}

void task_led_run(void)
{
    uint32_t now = millis();
    if (now - s_last_tick >= s_blink_interval / 2) {
        s_last_tick = now;
        drv_led_toggle();
    }
}
