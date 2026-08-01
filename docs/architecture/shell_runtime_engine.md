# 🏛️ BOS SHELL RUNTIME ENGINE (BSR V1.0) — MASTER SPECIFICATION

> **Subsystem Name:** BOS Shell Runtime Engine (BSR V1.0)  
> **Master Role:** Master Shell Authority of Signatures OS  
> **Layer:** Master Controller above DRE, BDR, and BO-TREE  
> **Public Interface:** `BSR_*` Unified API  

---

## 1. Executive Summary & Architecture Philosophy

The **BOS Shell Runtime Engine (BSR V1.0)** is the top-level Master Shell Authority of Signatures OS. It acts as the central control brain across all user-facing desktop elements (Explorer windows, Desktop environment, Terminal sessions, File Dialogs, AI interface, and Application Launchers).

### 1.1 Architectural Hierarchy
No application component (Explorer, Desktop, Terminal, File Dialog, Browser, AI) contains business logic, path management, file associations, clipboard queues, drag-and-drop engines, or search pipelines. Every user interaction is dispatched to **BSR**, which routes execution down through **DRE**, **BDR**, **BO-TREE**, and **VFS**:

```
                       USER INPUT (Mouse / Keyboard)
                                    │
                                    ▼
                     BOS SHELL RUNTIME ENGINE (BSR V1.0)
                ┌───────────────────┼───────────────────┐
                │                   │                   │
         Explorer Windows    Desktop Session     Terminal Session
                │                   │                   │
                └───────────────────┼───────────────────┘
                                    │
                                    ▼
                    DIRECTORY RUNTIME ENGINE (DRE V1.0)
                                    │
                                    ▼
                       BO-TREE ENGINE (BDE V1.0)
                                    │
                                    ▼
                      VIRTUAL FILESYSTEM LAYER (VFS)
                                    │
                                    ▼
                 PHYSICAL STORAGE (NTFS / FAT32 / USB)
```

---

## 2. Directory Structure (`kernel/shell_runtime/`)

```
kernel/shell_runtime/
├── include/                  # Public & Internal BSR Headers
│   ├── bsr_types.h           # BSR_Runtime Struct, Enums, Handles, & Flags
│   └── bsr_api.h             # Master Public Shell Runtime API (BSR_*)
├── core/                     # 1. Core Runtime Engine
│   └── bsr_core.c            # BSR_Init, BSR_CreateRuntime, BSR_DestroyRuntime
├── session/                  # 2. Session Engine
│   └── bsr_session.c         # Session creation & application binding
├── dispatcher/               # 3. Dispatcher Engine
│   └── bsr_dispatcher.c      # BSR_Open, BSR_Delete, BSR_Copy, BSR_Move, BSR_Rename
├── navigation/               # 4. Navigation Engine
│   └── bsr_navigation.c      # Unified shell navigation & history stack dispatch
├── clipboard/                # 5. Clipboard Engine
│   └── bsr_clipboard.c       # Global shell clipboard (Copy, Cut, Paste)
├── dragdrop/                 # 6. Drag Drop Engine
│   └── bsr_dragdrop.c        # Shell drag-and-drop state machine
├── contextmenu/              # 7. Context Menu Engine
│   └── bsr_contextmenu.c     # Unified context menu generator
├── dialogs/                  # 8. Dialog Engine
│   └── bsr_dialogs.c         # System File Open / Save Dialog controller
├── association/              # 9. File Association Engine
│   └── bsr_file_association.c# File extension-to-application mapping (.txt, .bmp, .elf)
├── launcher/                 # 10. Launcher Engine
│   └── bsr_launcher.c        # Double-click handler -> Application spawn
├── search/                   # 11. Search Engine
│   └── bsr_search.c          # Master search router across BO-TREE & DRE
├── recent/                   # 12. Recent Files Engine
│   └── bsr_recent.c          # LRU recent files history tracker
├── favorites/                # 13. Favorites Engine
│   └── bsr_favorites.c       # Quick access / pinned favorite folders
├── notifications/            # 14. Notification Engine
│   └── bsr_notifications.c   # Master shell notification broadcast engine
├── diagnostics/              # 15. Diagnostics Engine
│   └── bsr_diagnostics.c     # Shell latency, open time, memory & telemetry
├── tests/                    # 30-Test Production Certification Suite
│   └── bsr_certification_tests.c
└── docs/                     # Documentation
    └── shell_runtime_engine.md # Master Architecture Specification
```

---

## 3. The 15 Core Runtime Engines Detailed Specification

### 3.1 Core Runtime Engine (`bsr_core`)
* Manages global BSR initialization (`BSR_Init()`) and master runtime instances.
* Allocates and releases `BSR_Runtime` handles.

### 3.2 Session Engine (`bsr_session`)
* Manages isolated runtime environments for multiple Explorer windows, Terminal sessions, and Desktop sessions.

### 3.3 Dispatcher Engine (`bsr_dispatcher`)
* Centralized command router. All application UI layers invoke dispatcher methods: `BSR_Open()`, `BSR_Copy()`, `BSR_Move()`, `BSR_Delete()`, `BSR_Rename()`, `BSR_NewFolder()`.

### 3.4 Navigation Engine (`bsr_navigation`)
* Routes navigation requests from Explorer, Terminal, or File Dialogs through DRE sessions.

### 3.5 Clipboard Engine (`bsr_clipboard`)
* System-wide master shell clipboard. Enables cross-app copy-paste (e.g. Copy in Explorer $\rightarrow$ Paste on Desktop).

### 3.6 Drag & Drop Engine (`bsr_dragdrop`)
* Tracks mouse drag gestures from source views to target drops, automatically triggering file copy/move transactions.

### 3.7 Context Menu Engine (`bsr_contextmenu`)
* Constructs context menus dynamically based on file selection type and system capabilities.

### 3.8 Dialog Engine (`bsr_dialogs`)
* Controls reusable system Open/Save File Dialogs powered by DRE.

### 3.9 File Association Engine (`bsr_file_association`)
* Maps file extensions to handler applications:
  - `.txt` / `.md` $\rightarrow$ Text Viewer
  - `.bmp` / `.png` $\rightarrow$ Image Viewer
  - `.elf` / `.bin` $\rightarrow$ Horse Engine Process Spawner

### 3.10 Launcher Engine (`bsr_launcher`)
* Intercepts double-click / Enter activation events, queries File Associations, and launches target processes.

### 3.11 Search Engine (`bsr_search`)
* Single search entry point used by Explorer, Desktop, AI Engine, and Terminal.

### 3.12 Recent Files Engine (`bsr_recent`)
* Tracks the 50 most recently opened files and folders across all applications.

### 3.13 Favorites Engine (`bsr_favorites`)
* Manages pinned quick-access locations ("Desktop", "Documents", "Downloads", "Pictures").

### 3.14 Notification Engine (`bsr_notifications`)
* Master notification dispatcher broadcasting system toasts and refreshing active windows.

### 3.15 Diagnostics Engine (`bsr_diagnostics`)
* Tracks shell latency, folder switch times, file association lookup speed, and memory usage.

---

## 4. Public API Design (`bsr_api.h`)

```c
#ifndef BSR_API_H
#define BSR_API_H

#include "bsr_types.h"

// Runtime Creation & Lifecycle
BSR_Runtime* BSR_CreateRuntime(uint32_t owner_pid, BSR_AppType app_type);
void         BSR_DestroyRuntime(BSR_Runtime* rt);

// Master Dispatcher API
int32_t      BSR_Open(BSR_Runtime* rt, const char* target_path);
int32_t      BSR_Back(BSR_Runtime* rt);
int32_t      BSR_Forward(BSR_Runtime* rt);
int32_t      BSR_Up(BSR_Runtime* rt);
int32_t      BSR_Copy(BSR_Runtime* rt);
int32_t      BSR_Cut(BSR_Runtime* rt);
int32_t      BSR_Paste(BSR_Runtime* rt, const char* target_dir);
int32_t      BSR_Delete(BSR_Runtime* rt, bool send_to_recycle);
int32_t      BSR_Rename(BSR_Runtime* rt, const char* old_name, const char* new_name);
int32_t      BSR_NewFolder(BSR_Runtime* rt, const char* folder_name);

// Launcher & Associations
int32_t      BSR_Launch(const char* file_path);
int32_t      BSR_RegisterAssociation(const char* ext, const char* app_name, const char* app_path);

// File Dialog & Search
int32_t      BSR_ShowFileDialog(BSR_Runtime* rt, const char* title, bool is_save, char* out_selected_path, size_t max_len);
int32_t      BSR_Search(const char* query_pattern, BDeDirEntry** out_results, uint32_t* out_count);

// Diagnostics
void         BSR_GetDiagnostics(BSR_Diagnostics* out_diag);

#endif // BSR_API_H
```

---

## 5. 30-Test Production Certification Suite

The certification module [`bsr_certification_tests.c`](file:///D:/Signatures_OS/kernel/shell_runtime/tests/bsr_certification_tests.c) validates:

1. `✓ BSR Subsystem Initialization`
2. `✓ Master Runtime Creation`
3. `✓ Master Runtime Destruction`
4. `✓ Explorer Session Integration`
5. `✓ Desktop Session Integration`
6. `✓ Terminal Session Integration`
7. `✓ Navigation Open Dispatch`
8. `✓ Navigation Back Dispatch`
9. `✓ Navigation Forward Dispatch`
10. `✓ Navigation Up Dispatch`
11. `✓ Global Clipboard Copy`
12. `✓ Global Clipboard Cut`
13. `✓ Global Clipboard Paste`
14. `✓ Drag & Drop Session Begin`
15. `✓ Drag & Drop Drop Execution`
16. `✓ Context Menu Generation`
17. `✓ System File Open Dialog`
18. `✓ File Association Query (.txt)`
19. `✓ File Association Query (.bmp)`
20. `✓ Application Launcher Dispatch`
21. `✓ Master Search Query`
22. `✓ Recent Files Tracking`
23. `✓ Favorites Pinning`
24. `✓ Notification Broadcast`
25. `✓ Multi-Runtime Session Isolation`
26. `✓ USB Device Hotplug Event Dispatch`
27. `✓ Cache Invalidation Sync`
28. `✓ Diagnostics Telemetry`
29. `✓ Memory Leak Verification`
30. `✓ Stress Test (100,000 Operations)`
