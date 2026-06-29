/*
 * srv_wifi.h - WiFi 服务层头文件（预留）
 * 适配硬件：ESP8266-01S
 * 创建时间：2026-06-29
 */

#ifndef __SRV_WIFI_H__
#define __SRV_WIFI_H__

#include "../common/err_code.h"

#ifdef __cplusplus
extern "C" {
#endif

err_code_t srv_wifi_init(void);

#ifdef __cplusplus
}
#endif

#endif
