# VFS & Storage Architecture

## Purpose
The Virtual File System (VFS) and Storage layer provide a clean abstraction over physical storage devices, allowing the OS to access files uniformly while optimizing for speed.

## Responsibilities
- **VFS:** Abstract file operations (open, read, write, close, seek).
- **Storage Drivers:** Communicate directly with hardware (ATA, AHCI, NVMe).
- **Fast Lookup Integration:** Provide hooks for the Horse Engine to quickly resolve file paths without slow directory traversals.

## Architecture
- **Modular File Systems:** FAT32 and custom FS implementations must register with the VFS.
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
