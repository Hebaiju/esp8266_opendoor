#include "task_headers.h"
#include "../driver/drv_led.h"
#include "../common/config.h"
#include "../common/log.h"

static void ICACHE_FLASH_ATTR task_led_func(void *pvParameters)
{
    (void)pvParameters;

    LOG_I("LED task started, blink period: %dms", LED_BLINK_PERIOD_MS);

    for (;;) {
        drv_led_toggle();
        vTaskDelay(LED_BLINK_PERIOD_MS / 2 / portTICK_RATE_MS);
    }
}

void ICACHE_FLASH_ATTR task_led_create(void)
{
    portBASE_TYPE ret = xTaskCreate(
        task_led_func,
        "task_led",
        TASK_LED_STACK_SIZE,
        NULL,
        TASK_LED_PRIORITY,
        NULL
    );

    if (ret != pdPASS) {
        LOG_E("Failed to create LED task");
    } else {
        LOG_I("LED task created (stack=%d, prio=%d)", TASK_LED_STACK_SIZE, TASK_LED_PRIORITY);
    }
}
