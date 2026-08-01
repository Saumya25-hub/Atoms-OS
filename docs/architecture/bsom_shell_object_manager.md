# 🏛️ BOS SHELL OBJECT MANAGER (BSOM V1.0) — MASTER SPECIFICATION

> **Subsystem Name:** BOS Shell Object Manager (BSOM V1.0)  
> **Master Role:** Universal Object Layer for Filesystem & Shell Entities  
> **Architecture Level:** Phase 8 Master Backbone  
> **Public Library Interface:** `BSOM_*` Master API & `bsom.sll`  

---

## 1. Executive Summary & Universal Object Architecture

The **BOS Shell Object Manager (BSOM V1.0)** is Phase 8 of the Signatures OS system architecture. It creates a **Universal Object Model** over every filesystem entity, runtime session, device, and UI resource in the operating system.

Applications never manipulate raw path strings or VFS file descriptors directly. Instead, every entity is instantiated as a ref-counted `BSOMObject` handle, allowing uniform properties, context menu generation, icon/thumbnail binding, permission enforcement, drag-drop sessions, and transaction tracing.

```text
+-----------------------------------------------------------------------------------+
|                                   APPLICATIONS                                    |
|   (Explorer, Desktop, Terminal, Browser, AI Engine, Settings, Dialogs, Media)     |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
|                 BOS SHELL OBJECT MANAGER LAYER (BSOM V1.0 / BSOMObject)           |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Core Object Mgmt │  │ Object Registry  │  │ Handle Manager   │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Object Factory   │  │ Object Resolver  │  │ Namespace Engine │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Property System  │  │ Context Menu Engine│ │ Thumbnail Engine │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Icon Engine      │  │ Clipboard Engine │  │ Drag Drop Engine │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Search Engine    │  │ Recent Engine    │  │ Favorites Engine │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                 |
|  │ Recycle Engine   │  │ Permissions Engine│ │ Diagnostics Hub  │                 |
|  └──────────────────┘  └──────────────────┘  └──────────────────┘                 |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
                      BRT RUNTIME MANAGER LAYER (BRT V1.0)
                                          │
                                          ▼
                   BO-TREE FILESYSTEM SERVICES LAYER (BFS V1.0)
                                          │
                                          ▼
                            BO-TREE CORE ENGINE (BDE V1.0)
                                          │
                                          ▼
                            VIRTUAL FILESYSTEM LAYER (VFS)
                                          │
                                          ▼
                      STORAGE (NTFS / FAT32 / ISO / USB / BOSFS)
```

---

## 2. Supported BSOM Object Class Types (`BSOMClassType`)

1. `BSOM_CLASS_FILE` (Regular File Object)
2. `BSOM_CLASS_FOLDER` (Directory Object)
3. `BSOM_CLASS_DRIVE` (Physical Partition / Volume Object)
4. `BSOM_CLASS_USB` (Hotplug USB Device Object)
5. `BSOM_CLASS_SHORTCUT` (.lnk / OS Shortcut Object)
6. `BSOM_CLASS_APP` (Executable Application Object)
7. `BSOM_CLASS_IMAGE` (Bitmap / PNG Image Asset Object)
8. `BSOM_CLASS_DOCUMENT` (Text / PDF Document Object)
9. `BSOM_CLASS_AUDIO` (PCM / WAV Audio Stream Object)
10. `BSOM_CLASS_VIDEO` (Video Media Stream Object)
11. `BSOM_CLASS_VIRTUAL` (Virtual Shell Namespace Object)
12. `BSOM_CLASS_NETWORK` (Network Share Object)
13. `BSOM_CLASS_SEARCH` (Search Result Object Set)
14. `BSOM_CLASS_RECENT` (Recent Database Entry Object)
15. `BSOM_CLASS_FAVORITE` (User Pinned Favorite Object)
16. `BSOM_CLASS_RECYCLE` (Recycle Bin Trash Entry Object)
17. `BSOM_CLASS_CLIPBOARD` (Global Clipboard Container Object)
18. `BSOM_CLASS_DRAG` (Active Drag-and-Drop Session Object)
19. `BSOM_CLASS_THUMBNAIL` (Rendered Bitmap Preview Object)
20. `BSOM_CLASS_ICON` (System Icon Resource Object)
21. `BSOM_CLASS_PROPERTY` (Key-Value Metadata Property Object)
22. `BSOM_CLASS_PERMISSION` (Security ACL Policy Object)
23. `BSOM_CLASS_TRANSACTION` (Multi-Step File Transaction Object)
24. `BSOM_CLASS_CONTEXTMENU` (Interactive Shell Context Menu Object)
25. `BSOM_CLASS_AI_WORKSPACE` (AI Workspace Context Container Object)

---

## 3. Directory Structure (`kernel/bsom/`)

```
kernel/bsom/
├── include/                  # Master BSOM Headers
│   ├── bsom_types.h          # BSOMObject, Handles, Classes & Metrics
│   ├── bsom_api.h            # Public Master API (BSOM_*)
│   └── bsom.h                # Convenience Header
├── core/                     # 1. Core Object Manager
│   └── bsom_core.c           # BSOM_Init, BSOM_CreateObject, BSOM_Retain, BSOM_Release
├── registry/                 # 2. Object Registry
│   └── bsom_registry.c       # Global object table and PID ownership lookup
├── factory/                  # 3. Object Factory
│   └── bsom_factory.c        # Spawns typed objects (file, folder, drive, app)
├── handles/                  # 4. Handle Manager
│   └── bsom_handles.c        # Secure handle table mapping handles to BSOMObject*
├── lifetime/                 # 5. Lifetime Manager
│   └── bsom_lifetime.c       # Garbage collection and pool reclamation
├── resolver/                 # 6. Object Resolver
│   └── bsom_resolver.c       # Resolves paths or URIs to BSOMObject*
├── namespace/                # 7. Namespace Object Engine
│   └── bsom_namespace.c      # Virtual shell objects (ThisPC, Desktop, Trash)
├── properties/               # 8. Property & Context Menu Engine
│   └── bsom_properties.c     # BSOM_GetProperty, BSOM_SetProperty, Context Menu
├── icons/                    # 9. Icon Engine
│   └── bsom_icons.c          # Icon object creation and caching
├── thumbnails/               # 10. Thumbnail Engine
│   └── bsom_thumbnails.c     # Thumbnail bitmap object generator
├── clipboard/                # 11. Clipboard Engine
│   └── bsom_clipboard.c      # Clipboard object store
├── dragdrop/                 # 12. Drag & Drop Engine
│   └── bsom_dragdrop.c       # Active drag session object
├── search/                   # 13. Search Engine
│   └── bsom_search.c         # Search result object generator
├── recent/                   # 14. Recent Engine
│   └── bsom_recent.c         # Recent item objects
├── favorites/                # 15. Favorites Engine
│   └── bsom_favorites.c      # Favorite item objects
├── recycle/                  # 16. Recycle Engine
│   └── bsom_recycle.c        # Trash item objects
├── permissions/              # 17. Permissions Engine
│   └── bsom_permissions.c   # Security policy verification
├── metadata/                 # 18. Metadata Engine
│   └── bsom_metadata.c       # Extended attribute objects
├── cache/                    # 19. Object Cache
│   └── bsom_cache.c          # O(1) hash table LRU object cache
├── diagnostics/              # 20. Diagnostics Hub
│   └── bsom_diagnostics.c    # Object count, refcounts, handle leaks telemetry
├── tests/                    # 75-Test Production Certification Suite
│   └── bsom_certification_tests.c
└── docs/                     # Documentation
    └── bsom_shell_object_manager.md
```

---

## 4. Master Public API (`bsom_api.h`)

```c
#ifndef BSOM_API_H
#define BSOM_API_H

#include "bsom_types.h"

// Initialization
int32_t       BSOM_Init(void);

// Object Lifecycle & Handles
BSOMObject*   BSOM_CreateObject(const char* name, BSOMClassType class_type);
BSOMHandle    BSOM_OpenObject(const char* path_or_uri);
void          BSOM_CloseObject(BSOMHandle handle);
void          BSOM_Retain(BSOMObject* obj);
void          BSOM_Release(BSOMObject* obj);

// Properties & Attributes
int32_t       BSOM_GetProperty(BSOMObject* obj, const char* key, char* out_val, size_t max_len);
int32_t       BSOM_SetProperty(BSOMObject* obj, const char* key, const char* val);
int32_t       BSOM_ShowProperties(BSOMObject* obj);

// Object Operations
int32_t       BSOM_GetChildren(BSOMObject* obj, BSOMObject*** out_children, uint32_t* out_count);
int32_t       BSOM_Enumerate(BSOMObject* obj, BSOMEnumCallback cb);
int32_t       BSOM_Copy(BSOMObject* obj, BSOMObject* dest_folder);
int32_t       BSOM_Move(BSOMObject* obj, BSOMObject* dest_folder);
int32_t       BSOM_Delete(BSOMObject* obj, bool send_to_recycle);
int32_t       BSOM_Rename(BSOMObject* obj, const char* new_name);

// Shortcuts & Applications
BSOMObject*   BSOM_CreateShortcut(const char* target_path, const char* shortcut_path);
int32_t       BSOM_ResolveShortcut(BSOMObject* shortcut_obj, char* out_target_path);
int32_t       BSOM_Invoke(BSOMObject* obj);

// Context Menu, Icons & Thumbnails
int32_t       BSOM_GetContextMenu(BSOMObject* obj, BSOMContextMenu* out_menu);
uint32_t      BSOM_GetIcon(BSOMObject* obj);
void*         BSOM_GetThumbnail(BSOMObject* obj, uint32_t w, uint32_t h);

// Favorites & Quick Access
int32_t       BSOM_AddFavorite(BSOMObject* obj);
int32_t       BSOM_RemoveFavorite(BSOMObject* obj);
int32_t       BSOM_PinQuickAccess(BSOMObject* obj);
int32_t       BSOM_UnpinQuickAccess(BSOMObject* obj);
int32_t       BSOM_GetRecent(BSOMObject*** out_items, uint32_t* out_count);

// Search & Locking
int32_t       BSOM_Search(const char* pattern, BSOMObject*** out_results, uint32_t* out_count);
int32_t       BSOM_Lock(BSOMObject* obj, uint32_t lock_type);
int32_t       BSOM_Unlock(BSOMObject* obj);

// Diagnostics
void          BSOM_GetDiagnostics(BSOM_Diagnostics* out_diag);

#endif // BSOM_API_H
```

---

## 5. 75-Test Production Certification Suite

The certification module [`bsom_certification_tests.c`](file:///D:/Signatures_OS/kernel/bsom/tests/bsom_certification_tests.c) validates:

1. `✓ BSOM Subsystem Initialization`
2. `✓ Create File Object`
3. `✓ Create Folder Object`
4. `✓ Create Drive Object`
5. `✓ Create USB Object`
6. `✓ Create Shortcut Object`
7. `✓ Create Application Object`
8. `✓ Create Image Object`
9. `✓ Create Document Object`
10. `✓ Create Audio Object`
11. `✓ Create Video Object`
12. `✓ Create Virtual Folder Object`
13. `✓ Create Network Object`
14. `✓ Create Search Result Object`
15. `✓ Create Recent Item Object`
16. `✓ Create Favorite Object`
17. `✓ Create Recycle Item Object`
18. `✓ Create Quick Access Object`
19. `✓ Create Clipboard Object`
20. `✓ Create Drag Session Object`
21. `✓ Create Thumbnail Object`
22. `✓ Create Icon Object`
23. `✓ Create Property Object`
24. `✓ Create Permission Object`
25. `✓ Create Transaction Object`
26. `✓ Create Context Menu Object`
27. `✓ Create AI Workspace Object`
28. `✓ Handle Allocation & Resolution`
29. `✓ Handle Release`
30. `✓ Object Reference Counting (Retain)`
31. `✓ Object Reference Counting (Release)`
32. `✓ Property Set Query`
33. `✓ Property Get Query`
34. `✓ Show Properties Dialog`
35. `✓ Get Directory Children Objects`
36. `✓ Enumerate Objects Callback`
37. `✓ Copy Object Operation`
38. `✓ Move Object Operation`
39. `✓ Delete Object Operation`
40. `✓ Rename Object Operation`
41. `✓ Create Shortcut Object`
42. `✓ Resolve Shortcut Target`
43. `✓ Application Object Invoke`
44. `✓ Get Context Menu Commands`
45. `✓ Get Icon Object Handle`
46. `✓ Get Thumbnail Bitmap Object`
47. `✓ Add Favorite Object`
48. `✓ Remove Favorite Object`
49. `✓ Pin Quick Access Object`
50. `✓ Unpin Quick Access Object`
51. `✓ Get Recent Object Stream`
52. `✓ Object Search Router`
53. `✓ Shared Object Lock`
54. `✓ Exclusive Object Lock`
55. `✓ Object Unlock`
56. `✓ Registry PID Owner Mapping`
57. `✓ Lifetime Manager Garbage Collection`
58. `✓ Shared Object Cache O(1) Insertion`
59. `✓ Shared Object Cache O(1) Lookup`
60. `✓ Shared Object Cache LRU Eviction`
61. `✓ Multi-Window BSOM Handle Isolation`
62. `✓ Multi-Runtime Session Integration`
63. `✓ Diagnostics Active Object Metric`
64. `✓ Diagnostics Active Handle Metric`
65. `✓ Diagnostics Latency Metric`
66. `✓ Diagnostics Memory Metric`
67. `✓ Race Condition Validation`
68. `✓ Deadlock Avoidance`
69. `✓ Memory Leak Verification`
70. `✓ Explorer Integration`
71. `✓ Desktop Integration`
72. `✓ Terminal Integration`
73. `✓ Browser Integration`
74. `✓ AI Engine Integration`
75. `✓ Stress Test (1,000,000 Operations)`
