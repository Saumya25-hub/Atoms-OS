#ifndef BOS_BAR_TYPES_H
#define BOS_BAR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BAR_MAX_PROCESSES 128
#define BAR_MAX_WINDOWS   256
#define BAR_MAX_TIMERS    128
#define BAR_MAX_MESSAGES  512
#define BAR_MAX_RESOURCES 256
#define BAR_MAX_LIBRARIES 64

typedef uint32_t BARHandle;
typedef uint32_t BARProcessID;
typedef uint32_t BARWindowID;
typedef uint32_t BARTimerID;
typedef uint32_t BARDialogID;
typedef uint32_t BARResourceID;
typedef uint32_t BARModuleHandle;

typedef enum {
    BAR_MSG_NULL = 0,
    BAR_MSG_CREATE,
    BAR_MSG_DESTROY,
    BAR_MSG_SHOW,
    BAR_MSG_HIDE,
    BAR_MSG_PAINT,
    BAR_MSG_MOUSE_MOVE,
    BAR_MSG_MOUSE_DOWN,
    BAR_MSG_MOUSE_UP,
    BAR_MSG_KEY_DOWN,
    BAR_MSG_KEY_UP,
    BAR_MSG_TIMER,
    BAR_MSG_COMMAND,
    BAR_MSG_CLOSE
} BARMessageType;

typedef struct {
    BARWindowID   window_id;
    BARMessageType type;
    uint32_t      param1;
    uint32_t      param2;
    uint64_t      timestamp;
} BARMessage;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BARRect;

typedef struct {
    BARProcessID process_id;
    char         name[64];
    bool         active;
    uint32_t     window_count;
    uint32_t     security_token;
} BARProcessInfo;

typedef struct {
    BARWindowID window_id;
    BARProcessID process_id;
    BARRect     bounds;
    char        title[128];
    bool        visible;
    bool        focused;
    BARWindowID parent_id;
} BARWindowInfo;

#endif // BOS_BAR_TYPES_H
