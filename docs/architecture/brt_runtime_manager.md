# 🏛️ BO-TREE RUNTIME INTEGRATION LAYER (BRT V1.0) — MASTER SPECIFICATION

> **Subsystem Name:** BO-TREE Runtime Integration Layer (BRT V1.0)  
> **Master Role:** Highest Runtime Authority for Filesystem & Shell Coordination  
> **Architecture Level:** Phase 6 Master Backbone  
> **Public Interface:** `BRT_*` Master API & `brt.sll`  

---

## 1. Executive Summary & Architectural Philosophy

The **BO-TREE Runtime Integration Layer (BRT V1.0)** is the ultimate runtime authority of Signatures OS. It acts as the central nervous system that coordinates all lower-level runtime subsystems (**BSR**, **BDR**, **DRE**, and **BO-TREE Core**), serving as the unified gateway for every application (Explorer, Desktop, Terminal, Browser, AI, File Dialogs, Settings, Media Player, and Developer Tools).

### 1.1 Architectural Hierarchy
No application or window touches BSR, BDR, DRE, or BO-TREE directly. All applications instantiate or bind to a `BRTRuntime*` object, which routes requests down through the system hierarchy:

```
+-----------------------------------------------------------------------------------+
|                                   APPLICATIONS                                    |
|   (Explorer, Desktop, Terminal, Browser, AI Engine, File Dialogs, Settings, Apps) |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
|               BO-TREE RUNTIME INTEGRATION LAYER (BRT V1.0 / BRTRuntime)          |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Runtime Registry │  │ Dispatcher       │  │ Object Manager   │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Event Hub        │  │ Shared Cache     │  │ Master Clipboard │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Drag & Drop      │  │ Notifications    │  │ Search Engine    │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Security Engine  │  │ Transaction Queue│  │ Diagnostics Hub  │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
+-----------------------------------------------------------------------------------+
                                          │
                   ┌──────────────────────┼──────────────────────┐
                   ▼                      ▼                      ▼
            ┌──────────────┐       ┌──────────────┐       ┌──────────────┐
            │  BSR Engine  │       │  BDR Engine  │       │  DRE Engine  │
            └──────────────┘       └──────────────┘       └──────────────┘
                   │                      │                      │
                   └──────────────────────┼──────────────────────┘
                                          │
                                          ▼
                             BO-TREE CORE ENGINE (BDE)
                                          │
                                          ▼
                            VIRTUAL FILESYSTEM LAYER (VFS)
                                          │
                                          ▼
                        PHYSICAL STORAGE (NTFS / FAT32 / USB)
```

---

## 2. Directory Structure (`kernel/brt/`)

```
kernel/brt/
├── include/                  # Master BRT Headers
│   ├── brt_types.h           # BRTRuntime, BRTObject, Enums, Handles & Metrics
│   └── brt_api.h             # Master Public BRT API (BRT_*)
├── core/                     # 1. Core Runtime Engine
│   └── brt_core.c            # BRT_Init, BRT_CreateRuntime, BRT_DestroyRuntime
├── registry/                 # 2. Runtime Registry Engine
│   └── brt_registry.c        # BRT_RegisterRuntime, PID & Window mapping pool
├── dispatcher/               # 3. Runtime Dispatcher Engine
│   └── brt_dispatcher.c       # BRT_Open, BRT_Delete, BRT_Rename, BRT_Copy, BRT_Paste
├── objects/                  # 4. Runtime Object Manager
│   └── brt_objects.c         # BRT_CreateObject, refcounts, ownership, handles
├── runtime/                  # 5. Runtime Session Engine
│   └── brt_session.c         # BRT_SaveRuntime, BRT_RestoreRuntime, serialization
├── cache/                    # 6. Runtime Cache Engine
│   └── brt_cache.c           # O(1) LRU shared cache, metadata & icon cache
├── events/                   # 7. Runtime Event Engine
│   └── brt_events.c          # BRT_PostEvent, BRT_Subscribe, async event bus
├── clipboard/                # 8. Runtime Clipboard Engine
│   └── brt_clipboard.c       # System-wide master clipboard (BRT_SetClipboard)
├── dragdrop/                 # 9. Runtime Drag & Drop Engine
│   └── brt_dragdrop.c        # Cross-window & cross-volume drag sessions
├── notifications/            # 10. Runtime Notification Engine
│   └── brt_notifications.c   # Unified toast & status bar notification authority
├── search/                   # 11. Runtime Search Engine
│   └── brt_search.c          # Instant live search & wildcard search router
├── security/                 # 12. Runtime Security Engine
│   └── brt_security.c        # Permission context & capability checks
├── transactions/             # 13. Runtime Transaction Engine
│   └── brt_transactions.c    # BRT_StartTransaction, Commit, Rollback
├── diagnostics/              # 14. Runtime Diagnostics Engine
│   └── brt_diagnostics.c     # Latency, object count, lock contention metrics
├── tests/                    # 50-Test Production Certification Suite
│   └── brt_certification_tests.c
└── docs/                     # Documentation
    └── brt_runtime_manager.md# Master Architecture Specification
```

---

## 3. The 14 Core Subsystem Engines Detailed Specification

### 3.1 Core Runtime Engine (`brt_core`)
* Manages global subsystem lifecycle (`BRT_Init()`) and master `BRTRuntime` instances.
* Allocates thread-safe, ref-counted runtime structures.

### 3.2 Registry Engine (`brt_registry`)
* Maintains an $O(1)$ lookup table mapping `owner_pid` and `window_id` to `BRTRuntime*`.

### 3.3 Dispatcher Engine (`brt_dispatcher`)
* Unified action router. Performs `BRT_Open()`, `BRT_Launch()`, `BRT_Delete()`, `BRT_Rename()`, `BRT_Copy()`, `BRT_Paste()`, `BRT_NewFolder()`.

### 3.4 Object Manager (`brt_objects`)
* Manages `BRTObject` handles with atomic reference counting (`ref_count`), ownership verification, and security context tags.

### 3.5 Session Engine (`brt_session`)
* Handles session serialization, state snapshotting, suspend/resume, and crash recovery.

### 3.6 Cache Engine (`brt_cache`)
* $O(1)$ hash table with LRU eviction for directory entries, icons, and metadata.

### 3.7 Event Engine (`brt_events`)
* Event publication/subscription bus (`BRT_PostEvent()`, `BRT_Subscribe()`) for volume insertion, file changes, and UI invalidation.

### 3.8 Clipboard Engine (`brt_clipboard`)
* System-wide master clipboard.

### 3.9 Drag & Drop Engine (`brt_dragdrop`)
* Manages `BRT_BeginDrag()`, `BRT_UpdateDrag()`, `BRT_EndDrag()`.

### 3.10 Notification Engine (`brt_notifications`)
* Unified toast and status bar dispatcher.

### 3.11 Search Engine (`brt_search`)
* Multi-threaded instant search query router.

### 3.12 Security Engine (`brt_security`)
* Capability checking and access control validation before performing filesystem mutations.

### 3.13 Transaction Engine (`brt_transactions`)
* Multi-step job queue supporting `BRT_StartTransaction()`, `BRT_CommitTransaction()`, `BRT_RollbackTransaction()`.

### 3.14 Diagnostics Engine (`brt_diagnostics`)
* Tracks API latency, handle leaks, memory consumption, and lock contention.

---

## 4. Master Public API (`brt_api.h`)

```c
#ifndef BRT_API_H
#define BRT_API_H

#include "brt_types.h"

// Subsystem Init & Master Runtime Creation
int32_t      BRT_Init(void);
BRTRuntime*  BRT_CreateRuntime(uint32_t owner_pid, BRT_AppType app_type);
void         BRT_DestroyRuntime(BRTRuntime* rt);

// Object Manager API
BRTObject*   BRT_CreateObject(BRTRuntime* rt, const char* name, BRTObjectType type);
void         BRT_RetainObject(BRTObject* obj);
void         BRT_ReleaseObject(BRTObject* obj);

// Master Dispatcher API
int32_t      BRT_Open(BRTRuntime* rt, const char* target_path);
int32_t      BRT_Back(BRTRuntime* rt);
int32_t      BRT_Forward(BRTRuntime* rt);
int32_t      BRT_Up(BRTRuntime* rt);
int32_t      BRT_Refresh(BRTRuntime* rt);
int32_t      BRT_Copy(BRTRuntime* rt);
int32_t      BRT_Move(BRTRuntime* rt, const char* dest_dir);
int32_t      BRT_Delete(BRTRuntime* rt, bool send_to_recycle);
int32_t      BRT_Rename(BRTRuntime* rt, const char* old_name, const char* new_name);
int32_t      BRT_NewFolder(BRTRuntime* rt, const char* folder_name);
int32_t      BRT_Launch(const char* file_path);

// Event Engine & Clipboard
int32_t      BRT_PostEvent(uint32_t event_id, void* payload);
int32_t      BRT_Subscribe(uint32_t event_id, BRTEventCallback cb);
int32_t      BRT_SetClipboard(const char* path, bool is_cut);
int32_t      BRT_GetClipboard(char* out_path, size_t max_len, bool* out_is_cut);

// Drag & Drop
int32_t      BRT_BeginDrag(BRTRuntime* rt, int32_t start_x, int32_t start_y);
int32_t      BRT_UpdateDrag(BRTRuntime* rt, int32_t cur_x, int32_t cur_y);
int32_t      BRT_EndDrag(BRTRuntime* rt, const char* drop_target);

// Transactions
BRTTxHandle  BRT_StartTransaction(BRTRuntime* rt, BRTTxType type);
int32_t      BRT_CommitTransaction(BRTTxHandle tx);
int32_t      BRT_RollbackTransaction(BRTTxHandle tx);

// Diagnostics & Security
void         BRT_GetDiagnostics(BRT_Diagnostics* out_diag);

#endif // BRT_API_H
```

---

## 5. 50-Test Production Certification Suite

The certification module [`brt_certification_tests.c`](file:///D:/Signatures_OS/kernel/brt/tests/brt_certification_tests.c) validates:

1. `✓ BRT Subsystem Initialization`
2. `✓ Master Runtime Creation (Explorer)`
3. `✓ Master Runtime Creation (Desktop)`
4. `✓ Master Runtime Creation (Terminal)`
5. `✓ Master Runtime Creation (Browser)`
6. `✓ Master Runtime Creation (AI Engine)`
7. `✓ Master Runtime Creation (File Dialog)`
8. `✓ Registry PID Mapping`
9. `✓ Registry Window Mapping`
10. `✓ Object Manager Allocation`
11. `✓ Object Reference Counting (Retain)`
12. `✓ Object Reference Counting (Release)`
13. `✓ BSR Authority Link`
14. `✓ BDR Authority Link`
15. `✓ DRE Authority Link`
16. `✓ Navigation Open Dispatch`
17. `✓ Navigation Back Dispatch`
18. `✓ Navigation Forward Dispatch`
19. `✓ Navigation Up Dispatch`
20. `✓ Navigation Refresh Dispatch`
21. `✓ Master Clipboard Set`
22. `✓ Master Clipboard Get`
23. `✓ Drag Session Begin`
24. `✓ Drag Session Update`
25. `✓ Drag Session End Drop`
26. `✓ Event Bus Publish`
27. `✓ Event Bus Subscription`
28. `✓ Broadcast USB Hotplug Event`
29. `✓ Notification Toast Queue`
30. `✓ Instant Search Router`
31. `✓ Security Capability Validation`
32. `✓ Start Transaction`
33. `✓ Commit Transaction`
34. `✓ Rollback Transaction`
35. `✓ Session State Save`
36. `✓ Session State Restore`
37. `✓ Shared Cache O(1) Insertion`
38. `✓ Shared Cache O(1) Lookup`
39. `✓ Shared Cache LRU Eviction`
40. `✓ Multi-Runtime Session Isolation`
41. `✓ Multi-App Clipboard Sharing`
42. `✓ RW Lock Acquisition`
43. `✓ Deadlock Detection`
44. `✓ Diagnostics Latency Telemetry`
45. `✓ Diagnostics Memory Telemetry`
46. `✓ Runtime Destroy`
47. `✓ Object Destruction`
48. `✓ Memory Leak Verification`
49. `✓ Race Condition Validation`
50. `✓ Stress Test (100,000 Operations)`
