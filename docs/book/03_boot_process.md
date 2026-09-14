# Chapter 03: Boot Process & Execution Trampoline

The boot path of ATOMS OS transitions the computer from cold silicon into an interactive multi-window desktop shell.

## 1. Boot Stages
```
[ UEFI Firmware (POST) ]
          │
          ▼
[ EFI/BOOT/BOOTX64.EFI ]
  • Locate GOP (2560x1600 / 1920x1080 32-bpp)
  • Load KERNEL.BIN into physical RAM (0x100000)
  • Acquire Memory Map via GetMemoryMap()
  • Terminate Firmware via ExitBootServices()
          │
          ▼
[ kernel_entry.asm (Trampoline) ]
  • Setup 256 KB Boot Stack (boot_stack_top)
  • Pass boot_info_t pointer in RDI (System V AMD64 ABI)
  • Reload CS:DS segment registers
          │
          ▼
[ kernel_main() in kernel/kernel.c ]
  • Initialize COM1 Serial Telemetry (115200 baud)
  • Render ABDE Diagnostic Framebuffer Table
  • Initialize PMM (Physical Page Allocator)
  • Initialize VMM (PML4 Virtual Memory)
  • Load GDT & 64-bit TSS (Ring 0 / Ring 3 descriptors)
  • Load IDT & mask legacy 8259 PIC
  • Enumerate PCI bus & initialize xHCI USB 3.0
  • Initialize VFS & Mount BOFS / FAT32
  • Setup IA32_LSTAR Syscall Gateway
          │
          ▼
[ Rook Login Supervisor & Desktop Shell ]
  • Blit Desktop Wallpaper & Background Surface
  • Spawn Rook Interactive Login Loop
  • Enable USB Mouse & Keyboard Interrupt Polling
  • Enter 60 FPS Compositor Dispatch Loop
```

## 2. Kernel ABI Parameters
The UEFI bootloader builds a contiguous `boot_info_t` structure containing:
- `framebuffer_base`: 64-bit physical address of GOP VRAM.
- `framebuffer_size`: Total byte size of video memory.
- `screen_width`: Horizontal resolution (e.g. 2560 pixels).
- `screen_height`: Vertical resolution (e.g. 1600 pixels).
- `pixels_per_scanline`: Pitch in pixels per scanline.
- `memory_map`: Pointer to UEFI memory map descriptor array.
- `memory_map_size`: Byte length of memory descriptor table.
- `descriptor_size`: Stride per memory descriptor.
