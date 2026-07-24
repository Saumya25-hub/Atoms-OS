#ifndef ATOMS_APP_MANAGER_H
#define ATOMS_APP_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

// Application State Lifecycles (Phase 9 Specification)
typedef enum {
    ATOMS_APP_STATE_CLOSED = 0,
    ATOMS_APP_STATE_INSTALLING,
    ATOMS_APP_STATE_LOADED,
    ATOMS_APP_STATE_RUNNING,
    ATOMS_APP_STATE_BACKGROUND,
    ATOMS_APP_STATE_SUSPENDED,
    ATOMS_APP_STATE_CLOSING,
    ATOMS_APP_STATE_STOPPED
} ATOMS_AppState;

// Compatibility typedefs for legacy BOS surface callers
typedef ATOMS_AppState BWE_AppState;
#define BWE_APP_STATE_CLOSED    ATOMS_APP_STATE_CLOSED
#define BWE_APP_STATE_STARTING  ATOMS_APP_STATE_LOADED
#define BWE_APP_STATE_RUNNING   ATOMS_APP_STATE_RUNNING
#define BWE_APP_STATE_SUSPENDED ATOMS_APP_STATE_SUSPENDED
#define BWE_APP_STATE_CLOSING   ATOMS_APP_STATE_CLOSING

// Application Callbacks
typedef bwe_error_t (*ATOMS_AppInitFunc)(uint32_t* out_main_window_id);
typedef void (*ATOMS_AppExitFunc)(void);

typedef ATOMS_AppInitFunc BWE_AppInitFunc;
typedef ATOMS_AppExitFunc BWE_AppExitFunc;

#define ATOMS_MAX_APPS 32
#define BWE_MAX_APPS   ATOMS_MAX_APPS
#define ATOMS_MAX_WINDOWS_PER_APP 16

// Production Application Metadata & Mapping Record
typedef struct {
    uint32_t        app_id;             // Global OS App Identifier
    char            name[64];           // Display / Package Name
    char            version[16];        // Semantic Version
    char            executable_path[128];// Path to ELF / Binary if non-builtin
    
    ATOMS_AppState  state;              // App State Machine
    uint32_t        pid;                // Kernel Process ID
    uint32_t        main_window_id;     // Primary BWE Window Surface ID
    uint32_t        window_ids[16];     // All owned window surface IDs
    uint32_t        window_count;       // Count of owned windows
    
    uint32_t        capabilities_mask;  // Security capabilities bitmap
    bool            is_builtin;         // Built-in kernel app vs usermode app
    
    ATOMS_AppInitFunc on_init;
    ATOMS_AppExitFunc on_exit;
} ATOMS_Application;

typedef ATOMS_Application BOS_Application;

// Core Application Manager APIs
void        ATOMS_AppManager_Init(void);
bwe_error_t ATOMS_RegisterApplication(const char* name, const char* version, const char* exec_path,
                                       ATOMS_AppInitFunc on_init, ATOMS_AppExitFunc on_exit,
                                       uint32_t capabilities_mask, uint32_t* out_app_id);

bwe_error_t ATOMS_StartApplication(uint32_t app_id);
bwe_error_t ATOMS_StopApplication(uint32_t app_id);
bwe_error_t ATOMS_StopApplicationByWindow(uint32_t window_id);
bwe_error_t ATOMS_SetApplicationState(uint32_t app_id, ATOMS_AppState new_state);

ATOMS_Application* ATOMS_GetApplication(uint32_t app_id);
ATOMS_Application* ATOMS_GetApplicationByPID(uint32_t pid);
ATOMS_Application* ATOMS_GetApplicationByWindow(uint32_t window_id);

uint32_t    ATOMS_GetActiveAppCount(void);
void        ATOMS_RegisterWindowOwner(uint32_t app_id, uint32_t window_id);
void        ATOMS_UnregisterWindowOwner(uint32_t window_id);
uint32_t    ATOMS_GetFocusedAppID(void);

// Legacy wrappers for backward compatibility with BOSurface
void        BOS_AppManager_Init(void);
bwe_error_t BOS_RegisterApplication(const char* name, const char* version, BWE_AppInitFunc on_init, BWE_AppExitFunc on_exit, uint32_t* out_app_id);
bwe_error_t BOS_StartApplication(uint32_t app_id);
bwe_error_t BOS_StopApplication(uint32_t app_id);
bwe_error_t BOS_StopApplicationByWindow(uint32_t window_id);
BOS_Application* BOS_GetApplication(uint32_t app_id);

#endif // ATOMS_APP_MANAGER_H
