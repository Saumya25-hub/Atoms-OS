#ifndef BDR_API_H
#define BDR_API_H

#include "bdr_types.h"

// Session Management API
BDrSession* BDR_CreateSession(uint32_t user_id);
void        BDR_DestroySession(BDrSession* session);
int32_t     BDR_LoadSession(BDrSession* session);
int32_t     BDR_SaveSession(BDrSession* session);

// Icon Management API
int32_t     BDR_AddIcon(BDrSession* session, const char* name, const char* path, BDrIconType type, int32_t grid_x, int32_t grid_y);
int32_t     BDR_RemoveIcon(BDrSession* session, uint32_t icon_id);
int32_t     BDR_AutoArrangeIcons(BDrSession* session);

// Grid API
int32_t     BDR_SnapToGrid(int32_t raw_x, int32_t raw_y, int32_t* out_grid_x, int32_t* out_grid_y);

// Selection API
int32_t     BDR_SelectIcon(BDrSession* session, uint32_t icon_id, bool add_to_selection);
int32_t     BDR_SelectBox(BDrSession* session, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
int32_t     BDR_DeselectAll(BDrSession* session);

// Wallpaper API
int32_t     BDR_SetWallpaper(BDrSession* session, const char* image_path, BDrWallpaperMode mode);

// Notifications API
int32_t     BDR_PushNotification(BDrSession* session, const char* title, const char* message, BDrNotificationType type);

// Devices Engine API
int32_t     BDR_RefreshDevices(BDrSession* session);

// Diagnostics API
void        BDR_GetDiagnostics(BDrSession* session, BDrDiagnostics* out_diag);

#endif // BDR_API_H
