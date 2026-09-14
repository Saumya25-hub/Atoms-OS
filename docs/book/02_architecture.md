# Chapter 02: 7-Tier System Architecture

ATOMS OS is organized into seven strictly decoupled architectural tiers. Each tier provides services to higher layers while isolating hardware complexity.

```
+-------------------------------------------------------------------------+
| Tier 7: Applications (DOOM, Media Player, Dashboards, ATRIX Browser)    |
+-------------------------------------------------------------------------+
| Tier 6: Userspace Runtime & Desktop Shell (Rook Login, BCM Windowing)  |
+-------------------------------------------------------------------------+
| Tier 5: Syscall Gateway (IA32_LSTAR, Fast SYSRET64, User Validation)    |
+-------------------------------------------------------------------------+
| Tier 4: VFS & Filesystem Engines (BOFS, FAT32 ESP, NTFS Read-Only)     |
+-------------------------------------------------------------------------+
| Tier 3: Core Drivers (xHCI USB 3.0, NVMe Gen4, AHCI SATA, RTL8168, HDA)|
+-------------------------------------------------------------------------+
| Tier 2: BOS Kernel Core (PMM Bitmap, VMM Paging, Heap, GDT/TSS, IDT)   |
+-------------------------------------------------------------------------+
| Tier 1: Firmware & Boot Interface (UEFI 2.x, GOP Framebuffer, Trampoline)|
+-------------------------------------------------------------------------+
```

## Tier Descriptions
1. **Tier 1 — Firmware & Boot**: Pure 64-bit UEFI application (`BOOTX64.EFI`) that queries firmware services, acquires the physical memory map, configures GOP video mode, loads the kernel payload, and transitions execution to Long Mode.
2. **Tier 2 — Kernel Core**: Manages fundamental CPU structures (GDT, Dual TSS, IDT), 32 GB Bitmap Physical Memory Manager, 4-level PML4 Virtual Memory Manager, dynamic heap allocator, and Local APIC timer.
3. **Tier 3 — Core Drivers**: Interacts directly with PCI/PCIe hardware: xHCI USB 3.0 host controller with HID keyboard/mouse, NVMe Gen4 SSD driver, AHCI SATA driver, Realtek RTL8168 Gigabit Ethernet, and Intel HDA audio codec.
4. **Tier 4 — VFS & Filesystems**: Provides a unified hierarchical namespace supporting the native transactional **BOFS** filesystem with Write-Ahead Logging (WAL), FAT32 EFI partitions, and read-only NTFS exploration.
5. **Tier 5 — Syscall Gateway**: Implements hardware fast-syscalls via MSR `IA32_LSTAR`, enabling microsecond-scale user/kernel privilege transitions with pointer bounds enforcement.
6. **Tier 6 — Desktop Shell & Compositor**: Double-buffered composition engine (**BCM**) running at 60 FPS, Rook login supervisor, wallpaper service, and window management.
7. **Tier 7 — Applications**: Native user-facing applications running in Ring 3 userspace, including the DOOM engine port, hardware video/audio player, diagnostic dashboards, and experimental ATRIX browser components.
