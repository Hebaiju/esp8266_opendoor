#ifndef __TASK_HEADERS_H__
#define __TASK_HEADERS_H__

#include <Arduino.h>

typedef enum {
    DOOR_CMD_OPEN = 0,
} door_cmd_t;

typedef struct {
    door_cmd_t cmd;
    uint8_t    reserved;
} door_msg_t;

#define DOOR_QUEUE_SIZE  4

void task_door_init(void);
void task_door_run(void);

void task_led_init(void);
void task_led_run(void);
void task_led_set_interval(uint32_t ms);
void task_led_set_mode(uint8_t mode);
uint8_t task_led_get_mode(void);
void task_led_manual_toggle(void);

void task_uart_init(void);
void task_uart_run(void);

void task_blinker_init(void);
void task_blinker_run(void);
void task_blinker_report_light(bool on);

void door_send_cmd(door_cmd_t cmd);

#endif
