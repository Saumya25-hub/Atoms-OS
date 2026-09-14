# ATOMS OS — FILESYSTEM CAPABILITY MATRIX

**Document Version**: 1.0  
**Phase**: Phase 1A Forensic Audit  
**Target Hardware**: ASUS B750M-K (Intel Core i3-14100F, Native UEFI GOP at 2560×1600)  

---

## 1. Comprehensive Subsystem Capability Matrix

| Filesystem Capability | Core VFS (`vfs.c`) | NTFS Driver (`ntfs.c`) | FAT32 Driver (`fat32.c`) | DummyFS (`dummy_fs.c`) | Syscall ABI (`syscall.h`) | Ring 3 User Accessible? | File Manager UI Accessible? | Notes & Implementation Details |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **Mount Filesystem** | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🟡 Partial | Handled in kernel via `vfs_mount_fs()`. Max 32 mounts. Not exposed as syscall. |
| **Unmount Filesystem** | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | `-EBUSY` check protects active open FDs. Tested 1000 cycles with 0 leaks. |
| **Detect Filesystem** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | 🔴 NO | `vfs_detect_fs()` inspects sector 0 for `"NTFS    "` OEM signature and FAT32 BPB signatures. |
| **Enumerate Mounts** | 🟡 Internal | N/A | N/A | N/A | 🔴 NO | 🔴 NO | 🔴 NO | `mount_table` is `static list_t` in `vfs.c`. No public iterator or count API exists yet. |
| **Open File** | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES (`SYS_OPEN`) | 🟢 YES | 🟢 YES | Global `g_fd_table[32]` with FDs 3–31. Syscall validates path pointer. |
| **Read File** | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES (`SYS_READ`) | 🟢 YES | 🟢 YES | Supports sequential `vfs_read()` and position-independent `vfs_pread()`. |
| **Write File** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🟢 YES (`SYS_WRITE_FILE`) | 🟢 YES | 🟡 Partial | Writes through driver callback. Extents checked against partition bounds. |
| **Seek File** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🟢 YES (`SYS_SEEK`) | 🟢 YES | 🟡 Partial | Supports `SEEK_SET`, `SEEK_CUR`, and `SEEK_END`. |
| **Close File** | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES | 🟢 YES (`SYS_CLOSE`) | 🟢 YES | 🟢 YES | Frees VFS node and marks FD available. |
| **List Directory (`readdir`)** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | 🟢 Ring 0 | `vfs_readdir(path, index, &dirent)` traverses directory entries. Missing in syscall ABI. |
| **Create Directory (`mkdir`)** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | 🟢 Ring 0 | Creates directory node. UI uses this for "+ New Folder". Missing in syscall ABI. |
| **Create File (`create`)** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | 🟢 Ring 0 | Creates 0-byte file node. Missing in syscall ABI. |
| **Rename File / Dir** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | 🟢 Ring 0 | Renames file/directory. Driver updates parent index. Missing in syscall ABI. |
| **Delete File / Dir** | 🟢 YES | 🟢 YES | 🟢 YES | 🔴 NO | 🔴 NO | 🔴 NO | 🟢 Ring 0 | Unlinks file or empty directory. Missing in syscall ABI. |
| **File Metadata (`stat`)** | 🔴 NO | 🟡 Via dirent | 🟡 Via dirent | 🔴 NO | 🔴 NO | 🔴 NO | 🟡 Partial | VFS returns `size` and `is_directory` in `vfs_dirent_t`. No dedicated `vfs_stat()` API. |
| **Path Traversal Guard** | 🟡 Prefix only | 🟢 Relative strip | 🟢 Path walk | 🔴 NO | 🟢 String len check | 🟢 Safe | 🟡 Partial | Mount prefix striping works; full lexical canonicalization (`..` collapse) needed. |
| **Volume Label / Identity** | 🔴 NO | 🟡 In BPB | 🟡 In BPB | 🔴 NO | 🔴 NO | 🔴 NO | 🔴 Hardcoded | Volume names currently hardcoded as `SYSTEM DRIVE (A:)` or `NTFS VOLUME (C:)`. |

---

## 2. Key Takeaways from the Capability Matrix

1. **Kernel Driver Depth**:
   - Both NTFS and FAT32 are fully capable of reading, writing, seeking, creating, listing, renaming, and deleting files and directories.
   - The filesystem driver layer is **NOT** the bottleneck.
2. **The Missing Bridge**:
   - In Ring 0 (Kernel/Shell context), all 10 VFS functions exist and work.
   - For Ring 3 user applications, directory listing (`readdir`), file deletion, creation, renaming, and mount enumeration are completely blocked due to lack of syscall IDs.
3. **Mount Visibility Defect**:
   - The File Manager currently cannot discover mounted disks dynamically because `mount_table` is private to `vfs.c` without an enumeration function (`vfs_get_mount_count`, `vfs_get_mount_by_index`).
   - Adding a lightweight, read-only mount enumeration API will allow the File Manager to show real mounted partitions dynamically (e.g. `System (/)`, `NTFS Data (/volumes/ntfs0)`, `USB Drive (/volumes/usb0)`) with zero hardcoding.
