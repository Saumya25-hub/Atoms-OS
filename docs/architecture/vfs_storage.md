# VFS & Storage Architecture

## Purpose
The Virtual File System (VFS) and Storage layer provide a clean abstraction over physical storage devices, allowing the OS to access files uniformly while optimizing for speed.

## Responsibilities
- **VFS:** Abstract file operations (open, read, write, close, seek).
- **Storage Drivers:** Communicate directly with hardware (ATA, AHCI, NVMe).
- **Fast Lookup Integration:** Provide hooks for the Horse Engine to quickly resolve file paths without slow directory traversals.

## Architecture
- **Modular File Systems:** FAT32 and NTFS implementations register with the VFS.
  - FAT32 Driver: `kernel/vfs/vfs_legacy/fs/fat32/`
  - NTFS Driver: `kernel/vfs/vfs_legacy/fs/ntfs/`
    - [NTFS Phase 1 Architecture & Certification](file:///d:/Signatures_OS/docs/architecture/ntfs_phase1_volume_foundation.md)
    - [NTFS Phase 2 MFT Core Engine Architecture & Certification](file:///d:/Signatures_OS/docs/architecture/ntfs_phase2_mft_core.md)
    - [NTFS Phase 3 Attribute Engine Architecture & Certification](file:///d:/Signatures_OS/docs/architecture/ntfs_phase3_attribute_engine.md)
    - [NTFS Phase 4 File Read Engine Architecture & Certification](file:///d:/Signatures_OS/docs/architecture/ntfs_phase4_file_read_engine.md)
    - [NTFS Phase 5 Directory & Index Engine Architecture & Certification](file:///d:/Signatures_OS/docs/architecture/ntfs_phase5_directory_index_engine.md)
    - [NTFS Phase 6 Production VFS Driver + NTFS Mount Integration](file:///d:/Signatures_OS/docs/architecture/ntfs_phase6_vfs_integration.md)
    - [NTFS Phase 7 Production Performance, Real-Media Validation & Advanced Read Engine](file:///d:/Signatures_OS/docs/architecture/ntfs_phase7_performance_real_media.md)
    - [NTFS Phase 7R Real Windows NTFS Media Certification & Level 5 Sign-Off](file:///d:/Signatures_OS/docs/architecture/ntfs_phase7r_real_media_validation.md)
- **Mount Points:** Support for logical drives or mount points.
- **Asynchronous I/O:** (Where possible) to prevent blocking the kernel during slow disk reads.

## Flow
1. Application requests to read `/system/app.exe`.
2. Request hits the VFS layer.
3. VFS checks with Horse Engine to see if the path is cached or optimized.
4. VFS routes the request to the appropriate File System driver.
5. File System driver reads from the Storage Driver block device.

## Future Expansion
- Implement an in-memory file system (tmpfs) for high-speed temporary storage.
- Expand Horse Engine integration for intelligent caching of frequently read blocks.

## TODO
- [ ] Audit the existing VFS layer in `kernel/vfs` and `kernel/storage`.
- [ ] Define the Fast File Lookup API for the Horse Engine.
