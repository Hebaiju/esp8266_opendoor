#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "c_types.h"

#define ONBOARD_LED_PIN     2
#define LED_ACTIVE_LEVEL    0

#define UART_BAUDRATE       115200

#define TASK_LED_STACK_SIZE     512
#define TASK_LED_PRIORITY       2
#define TASK_UART_STACK_SIZE    512
#define TASK_UART_PRIORITY      1

#define LED_BLINK_PERIOD_MS     1000

#endif
