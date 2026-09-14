# ATOMS OS — FILE MANAGER & VFS ARCHITECTURE SPECIFICATION

**Document Version**: 1.0  
**Status**: 📐 **ARCHITECTURE SPECIFICATION (Phase 1 Approved Reference)**  
**Author**: ATOMS OS Architecture Team  

---

## 1. Architectural Principles & Layering

The File Manager is an authoritative OS user experience component built strictly on top of clean, modular layers. It does not touch filesystem internals or hardware directly.

```text
┌────────────────────────────────────────────────────────────────────────┐
│                      ATOMS FILE MANAGER UI LAYER                       │
│  BWE Window, Navigation Toolbar, Address Bar, Sidebar, Canvas Grid     │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   FILE MANAGER SERVICE & MODEL LAYER                   │
│  Navigation History, Selection Model, Clipboard State, Path Formatter   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   ATOMS VFS / FILESYSTEM SERVICE API                   │
│  vfs_open, vfs_read, vfs_write, vfs_readdir, vfs_mkdir, vfs_delete...  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       CORE VFS MOUNT MANAGER                           │
│  vfs_mount_table, Boundary-Aware Prefix Matcher, -EBUSY Lock Checker   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                     ┌──────────────┴──────────────┐
                     ▼                             ▼
        ┌─────────────────────────┐   ┌─────────────────────────┐
        │   NTFS DRIVER (ntfs.c)  │   │  FAT32 DRIVER (fat32.c) │
        │  MFT B-Tree, Extent Map │   │  BPB, Cluster Chains    │
        └────────────┬────────────┘   └────────────┬────────────┘
                     │                             │
                     └──────────────┬──────────────┘
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                  BLOCK DEVICE & STORAGE SUBSYSTEM                      │
│     disk_manager, MBR Partitions, ATA/AHCI, xHCI USB Mass Storage      │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. UNIX / POSIX Reference Model vs. ATOMS Architecture

ATOMS OS uses mature UNIX/POSIX filesystem concepts purely as **semantic references** for familiarity and correctness. ATOMS does **not** copy UNIX internals, does **not** rename itself as POSIX, and preserves its own microkernel-inspired design.

| Operation / Concept | UNIX / POSIX Semantic Reference | ATOMS Native Implementation | Distinct Architectural Difference in ATOMS |
| :--- | :--- | :--- | :--- |
| **Path Separator** | Forward slash (`/`) | Forward slash (`/`) | ATOMS uses unified forward slash paths for all storage, eliminating DOS `\` escaping issues while supporting volume identifiers. |
| **Directory Enumeration** | `readdir()` / `getdents()` | `vfs_readdir(path, index, &out_dirent)` | ATOMS uses index-based, stateless directory enumeration, preventing kernel cursor desynchronization across preemptive tasks. |
| **File Opening** | `open(path, flags, mode)` | `vfs_open(path)` | Returns integer file descriptor (`fd >= 3`). Global FD table with isolated per-file `VFS_Node` instances. |
| **File Deletion** | `unlink(path)` / `rmdir(path)` | `vfs_delete(path)` | Unified single call for both files and empty directories, delegating type verification to the underlying driver. |
| **File Metadata** | `stat(path, &buf)` | `vfs_dirent_t` / `vfs_stat()` | Basic metadata (`size`, `is_directory`) returned directly in dirent during enumeration to eliminate $O(N)$ round-trip stat storms. |
| **Mount Points** | VFS unified root mount tree | `VFS_Mount mount_table[32]` | Mounts are tracked via an intrusive linked list with boundary-aware prefix matching and strict `-EBUSY` unmount guards. |
| **Volume Visibility** | `/mnt`, `/media`, `/Volumes` | `virtual://ThisPC` & `/volumes/...` | Virtual root view dynamically queries active block devices and filesystems. No hardcoded drive letters. |

---

## 3. ATOMS Native Filesystem Tree Model

ATOMS OS defines an explicit, authoritative tree model. The File Manager must never fabricate fake directories.

```text
ATOMS Root (/)
├── System/                  (Kernel, drivers, core OS libraries)
├── Apps/                    (Installed applications and binaries)
├── Users/                   (User profiles and workspaces)
│   └── Admin/
│       ├── Desktop/         (Desktop surface shortcuts and files)
│       ├── Documents/       (User document workspace)
│       ├── Downloads/       (Downloaded media and packages)
│       └── Pictures/        (User images and wallpapers)
└── Volumes/                 (Dynamic mount points for partitions)
    ├── SystemDrive/         (Mounted root volume / boot partition)
    ├── NTFS_Data/           (Auto-detected physical NTFS partition)
    └── USB_Storage/         (Hotplugged FAT32/NTFS USB mass storage disk)
```

### Path Resolution Invariants:
1. **Absolute Paths**: Always begin with `/`. Prefix matching determines the responsible `VFS_Mount`.
2. **Virtual Root (`virtual://ThisPC`)**: A presentation-layer abstraction that queries the mount table and presents all currently mounted volumes as visual cards.
3. **Volume Identity**: Derived dynamically from partition labels, filesystem types, and drive indices (e.g. `NTFS Volume (disk0p1)`). Drive letters like `C:` are only optional display hints, never hardcoded paths.

---

## 4. Dynamic Mounted Driver Visibility & Lifecycle

The File Manager discovers and renders volumes dynamically from the VFS mount table.

### Dynamic Discovery Flow:
```text
1. User opens File Manager / clicks "This PC"
                    ↓
2. File Manager queries vfs_get_mount_count()
                    ↓
3. Iterates vfs_get_mount_at_index(i)
                    ↓
4. For each active mount:
   - Reads mount_path (e.g. "/", "/volumes/ntfs0")
   - Reads fs_driver->name (e.g. "ntfs", "fat32")
   - Reads block_device->name (e.g. "disk0p1")
                    ↓
5. File Manager renders real volume cards dynamically
```

### Unmount & EBUSY Safety Invariant:
1. When a volume is unmounted (`vfs_unmount_fs`), VFS checks all active file descriptors in `g_fd_table`.
2. If any file descriptor is open under the target volume, `vfs_unmount_fs` rejects with `-16` (`-EBUSY`).
3. If the File Manager currently has that volume open, the File Manager receives an invalidation event, navigates safely back to `virtual://ThisPC`, and the volume card disappears from view.

---

## 5. File Manager MVP Design (Phase 1)

### Visual Layout:
```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│ 🗁 File Explorer - /Volumes/NTFS_Data                                   [-] [□] [X]    │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ [ < ] [ > ] [ ^ Up ] [ ↻ Refresh ]  Path: [ /Volumes/NTFS_Data/Documents          ]   │
├───────────────────┬────────────────────────────────────────────────────────────────────┤
│ QUICK ACCESS      │ Name                  Type            Size         Modified        │
│ ───────────────── │ ────────────────────────────────────────────────────────────────── │
│ 🖴 This PC        │ 📁 WorkProjects       Folder          --           --              │
│ 📁 Desktop        │ 📁 Personal           Folder          --           --              │
│ 📁 Documents      │ 🗎 design_spec.txt    Text Document   14.2 KB      --              │
│ 📁 Downloads      │ 🗎 kernel_log.log     Log File        128.5 KB     --              │
│ 📁 Pictures       │ 🖼️ wallpaper.bmp      Bitmap Image    16.3 MB      --              │
│                   │                                                                    │
│ MOUNTED VOLUMES   │                                                                    │
│ ───────────────── │                                                                    │
│ 🖴 System (/)     │                                                                    │
│ 🖴 NTFS Data      │                                                                    │
│ 🖴 USB Drive      │                                                                    │
├───────────────────┴────────────────────────────────────────────────────────────────────┤
│ 5 items | 1 item selected                                                              │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

### MVP Operations Supported in Phase 1:
1. **Directory Navigation**: Single click to select, double click to open folders.
2. **Back / Forward History**: Full navigation history stack (`history[64]`).
3. **Up Navigation**: Navigates to parent directory (`/a/b/c` ➔ `/a/b`).
4. **Refresh**: Re-scans active directory from the filesystem driver via `vfs_readdir`.
5. **Folder Creation**: `+ New Folder` button invokes `vfs_mkdir(path)`.
6. **File Creation**: Creates 0-byte document via `vfs_create(path)`.
7. **Item Renaming**: Modal rename dialog invokes `vfs_rename(old_path, new_name)`.
8. **Item Deletion**: Deletes file or directory via `vfs_delete(path)`.
9. **File Launching**: Double-clicking text/log files opens them in `notes_app`. Double-clicking media files launches media player.
10. **Volume Detection**: Displays all real, active mounted volumes dynamically in "This PC" and the sidebar.

### Deferred Features (Strictly Excluded from Phase 1):
- Background search indexing
- Real-time thumbnail generation for arbitrary video/image formats
- Trash / Recycle Bin recovery database
- Drag-and-drop copy/paste queues
- File permissions / ACL editors
- Archive extraction (ZIP/TAR)
- Network share browsing (SMB/NFS)

---

## 6. Safety, Boundary Protection & Security Contracts

1. **Path Traversal Containment**:
   - Every path must be validated before passing to VFS. Double dots (`..`) that escape the volume boundary are rejected.
   - Total path length capped at `256` bytes (`BDE_PATH_MAX`).
   - File name component length capped at `64` bytes.
2. **Pointer & Buffer Validation**:
   - In the syscall layer, all user-provided buffers must pass `syscall_validate_user_ptr` or `syscall_validate_user_ptr_writable`.
   - String arguments must pass `syscall_validate_user_string`.
3. **No Kernel Panics on Filesystem Error**:
   - Missing files, read-only media, full disks, corrupt sectors, and unmounted volumes return standardized negative error codes (`-1` to `-16`).
   - Kernel panic handlers are strictly prohibited in the filesystem path.
4. **Memory Allocation Bounds**:
   - Directory enumeration buffers are strictly bounded. `readdir` enumerates one entry at a time or in small fixed batches to prevent heap exhaustion.
