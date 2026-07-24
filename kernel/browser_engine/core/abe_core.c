#include "abe_core.h"
#include "kernel/core/lib/include/string.h"

static ABE_Runtime g_abe_runtime;

ABE_Error ABE_Core_Initialize(const ABE_Config* config) {
    if (g_abe_runtime.is_initialized) return ABE_ERR_ALREADY_INITIALIZED;
    memset(&g_abe_runtime, 0, sizeof(ABE_Runtime));

    // 1. Diagnostics Subsystem
    ABE_Diagnostics_Init();
    ABE_Log(ABE_LOG_INFO, "CORE", "=========================================================");
    ABE_Log(ABE_LOG_INFO, "CORE", " Initializing ATOMS Browser Engine (ABE V1.0 - Phase 1)  ");
    ABE_Log(ABE_LOG_INFO, "CORE", "=========================================================");

    // 2. Configuration Subsystem
    ABE_Error err = ABE_Config_Init(config);
    if (err != ABE_SUCCESS) return err;
    g_abe_runtime.config = *ABE_Config_Get();

    // 3. Feature Registry & Capabilities
    err = ABE_Feature_Init();
    if (err != ABE_SUCCESS) return err;

    // 4. Resource Manager
    err = ABE_ResourceManager_Init(g_abe_runtime.config.max_memory_pool_bytes);
    if (err != ABE_SUCCESS) return err;

    // 5. Browser Process Manager
    err = ABE_Process_Init();
    if (err != ABE_SUCCESS) return err;

    // 6. Session Lifecycle
    err = ABE_Session_Init();
    if (err != ABE_SUCCESS) return err;

    // 7. Browser Window Manager (BOSurface integration)
    err = ABE_Window_Init();
    if (err != ABE_SUCCESS) return err;

    // 8. Tab Engine
    err = ABE_TabEngine_Init();
    if (err != ABE_SUCCESS) return err;

    // 9. URL Engine
    err = ABE_URL_Init();
    if (err != ABE_SUCCESS) return err;

    // 10. Navigation Engine
    err = ABE_Navigation_Init();
    if (err != ABE_SUCCESS) return err;

    g_abe_runtime.is_initialized = true;
    g_abe_runtime.init_timestamp = 1000;
    g_abe_runtime.engine_id = 9001;

    ABE_Log(ABE_LOG_INFO, "CORE", "ABE Phase 1 Browser Core Foundation successfully initialized!");
    return ABE_SUCCESS;
}

ABE_Error ABE_Core_Shutdown(void) {
    if (!g_abe_runtime.is_initialized) return ABE_ERR_NOT_INITIALIZED;

    ABE_Log(ABE_LOG_INFO, "CORE", "Shutting down ATOMS Browser Engine (ABE Phase 1)...");

    ABE_Navigation_Init(); // Shutdown stubs
    ABE_TabEngine_Shutdown();
    ABE_Window_Shutdown();
    ABE_Session_Shutdown();
    ABE_Process_Shutdown();
    ABE_ResourceManager_Shutdown();
    ABE_Feature_Shutdown();
    ABE_Config_Shutdown();

    g_abe_runtime.is_initialized = false;
    ABE_Diagnostics_Shutdown();
    return ABE_SUCCESS;
}

bool ABE_Core_IsInitialized(void) {
    return g_abe_runtime.is_initialized;
}

ABE_Runtime* ABE_Core_GetRuntime(void) {
    return &g_abe_runtime;
}
