#ifndef BWE_APP_MANAGER_H
#define BWE_APP_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "surface.h"

// Application State
typedef enum {
    BWE_APP_STATE_CLOSED,
    BWE_APP_STATE_STARTING,
    BWE_APP_STATE_RUNNING,
    BWE_APP_STATE_SUSPENDED,
    BWE_APP_STATE_CLOSING
} BWE_AppState;

// Application Callbacks
typedef bwe_error_t (*BWE_AppInitFunc)(uint32_t* out_main_window_id);
typedef void (*BWE_AppExitFunc)(void);

#define BWE_MAX_APPS 16

typedef struct {
    uint32_t        app_id;         // Unique ID across OS
    char            name[64];
    char            version[16];
    
    BWE_AppState    state;
    uint32_t        pid;            // OS Process ID
    uint32_t        main_window_id; // Primary surface ID
    
    BWE_AppInitFunc on_init;
    BWE_AppExitFunc on_exit;
} BOS_Application;

// API
void        BOS_AppManager_Init(void);
bwe_error_t BOS_RegisterApplication(const char* name, const char* version, BWE_AppInitFunc on_init, BWE_AppExitFunc on_exit, uint32_t* out_app_id);
bwe_error_t BOS_StartApplication(uint32_t app_id);
bwe_error_t BOS_StopApplication(uint32_t app_id);
bwe_error_t BOS_StopApplicationByWindow(uint32_t window_id);
BOS_Application* BOS_GetApplication(uint32_t app_id);

#endif // BWE_APP_MANAGER_H
