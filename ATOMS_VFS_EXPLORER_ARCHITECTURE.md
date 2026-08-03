# ATOMS OS — VFS & Explorer Architecture Specification

**Document:** `ATOMS_VFS_EXPLORER_ARCHITECTURE.md`  
**System:** ATOMS OS / SignaturesOS  
**Architecture:** Freestanding x86_64 Monolithic Kernel + Real VFS Engine  

---

## 1. Overview & Core Philosophy

ATOMS OS File Explorer has been upgraded from a static view mockup to a **Real VFS-Driven System Engine**.

Rather than copying Microsoft Windows literal drive letters (`C:\`), ATOMS OS defines its own custom drive hierarchy centered around **`A:\` (ATOMS Drive)**, giving the operating system a unique identity while maintaining enterprise-grade Virtual File System (VFS) functionality.

---

## 2. ATOMS OS Drive & System Namespace Mapping (`A:\`)

The primary boot partition and VFS root (`/`) are presented in the GUI and Shell as **`A:\` (ATOMS Drive)**.

| ATOMS Path | VFS Path | Purpose / Description |
|---|---|---|
| `A:\` | `/` | Primary ATOMS OS System Drive Root |
| `A:\ATOMS` | `/ATOMS` | Core Kernel binaries, boot modules, and micro-kernel components |
| `A:\SYS32` | `/SYS32` | System runtime libraries, SLL binaries, and syscall entrypoints |
| `A:\SURFACE` | `/SURFACE` | Rook Engine graphics assets, BWE themes, wallpapers, and fonts |
| `A:\APPS` | `/APPS` | Installed application executables (`.BOSX`, `.ELF`, `DOOM`) |
| `A:\USERS` | `/USERS` | User profile space, including Desktop (`/desktop`) and Documents (`/DOCS`) |
| `A:\NTFS` | `/ntfs` | Genuine Windows XP formatted read-only NTFS storage volume |

---

## 3. Real VFS Engine Integration (`explorer.c` & `explorer_ui.c`)

### 3.1 Directory Enumeration (`vfs_readdir`)
File Explorer no longer displays static dummy arrays. `Explorer_Refresh()` calls kernel [`vfs_readdir()`](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs.h#L57) in a loop:

```c
vfs_dirent_t dirent;
int index = 0;
while (vfs_readdir(current_path, index, &dirent) == 0) {
    // Populate real file or directory item into ExplorerViewItem grid
    index++;
}
```

### 3.2 Dynamic Folder Creation (`vfs_mkdir`)
When the user clicks **"+ New v"** or chooses **New Folder**, Explorer invokes [`vfs_mkdir()`](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs.h#L58):
1. Construct `new_path = current_path + "/New Folder"`
2. Execute `vfs_mkdir(new_path)`
3. Re-trigger `Explorer_Refresh(ctx)` to instantly render the new directory.

### 3.3 File & Directory Deletion (`vfs_delete`)
When an item is selected and the **"Delete"** button is clicked, Explorer invokes [`vfs_delete()`](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs.h#L61):
1. Retrieve target path `obj->path` from selected item
2. Execute `vfs_delete(path)`
3. Remove object handle from BSOM and refresh viewport.

---

## 4. Context Menu & Desktop Synchronization Roadmap

### 4.1 Right-Click Context Menu Engine (`BWE_EVENT_MOUSE_RIGHT_CLICK`)
* **Desktop Right-Click**:
  * 📁 *New Folder* -> `vfs_mkdir("/desktop/New Folder")`
  * 📄 *New File* -> `vfs_create("/desktop/New Document.txt")`
  * 🔄 *Refresh Desktop* -> [`desktop_refresh_background()`](file:///D:/Signatures_OS/kernel/shell/desktop_shell/desktop_shell.c#L76)
  * 🎨 *Display Settings* -> `horse_launch(APP_ID_SETTINGS)`
* **Explorer Grid Right-Click**:
  * 📂 *Open* -> Navigate into directory or execute application
  * ✏️ *Rename* -> Inline text box -> `vfs_rename(old_path, new_name)`
  * 🗑️ *Delete* -> `vfs_delete(path)`

---

## 5. Verification & Testing Matrix

- **VFS Read Directory (`vfs_readdir`)**: Verified against `/`, `/ntfs`, `/SYS32`
- **Folder Creation (`vfs_mkdir`)**: Verified creating `/New Folder` in VFS
- **Folder Deletion (`vfs_delete`)**: Verified deleting nodes from VFS tree
- **ATOMS Drive Identity (`A:\`)**: Fully integrated into Address Bar & Sidebar
