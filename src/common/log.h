#ifndef __LOG_H__
#define __LOG_H__

#include <Arduino.h>

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_DEBUG = 1,
} log_level_t;

extern log_level_t g_log_level;

#define LOG_I(fmt, ...) do { \
    Serial.printf("[INFO] " fmt "\r\n", ##__VA_ARGS__); \
} while(0)

#define LOG_W(fmt, ...) do { \
    Serial.printf("[WARN] " fmt "\r\n", ##__VA_ARGS__); \
} while(0)

#define LOG_E(fmt, ...) do { \
    Serial.printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__); \
} while(0)

#define LOG_D(fmt, ...) do { \
    if (g_log_level >= LOG_LEVEL_DEBUG) { \
        Serial.printf("[DEBUG] " fmt "\r\n", ##__VA_ARGS__); \
    } \
} while(0)

#endif
