# ATOMS OS — FILE MANAGER ROADMAP & PHASE 1 IMPLEMENTATION PLAN

**Document Version**: 1.0  
**Status**: 📋 **IMPLEMENTATION PLAN (Awaiting Approval to Execute)**  
**Target Milestone**: Phase 1 — Filesystem Audit & Real File Manager MVP  

---

## 1. High-Level Engineering Roadmap

```text
┌────────────────────────────────────────────────────────────────────────┐
│  PHASE 1: Filesystem Audit & Real File Manager MVP (CURRENT)          │
│  - Initialize VFS & storage stack cleanly on normal boot               │
│  - Implement dynamic mount enumeration (remove hardcoded A:, C:, E:)   │
│  - Eliminate synthesized fake folders (SYS32, APPS, etc.)              │
│  - Wire File Manager to real NTFS/FAT32 driver directory listings      │
│  - Support: Browse, Back, Forward, Up, Refresh, New Folder, Rename,    │
│    Delete, and Open text/log files via notes_app                       │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│  PHASE 2: File Copy, Move & Streaming Operations                       │
│  - Streaming file copy buffer with non-blocking progress UI            │
│  - Cross-volume and intra-volume file move                             │
│  - Conflict resolution dialog (Overwrite, Skip, Keep Both)             │
│  - Clipboard cut/copy/paste pipeline integration                       │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│  PHASE 3: Properties, Search & File Associations                       │
│  - File / Folder properties inspection (size, clusters, permissions)   │
│  - Substring file search within active directory subtree               │
│  - Authoritative file association registry (ext -> application launch) │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│  PHASE 4: Thumbnails, Previews & UX Polish                             │
│  - Asynchronous image thumbnail generator (BMP, PNG)                   │
│  - Column sorting (Name, Size, Type, Date)                             │
│  - Drag-and-drop visual selection marquee                              │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│  PHASE 5: Advanced Drivers & Hotplug Infrastructure                    │
│  - xHCI USB Mass Storage dynamic hotplug auto-mount / auto-unmount    │
│  - GPT partition table parser                                          │
│  - Ext2/Ext4 read-only driver introduction                             │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Phase 1 Detailed Execution Steps

### Step 1: Storage & VFS Boot Initialization
- **Target File**: [`kernel/kernel.c`](file:///d:/Signatures_OS/kernel/kernel.c)
- **Action**:
  - In `kernel.c`, right before `Desktop_Shell_Initialize()`, invoke:
    ```c
    extern void block_device_init(void);
    extern void vfs_init(void);
    extern void dummyfs_init(void);
    extern void fat32_init(void);
    extern void ntfs_init(void);
    extern void disk_manager_init(void);
    ```
  - Mount an initial in-memory root filesystem or boot volume at `/` so root path lookups never fail silently.

### Step 2: Dynamic Mount Enumeration API
- **Target Files**:
  - [`kernel/vfs/vfs_legacy/include/vfs.h`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs.h)
  - [`kernel/vfs/vfs_legacy/src/vfs.c`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c)
- **Action**:
  - Implement two lightweight, thread-safe public queries:
    ```c
    uint32_t vfs_get_mount_count(void);
    bool     vfs_get_mount_info(uint32_t index, char* out_path, uint32_t max_path, char* out_fs, uint32_t max_fs, char* out_dev, uint32_t max_dev);
    ```
  - Allows caller to discover all active mount points dynamically without exposing private linked-list nodes.

### Step 3: De-Hardcode File Manager (Remove Fake Drives & Fake Folders)
- **Target Files**:
  - [`kernel/shell/apps/explorer.c`](file:///d:/Signatures_OS/kernel/shell/apps/explorer.c)
  - [`kernel/shell/apps/explorer_sidebar.c`](file:///d:/Signatures_OS/kernel/shell/apps/explorer_sidebar.c)
- **Action**:
  - In `Explorer_Refresh()`:
    - Replace the hardcoded `"SYSTEM DRIVE (A:)"` and `"NTFS VOLUME (C:)"` with dynamic enumeration of `vfs_get_mount_info()`.
    - Delete the code block synthesizing fake `SYS32`, `APPS`, and `SURFACE` directories at lines 159–200.
    - Directory items will populate strictly and purely from `vfs_readdir(path, index, &dirent)`.
  - In `explorer_sidebar.c`:
    - Populate the sidebar dynamically from real mounts plus standard user directories (`This PC`, `System Root`, `Documents`).

### Step 4: Verify Navigation & File Operations
- **Action**:
  - Verify folder navigation (`Explorer_Navigate`), `Go Back`, `Go Forward`, `Go Up`, and `Refresh`.
  - Test `+ New Folder` (`vfs_mkdir`).
  - Test `Rename` (`vfs_rename`).
  - Test `Delete` (`vfs_delete`).
  - Test double-clicking `.txt` / `.log` files to open directly in `notes_app`.

---

## 3. Test & Validation Plan

### Automated Regression & Sanity Tests:
1. **Compilation**: Clean build with zero warnings and zero linker errors.
2. **Mount Enumeration**: `vfs_get_mount_count()` returns accurate counts across mount/unmount cycles.
3. **EBUSY Protection**: Unmounting a busy mount point while File Manager has files open returns `-16` (`-EBUSY`).
4. **QEMU UEFI Validation**:
   - Boot into desktop cleanly.
   - Launch File Manager.
   - Verify "This PC" displays only real mounted volumes.
   - Browse into real directories; confirm no fake `SYS32` folders appear.
   - Create a folder, rename it, delete it.
5. **Physical ASUS B750M-K Validation**:
   - Boot on bare metal.
   - Verify real physical storage partitions (NTFS/FAT32) enumerate and open cleanly.
