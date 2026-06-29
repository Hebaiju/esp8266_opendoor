#ifndef __TASK_HEADERS_H__
#define __TASK_HEADERS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_led_create(void);
void task_uart_create(void);

#endif
