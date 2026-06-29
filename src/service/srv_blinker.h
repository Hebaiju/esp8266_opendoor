#ifndef __SRV_BLINKER_H__
#define __SRV_BLINKER_H__

#include "../common/err_code.h"
#include "../common/config.h"

typedef void (*blinker_btn_cb_t)(void);

err_code_t srv_blinker_init(blinker_btn_cb_t open_cb);
void srv_blinker_run(void);
bool srv_blinker_is_ready(void);

#endif
