#ifndef BOS_CONTROLPANEL_API_H
#define BOS_CONTROLPANEL_API_H

#include "controlpanel_types.h"

// System Init & Shutdown
int32_t ControlPanelInitialize(void);
void    ControlPanelShutdown(void);

// BOSC Module Management
bool    OpenModule(const char* module_name);
bool    CloseModule(const char* module_name);
bool    ReloadModule(const char* module_name);
bool    SearchModule(const char* query);
bool    InstallModule(const char* module_path);
bool    RemoveModule(const char* module_name);
bool    GetModuleInfo(const char* module_name, BOSC_MODULE_INFO* outInfo);
void    RefreshModules(void);
bool    PinModule(const char* module_name);
bool    UnpinModule(const char* module_name);

// Configuration Operations
bool    OpenSettingsPage(const char* page_name);
bool    SaveConfiguration(const char* section, const char* key, const char* value);
bool    RestoreDefaults(const char* section);

// Diagnostics
void    ControlPanelDumpDiagnostics(void);

#endif // BOS_CONTROLPANEL_API_H
