#include "task_headers.h"
#include "../service/srv_blinker.h"
#include "../common/log.h"

static void blinker_open_handler(void)
{
    LOG_I("Blinker open button pressed, send door cmd");
    door_send_cmd(DOOR_CMD_OPEN);
}

void task_blinker_init(void)
{
    srv_blinker_init(blinker_open_handler);
    LOG_I("Blinker task init");
}

void task_blinker_run(void)
{
    srv_blinker_run();
}
