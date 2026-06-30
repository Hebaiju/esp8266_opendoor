#include "task_headers.h"
#include "../driver/drv_servo.h"
#include "../driver/drv_led.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../service/srv_blinker.h"

typedef enum {
    DOOR_STATE_IDLE = 0,
    DOOR_STATE_OPENING,
    DOOR_STATE_HOLD,
    DOOR_STATE_CLOSING,
} door_state_t;

static door_state_t s_state = DOOR_STATE_IDLE;
static uint32_t s_state_start = 0;
static door_msg_t s_queue[DOOR_QUEUE_SIZE];
static uint8_t  s_q_head = 0;
static uint8_t  s_q_tail = 0;
static uint8_t  s_q_count = 0;

void door_send_cmd(door_cmd_t cmd)
{
    if (s_q_count >= DOOR_QUEUE_SIZE) {
        LOG_W("门队列已满，指令丢弃");
        return;
    }
    s_queue[s_q_tail].cmd = cmd;
    s_q_tail = (s_q_tail + 1) % DOOR_QUEUE_SIZE;
    s_q_count++;
}

static bool door_dequeue(door_msg_t *msg)
{
    if (s_q_count == 0) {
        return false;
    }
    *msg = s_queue[s_q_head];
    s_q_head = (s_q_head + 1) % DOOR_QUEUE_SIZE;
    s_q_count--;
    return true;
}

void task_door_init(void)
{
    s_state = DOOR_STATE_IDLE;
    s_state_start = millis();
    LOG_I("门控任务初始化");
}

void task_door_run(void)
{
    uint32_t now = millis();
    door_msg_t msg;

    switch (s_state) {
        case DOOR_STATE_IDLE:
            if (door_dequeue(&msg)) {
                if (msg.cmd == DOOR_CMD_OPEN) {
                    LOG_I("门: 开门中 (%d -> %d 度)",
                          SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
                    drv_servo_set_angle(SERVO_MAX_ANGLE);
                    srv_blinker_set_door_opening(true);
                    srv_blinker_notify_action("门");
                    s_state = DOOR_STATE_OPENING;
                    s_state_start = now;
                }
            }
            break;

        case DOOR_STATE_OPENING:
            if (now - s_state_start >= 500) {
                LOG_I("门: 保持 %dms", SERVO_HOLD_MS);
                s_state = DOOR_STATE_HOLD;
                s_state_start = now;
            }
            break;

        case DOOR_STATE_HOLD:
            if (now - s_state_start >= SERVO_HOLD_MS) {
                LOG_I("门: 关门中 (%d -> %d 度)",
                      SERVO_MAX_ANGLE, SERVO_MIN_ANGLE);
                drv_servo_set_angle(SERVO_MIN_ANGLE);
                s_state = DOOR_STATE_CLOSING;
                s_state_start = now;
            }
            break;

        case DOOR_STATE_CLOSING:
            if (now - s_state_start >= 500) {
                LOG_I("门: 空闲");
                srv_blinker_set_door_opening(false);
                s_state = DOOR_STATE_IDLE;
                s_state_start = now;
            }
            break;

        default:
            break;
    }
}

bool task_door_is_opening(void)
{
    return s_state == DOOR_STATE_OPENING || s_state == DOOR_STATE_HOLD;
}
