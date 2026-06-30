#ifndef __SRV_NTP_H__
#define __SRV_NTP_H__

#include "../common/err_code.h"
#include <time.h>

err_code_t srv_ntp_init(void);
void srv_ntp_run(void);
bool srv_ntp_is_synced(void);
bool srv_ntp_get_time(struct tm *out_tm);
uint8_t srv_ntp_get_hour(void);

#endif
