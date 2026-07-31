#ifndef BOS_RUNTIME_H
#define BOS_RUNTIME_H

#include "bos_window.h"
#include "bos_events.h"

typedef struct BOS_App BOS_App;

typedef struct {
    const char* app_name;
    const char* version;
    BOS_Result (*on_startup)(BOS_App* app);
    void       (*on_shutdown)(BOS_App* app);
    void       (*on_event)(BOS_App* app, const BOS_Event* event);
} BOS_AppConfig;

struct BOS_App {
    BOS_AppHandle  handle;
    BOS_AppConfig  config;
    BOS_EventQueue event_queue;
    bool           is_running;
    int            exit_code;
    uint32_t       pid;
};

BOS_Result BOS_InitApplication(const BOS_AppConfig* config, BOS_App** out_app);
BOS_Result BOS_RunApplication(BOS_App* app);
void       BOS_QuitApplication(BOS_App* app, int exit_code);
BOS_App*   BOS_GetCurrentApplication(void);

#endif /* BOS_RUNTIME_H */
