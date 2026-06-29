#ifndef __LOG_H__
#define __LOG_H__

#include "esp_common.h"

#define LOG_I(fmt, ...) do { \
    printf("[INFO] " fmt "\r\n", ##__VA_ARGS__); \
} while(0)

#define LOG_W(fmt, ...) do { \
    printf("[WARN] " fmt "\r\n", ##__VA_ARGS__); \
} while(0)

#define LOG_E(fmt, ...) do { \
    printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__); \
} while(0)

#endif
