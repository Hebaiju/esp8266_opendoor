#include "task_headers.h"
#include "../service/srv_blinker.h"
#include "../common/log.h"

static void blinker_servo_handler(void)
{
    LOG_I("点灯科技: 舵机按钮按下，发送开门指令");
    door_send_cmd(DOOR_CMD_OPEN);
}

static void blinker_light_handler(void)
{
    LOG_I("点灯科技: 灯光按钮按下，切换灯光");
    task_led_manual_toggle();
}

static void blinker_ac_pwr_handler(void)
{
    bool current = srv_blinker_get_ac_power();
    LOG_I("点灯科技: 空调电源切换: %s -> %s",
          current ? "开" : "关", current ? "关" : "开");
    srv_blinker_set_ac_power(!current);
}

static void blinker_ac_fan_handler(void)
{
    LOG_I("点灯科技: 空调风速按钮按下");
}

static void blinker_ac_cool_handler(void)
{
    LOG_I("点灯科技: 空调制冷模式");
    srv_blinker_set_ac_mode_cool();
}

static void blinker_ac_dry_handler(void)
{
    LOG_I("点灯科技: 空调除湿模式");
    srv_blinker_set_ac_mode_dry();
}

static void blinker_ac_hot_handler(void)
{
    LOG_I("点灯科技: 空调制热模式");
    srv_blinker_set_ac_mode_hot();
}

static void blinker_ac_auto_handler(void)
{
    LOG_I("点灯科技: 空调自动模式");
    srv_blinker_set_ac_mode_auto();
}

static void blinker_temp_slider_handler(int32_t value)
{
    LOG_I("点灯科技: 温度滑块: %d", (int)value);
    srv_blinker_set_ac_temp((int)value);
}

void task_blinker_init(void)
{
    srv_blinker_init(blinker_servo_handler,
                     blinker_light_handler,
                     blinker_ac_pwr_handler,
                     blinker_ac_fan_handler,
                     blinker_ac_cool_handler,
                     blinker_ac_dry_handler,
                     blinker_ac_hot_handler,
                     blinker_ac_auto_handler,
                     blinker_temp_slider_handler);
    LOG_I("点灯科技任务初始化");
}

void task_blinker_run(void)
{
    srv_blinker_run();
}

void task_blinker_report_light(bool on)
{
    srv_blinker_set_light_state(on);
}
