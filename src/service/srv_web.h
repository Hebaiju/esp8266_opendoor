#ifndef SRV_WEB_H
#define SRV_WEB_H

#include "../common/err_code.h"
#include <stdint.h>

err_code_t srv_web_init(void);
void srv_web_run(void);
uint32_t srv_web_get_request_count(void);

#endif
