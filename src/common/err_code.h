#ifndef __ERR_CODE_H__
#define __ERR_CODE_H__

typedef enum {
    APP_OK = 0,
    APP_FAIL = -1,
    APP_ERR_PARAM = -2,
    APP_ERR_TIMEOUT = -3,
    APP_ERR_NOMEM = -4,
    APP_ERR_BUSY = -5,
} err_code_t;

#endif
