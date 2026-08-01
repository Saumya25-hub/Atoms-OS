# 🏛️ ATOMS OS EXPLORER REWRITE — REAL INTEGRATION (Phase 9)

> **Phase:** 9 — Explorer Rewrite (Real Integration)  
> **Goal:** Convert Explorer into a PURE VIEW LAYER  
> **Authority:** BSOM V1.0 (Universal Object Manager)  

---

## 1. Architecture Before vs After

### BEFORE (Legacy)
```text
Explorer
├── Own current_path
├── Own history[]
├── Own selected[]
├── Own clipboard
├── Own drag_state
├── Own cache
├── Direct VFS calls (vfs_readdir, vfs_stat, etc.)
├── Manual path parsing
├── Manual icon color resolution
├── Manual file type detection
└── Manual filesystem logic in explorer_cache.c
```

### AFTER (Phase 9)
```text
Explorer (Pure Renderer)
├── Window Handle
├── Viewport & Scroll Position
├── Mouse & Keyboard State
├── Focused Widget
├── Theme & Animations
├── Drawing Commands
└── NOTHING ELSE

ALL operations → BSOM API
```

---

## 2. Complete Purge List

### Files Rewritten
| File | Action |
| :--- | :--- |
| `explorer.h` | Rewritten: ExplorerContext becomes pure view state |
| `explorer.c` | Rewritten: All logic delegated to BSOM |
| `explorer_view.h` | Rewritten: Clean renderer declarations |
| `explorer_view.c` | Rewritten: Pure renderer, zero filesystem access |
| `explorer_cache.c` | **GUTTED**: All VFS calls removed, delegates to BSOM |
| `explorer_cache.h` | **GUTTED**: Legacy types removed, uses BSOMObject |

### Legacy Code Removed
- `vfs_readdir()` calls in explorer_cache.c
- `vfs_dirent_t` usage
- `ExplorerItem` struct (replaced by BSOMObject)
- `ExplorerDirCache` struct (replaced by BSOM cache)
- `ExplorerIconCache` struct (replaced by BSOM_GetIcon)
- `explorer_get_file_type()` (replaced by BSOM class type)
- `explorer_get_item_color()` (replaced by BSOM icon ID)
- Manual file type detection logic
- Manual icon color mapping
- Direct `BDeRuntime*` and `BDeDirEntry*` access

---

## 3. New Explorer Architecture

```text
Mouse / Keyboard / Touch
         │
         ▼
Explorer UI (Renderer Only)
         │
    BSOM_* API calls
         │
         ▼
BSOM (Universal Object Layer)
         │
         ▼
BRT (Runtime Manager)
         │
         ▼
BFS (Filesystem Services)
         │
         ▼
BO-TREE (Core Engine)
         │
         ▼
VFS → NTFS / FAT32 / BOSFS
```

---

## 4. Explorer API (New)

```c
int  Explorer_Create(uint32_t* out_win);
void Explorer_Destroy(uint32_t win_id);
void Explorer_Update(ExplorerContext* ctx);
void Explorer_Render(BWE_Window* win);
void Explorer_HandleMouse(ExplorerContext* ctx, const BWE_Event* event);
void Explorer_HandleKeyboard(ExplorerContext* ctx, const BWE_Event* event);
void Explorer_Resize(ExplorerContext* ctx, int32_t w, int32_t h);
void Explorer_Navigate(ExplorerContext* ctx, const char* path);
```

---

## 5. BSOM Operation Mapping

| Explorer Action | BSOM API Call |
| :--- | :--- |
| Open folder | `BSOM_OpenObject()` |
| Open file | `BSOM_Invoke()` |
| Back | `BSOM_OpenObject(history_back)` |
| Forward | `BSOM_OpenObject(history_fwd)` |
| Refresh | `BSOM_GetChildren()` |
| Rename | `BSOM_Rename()` |
| Delete | `BSOM_Delete()` |
| Copy | `BSOM_Copy()` |
| Move | `BSOM_Move()` |
| Properties | `BSOM_ShowProperties()` |
| Children | `BSOM_GetChildren()` |
| Search | `BSOM_Search()` |
| Thumbnail | `BSOM_GetThumbnail()` |
| Icon | `BSOM_GetIcon()` |
| Context Menu | `BSOM_GetContextMenu()` |
| Favorites | `BSOM_AddFavorite()` |
| Recent | `BSOM_GetRecent()` |

---

## 6. Performance Targets

| Metric | Target |
| :--- | :--- |
| 100,000 files visible | ✅ |
| 60 FPS rendering | ✅ |
| Zero UI freeze | ✅ |
| Zero memory leak | ✅ |
| Zero VFS calls from Explorer | ✅ |
| Zero duplicated state | ✅ |
| Zero race conditions | ✅ |

---

## 7. Certification Suite (30 Tests)

See [`explorer_rewrite_certification.c`](file:///D:/Signatures_OS/kernel/shell/apps/tests/explorer_rewrite_certification.c)
