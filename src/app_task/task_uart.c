#include "task_headers.h"
#include "../common/config.h"
#include "../common/log.h"

static uint32_t s_heartbeat_count = 0;

static void ICACHE_FLASH_ATTR task_uart_func(void *pvParameters)
{
    (void)pvParameters;

    LOG_I("UART heartbeat task started");

    for (;;) {
        s_heartbeat_count++;
        LOG_I("Heartbeat #%lu - system running", (unsigned long)s_heartbeat_count);
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

void ICACHE_FLASH_ATTR task_uart_create(void)
{
    portBASE_TYPE ret = xTaskCreate(
        task_uart_func,
        "task_uart",
        TASK_UART_STACK_SIZE,
        NULL,
        TASK_UART_PRIORITY,
        NULL
    );

    if (ret != pdPASS) {
        LOG_E("Failed to create UART task");
    } else {
        LOG_I("UART task created (stack=%d, prio=%d)", TASK_UART_STACK_SIZE, TASK_UART_PRIORITY);
    }
}
