# Chapter 17: BOFS Filesystem & Transaction Engine

**BOFS (BOS Filesystem)** is the native operating system filesystem of ATOMS OS, engineered for transaction safety, wear resilience, and high-speed inode lookups.

## 1. Disk Layout
```
+----------------+----------------+----------------+----------------+
| Superblock     | WAL Journal    | Inode Bitmap   | Inode Table    |
| (Block 0..1)   | (Blocks 2..64) | & Block Bitmap | Extent Trees   |
+----------------+----------------+----------------+----------------+
| Data Blocks                                                       |
| (Contiguous 4096-byte extent clusters)                            |
+-------------------------------------------------------------------+
```

## 2. Key Architecture Features
- **Write-Ahead Logging (WAL)**: All metadata mutations (inode creation, directory link, block allocation) are written to the WAL journal before being committed to primary structures, preventing disk corruption on power loss.
- **Extent-Based Allocation**: File data is addressed as contiguous extents `(logical_block, physical_block, count)` rather than individual block pointers, drastically reducing metadata overhead for large media files.
- **VFS Integration**: Implements standard POSIX VFS operations: `mount()`, `unmount()`, `open()`, `read()`, `write()`, `readdir()`, `stat()`.
