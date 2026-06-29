#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_common.h"
#include "esp_misc.h"

#include "common/config.h"
#include "common/log.h"
#include "driver/drv_led.h"
#include "app_task/task_headers.h"

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    return 0x7D000 / 4096;
}

void ICACHE_FLASH_ATTR user_init(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("  ESP8266-01S OpenDoor - RTOS Test\r\n");
    printf("  Version: 0.0.1\r\n");
    printf("========================================\r\n");
    printf("\r\n");

    LOG_I("Hardware init...");

    if (drv_led_init() != ERR_OK) {
        LOG_E("LED init failed");
    } else {
        LOG_I("LED init ok");
    }

    LOG_I("Creating RTOS tasks...");

    task_led_create();
    task_uart_create();

    LOG_I("System init complete, scheduler running");
}
