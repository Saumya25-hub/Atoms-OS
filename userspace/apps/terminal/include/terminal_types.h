#ifndef BOS_TERMINAL_TYPES_H
#define BOS_TERMINAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TERMINAL_MAX_LINE_LEN      4096
#define TERMINAL_MAX_HISTORY       256
#define TERMINAL_MAX_ALIASES       128
#define TERMINAL_MAX_PLUGINS       64
#define TERMINAL_MAX_ENV_VARS      512
#define TERMINAL_MAX_ARGS          64
#define TERMINAL_PROMPT            "ATOMS> "

typedef enum {
    TERMINAL_OK        = 0,
    TERMINAL_ERR       = -1,
    TERMINAL_ERR_ARG   = -2,
    TERMINAL_ERR_MEM   = -3,
    TERMINAL_ERR_PERM  = -4,
    TERMINAL_NOT_FOUND = -5,
} TERMINAL_RESULT;

typedef int (*TERMINAL_COMMAND_HANDLER)(int argc, const char* argv[]);

typedef struct _TERMINAL_COMMAND {
    char  name[64];
    char  description[128];
    TERMINAL_COMMAND_HANDLER handler;
} TERMINAL_COMMAND;

typedef struct _TERMINAL_ALIAS {
    char alias[64];
    char target[256];
} TERMINAL_ALIAS;

typedef struct _TERMINAL_ENV_VAR {
    char key[128];
    char value[512];
} TERMINAL_ENV_VAR;

typedef struct _TERMINAL_SESSION {
    bool  active;
    char  cwd[512];
    uint32_t command_count;
    uint32_t error_count;
} TERMINAL_SESSION;

#endif // BOS_TERMINAL_TYPES_H
