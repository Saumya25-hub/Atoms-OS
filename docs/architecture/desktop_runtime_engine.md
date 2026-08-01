# 🏛️ DESKTOP RUNTIME ENGINE (BDR) V1.0 — MASTER SPECIFICATION

> **Subsystem Name:** BOS Desktop Runtime Engine (BDR V1.0)  
> **Interface:** `BDR_` Public API  
> **Architecture Level:** Desktop Session Authority  
> **Integration:** BO-TREE DRE V1.0, BWE Compositor, VFS  

---

## 1. Executive Summary & Architecture Philosophy

The **Desktop Runtime Engine (BDR)** is Phase 3 of the BO-TREE system architecture. It models the **Windows XP Explorer.exe Desktop Runtime** architecture, completely separating desktop state (icons, grid alignment, selection marquee, device discovery, wallpaper state, notifications, context menus, and persistence) from visual rendering code.

### 1.1 The Golden Principle
The Desktop UI layer is strictly a passive renderer. It owns zero path strings, zero icon layout arrays, and zero filesystem state. All desktop state is held, updated, and persisted by the **Desktop Runtime Engine (BDR)**, which communicates exclusively with the **BO-TREE Directory Runtime Engine (DRE)**.

```
+-----------------------------------------------------------------------------------+
|                            DESKTOP UI RENDERER                                    |
|   (Passive View Component: Renders Icons, Selection Box, Wallpaper, Context Menu) |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
|                        DESKTOP RUNTIME ENGINE (BDR V1.0)                          |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Desktop Session  │  │ Icon Manager     │  │ Grid Engine      │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Workspace Engine │  │ Wallpaper Engine │  │ Selection Engine │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Drag & Drop      │  │ Context Menu     │  │ Devices Engine   │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Recycle Bin      │  │ Notifications    │  │ Persistence      │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
|               BO-TREE RUNTIME ENGINE (DRE V1.0 / BDeRuntime)                      |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
|                            VIRTUAL FILESYSTEM LAYER (VFS)                         |
+-----------------------------------------------------------------------------------+
```

---

## 2. Directory Structure (`kernel/desktop_runtime/`)

```
kernel/desktop_runtime/
├── include/                  # Public & Internal Desktop Runtime Headers
│   ├── bdr_types.h           # Desktop Data Structures, Icon Nodes, Grid & State
│   └── bdr_api.h             # Master Public Desktop API (BDR_*)
├── core/                     # 1. Desktop Session Engine
│   └── bdr_core.c            # Desktop_Create, Destroy, Save, Load, Restore
├── icons/                    # 2. Desktop Icon Manager Engine
│   └── bdr_icons.c           # Icon registration, rename, open, shortcut resolve
├── grid/                     # 3. Desktop Grid Engine
│   └── bdr_grid.c            # Grid snap, auto-arrange, collision detection
├── workspace/                # 4. Desktop Workspace Engine
│   └── bdr_workspace.c       # Desktop refresh, selection box, layers
├── wallpaper/                # 5. Wallpaper Engine
│   └── bdr_wallpaper.c       # Solid colors, Stretch, Fit, Fill, Tile modes
├── selection/                # 6. Desktop Selection Engine
│   └── bdr_selection.c       # Single, Ctrl, Shift, Box marquee selection
├── dragdrop/                 # 7. Desktop Drag & Drop Engine
│   └── bdr_dragdrop.c        # Move, Copy, Create Shortcut, Auto-grid update
├── contextmenu/              # 8. Desktop Context Menu Engine
│   └── bdr_contextmenu.c     # Refresh, Paste, New Folder, Properties, Display
├── devices/                  # 9. Desktop Devices Engine
│   └── bdr_devices.c         # USB, NTFS, FAT32 auto-discovery & watcher sync
├── recycle/                  # 10. Recycle Bin Runtime
│   └── bdr_recycle.c         # Trash icon runtime & empty/restore bindings
├── notifications/            # 11. Desktop Notifications Engine
│   └── bdr_notifications.c   # USB insert/remove, file operation toast toasts
├── persistence/              # 12. Desktop Persistence Engine
│   └── bdr_persistence.c     # Icon coordinate & wallpaper registry save/restore
├── diagnostics/              # 13. Forensic Diagnostics & Telemetry
│   └── bdr_diagnostics.c     # Grid utilization, memory, frame cost metrics
├── tests/                    # 25-Test Production Certification Suite
│   └── bdr_certification_tests.c
└── docs/                     # Documentation
    └── desktop_runtime_engine.md # Master Architecture Specification
```

---

## 3. The 13 Subsystem Engines Detailed Specification

### 3.1 Desktop Session Engine (`bdr_core`)
* Manages global desktop session lifecycle per logged-in user.
* Operations: `Desktop_Create()`, `Desktop_Destroy()`, `Desktop_Save()`, `Desktop_Load()`, `Desktop_Reset()`, `Desktop_Restore()`.

### 3.2 Desktop Icon Manager (`bdr_icons`)
* Manages desktop icon representations for files, folders, shortcuts, drive mounts, and system nodes (`virtual://RecycleBin`).
* Handles icon focus, double-click activation, rename operations, and target execution via BO-TREE.

### 3.3 Desktop Grid Engine (`bdr_grid`)
* Enforces desktop icon grid layout (e.g. 80x80px grid slots).
* Supports **Grid Snap**, **Auto Arrange**, **Align to Grid**, and **Collision Detection**.

### 3.4 Desktop Workspace Engine (`bdr_workspace`)
* Tracks workspace bounds, active selection marquee coordinates `(x1, y1, x2, y2)`, and desktop layer invalidate signals.

### 3.5 Wallpaper Engine (`bdr_wallpaper`)
* Manages background image paths and rendering modes: `SOLID_COLOR`, `STRETCH`, `FIT`, `FILL`, `CENTER`, `TILE`.

### 3.6 Desktop Selection Engine (`bdr_selection`)
* Handles desktop marquee box selection and modifier keys (Ctrl+Click, Shift+Click).

### 3.7 Desktop Drag & Drop Engine (`bdr_dragdrop`)
* Drag-and-drop state machine supporting file drop to folders, desktop reordering, and copy/move job creation via BO-TREE Transactions.

### 3.8 Desktop Context Menu Engine (`bdr_contextmenu`)
* Generates desktop right-click menu options: `Refresh`, `Paste`, `New Folder`, `New Shortcut`, `Sort By`, `View Options`, `Personalize`.

### 3.9 Desktop Devices Engine (`bdr_devices`)
* Listens to BO-TREE Watcher events for volume insertion/removal (`/U`, `/C`). Automatically spawns or removes desktop drive icons.

### 3.10 Recycle Bin Runtime (`bdr_recycle`)
* Links the desktop "Recycle Bin" icon directly to BO-TREE Recycle Engine (`virtual://RecycleBin`).

### 3.11 Desktop Notifications Engine (`bdr_notifications`)
* Notification queue broadcasting system toasts for USB device arrival/removal and file transaction completions.

### 3.12 Desktop Persistence Engine (`bdr_persistence`)
* Saves desktop icon coordinates and user preferences to persistent storage (`/DESKTOP/.desktop_state`).

### 3.13 Forensic Diagnostics Engine (`bdr_diagnostics`)
* Tracks grid slot utilization, total desktop objects, selection counts, notification queue depth, and memory usage.

---

## 4. Public API Design (`bdr_api.h`)

```c
#ifndef BDR_API_H
#define BDR_API_H

#include "bdr_types.h"

// Session API
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

// Certification & Diagnostics
void        BDR_GetDiagnostics(BDrDiagnostics* out_diag);

#endif // BDR_API_H
```

---

## 5. 25-Test Production Certification Suite

The certification module [`bdr_certification_tests.c`](file:///D:/Signatures_OS/kernel/desktop_runtime/tests/bdr_certification_tests.c) verifies:

1. `✓ Desktop Session Creation`
2. `✓ Desktop Session Destruction`
3. `✓ Add System Icon (This PC)`
4. `✓ Add System Icon (Recycle Bin)`
5. `✓ Add File Icon`
6. `✓ Grid Snap Alignment`
7. `✓ Grid Auto Arrange`
8. `✓ Grid Collision Detection`
9. `✓ Single Icon Selection`
10. `✓ Box Marquee Selection`
11. `✓ Selection Inversion`
12. `✓ Deselect All`
13. `✓ Set Solid Wallpaper`
14. `✓ Set Stretch Wallpaper`
15. `✓ Drag & Drop Move`
16. `✓ Context Menu Refresh`
17. `✓ USB Device Arrival Event`
18. `✓ USB Device Removal Event`
19. `✓ Recycle Bin Empty Binding`
20. `✓ Push Notification Toast`
21. `✓ Desktop State Persistence Save`
22. `✓ Desktop State Persistence Restore`
23. `✓ Multi-Session Isolation`
24. `✓ Diagnostics Telemetry`
25. `✓ Memory Leak & Stress Test (1,000 Desktop Icons)`
