# BOS Shell V3 & BO-FileHUB Architecture

> **"Kernel owns storage. BO-FileHUB owns object logic. Shell manipulates objects. FileHub visualizes objects."**

## Overview
This document serves as the architectural foundation for **Signatures OS (BOS)** storage and developer environments. It establishes a unified, future-proof approach to managing data that will power the command-line Shell today and the graphical Desktop tomorrow.

**BO-FileHUB is a storage abstraction layer.** 
It provides a consistent object model independent of the underlying storage backend (whether that is FAT32 today, or BOSFS / NTFS in the future).

---

## BO-FileHUB Architecture

**BO-FileHUB** is the definitive Storage API for BOS. Rather than each application (Shell, Editor, Installer) independently interacting with raw syscalls, they utilize BO-FileHUB as a standardized abstraction layer.

### The Object Model
Everything managed by BO-FileHUB is treated as a `BOObject`. This guarantees that operations remain generic whether handling a standard text file or a future Desktop Shortcut.

```c
typedef enum {
    BO_OBJECT_FILE,
    BO_OBJECT_FOLDER,
    BO_OBJECT_SHORTCUT, // For Desktop integration
    BO_OBJECT_APP       // Application bundles
} BOObjectType;

typedef struct {
    BOObjectType type;
    char name[64];
    uint64_t size;
    // Metadata (Timestamps, Parent IDs) will be added here
} BOObject;
```

### The 6 Core Engines
BO-FileHUB provides operations through six dedicated engines in user-space (`libbos/bofilehub.c`), which internally wrap actual Kernel VFS syscalls:

1. **Navigation Engine**: Manages state for navigating hierarchies (`open`, `back`, `root`, `pwd`).
2. **Folder Engine**: Manages directory creation and visualization (`create`, `delete`, `tree`).
3. **File Engine**: Manages content (`create`, `open`, `save`, `delete`, `rename`, `copy`, `move`).
4. **Metadata Engine**: Handles attributes (`info`, `timestamps`, `tags`).
5. **Search Engine**: Queries objects (`search`, `filter`).
6. **Size Engine**: Calculates storage limits (`file size`, `folder size`, `disk stats`).

## Technical Specifications (V1)

**Architecture Limits (BO-FileHUB Level):**
* **Object Name:** 64 characters
* **Maximum Path:** 256 characters
* **Total Objects:** Unlimited (subject to storage capacity)
* **Directory Entries:** Dynamic
* **Storage Backend:** Pluggable (Any filesystem driver can attach underneath)
* **Memory Usage:** < 128 bytes per object in RAM
* **Maximum File Size:** Unlimited (Architecture Level)

**Current Backend Limits (FAT32 Driver Level):**
* **Maximum File Size:** 4 GB (FAT32 limitation)
* **Virtual Disk Scaling:** 64MB (Current) → 256MB → 512MB → 1GB

---

## Shell V3 Architecture

Shell V3 is the **BOS Developer Environment**. It is built dynamically upon a **Command Dispatcher** rather than a hardcoded monolithic structure. 

### Command Families
Commands are strictly organized into families. This modularity ensures a clean Help system and predictable categorization:

#### Navigation
- `pwd` - Print working directory
- `open` - Traverse into a `BO_OBJECT_FOLDER`
- `back` - Traverse to parent directory
- `root` - Traverse to `/`

#### Folder
- `mkdir` - Create a `BO_OBJECT_FOLDER`
- `rmdir` - Delete a `BO_OBJECT_FOLDER`
- `tree` - Visualize folder hierarchy

#### File
- `new` - Create a `BO_OBJECT_FILE`
- `cat` - View file content
- `rename` - Rename an object
- `copy` - Duplicate an object
- `move` - Transfer an object
- `delete` - Unlink an object

#### System
- `help` - Show modular command list
- `ver` - OS version information
- `about` - System context

#### Debug
- `heapinfo` - Dump memory metrics
- `heapwalk` - Analyze heap blocks
- `heapvalidate` - Verify allocator health
- `dmesg` - Print kernel log buffer
- `taskinfo` - Display scheduler task data

---

## Integration Pipeline (Future Roadmap)

1. **Phase A & B**: Implement BO-FileHUB Spec and modular Shell V3.
2. **Phase C**: Enable genuine FAT32 writing inside the Kernel VFS so objects persist.
3. **Phase D & E**: Deploy the text-based FileHub logic and BOS Editor.
4. **Phase F**: The **Graphical Transition**. Because the backend relies on the `BOObject` model, migrating to a GUI Framebuffer File Explorer involves *zero logic rewrites*. The GUI will simply draw an icon array instead of text, passing `open` requests to the exact same Navigation Engine.
