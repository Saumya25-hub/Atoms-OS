#ifndef BOS_SETTINGS_API_H
#define BOS_SETTINGS_API_H

#include "settings_types.h"

// Lifecycle
int32_t SettingsInitialize(void);
void    SettingsShutdown(void);

// Navigation & Category Launchers
bool    OpenCategory(SETTINGS_CATEGORY_ID categoryId);
bool    SearchSettings(const char* query);
void    RefreshSettings(void);
bool    OpenAbout(void);
bool    OpenUpdates(void);
bool    OpenNetwork(void);
bool    OpenDisplay(void);
bool    OpenAccounts(void);
bool    OpenStorage(void);
bool    OpenPrivacy(void);
bool    OpenAccessibility(void);

// Apply & Revert Changes
bool    ApplyChanges(void);
bool    DiscardChanges(void);

// Diagnostics
void    SettingsDumpDiagnostics(void);

#endif // BOS_SETTINGS_API_H
