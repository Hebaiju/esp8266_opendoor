#ifndef __ERR_CODE_H__
#define __ERR_CODE_H__

typedef enum {
    ERR_OK = 0,
    ERR_FAIL = -1,
    ERR_PARAM = -2,
    ERR_TIMEOUT = -3,
    ERR_NOMEM = -4,
    ERR_BUSY = -5,
} err_code_t;

#endif
