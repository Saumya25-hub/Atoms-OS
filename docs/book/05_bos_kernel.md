# Chapter 05: BOS Kernel Architecture

The **BOS Kernel** is a monolithic 64-bit Long Mode operating system kernel designed for high-throughput graphics compositing, deterministic interrupt response, and low-latency syscall dispatch.

## 1. Source Layout
The kernel implementation resides in [`kernel/`](file:///D:/Signatures_OS/kernel):
- `kernel/kernel.c`: Primary kernel initialization, subsystem orchestrator, and diagnostics.
- `kernel/core/`: PMM, VMM, Kernel Heap, Tasking, Syscall dispatcher, and ACPI.
- `kernel/debug/`: Autonomous BOS Diagnostic Engine (ABDE), klog, telemetry collectors.
- `kernel/drivers/`: Device drivers (xHCI, NVMe, AHCI, Display, Audio, Network).
- `kernel/display/`: DGL graphics pipeline, BSPE frame pacer, and BCM compositor.
- `kernel/vfs/`: VFS mount manager, BOFS driver, FAT32 driver, NTFS parser.
- `kernel/shell/`: Rook login supervisor, desktop shell, and windowing runtime.

## 2. Real-Time Telemetry & ABDE
Diagnostic transparency is central to the kernel design. The ABDE (Autonomous BOS Diagnostic Engine) provides:
- Dual-channel telemetry: simultaneous high-speed 115200-baud COM1 UART serial output and direct-to-VRAM bitmap font blitting.
- Heartbeat frame counter: hardware TSC/PIT-calibrated spinner rotating at 60 Hz to prove CPU liveness during long I/O operations.
- Forensic crash dumps: immediate visual register snapshot (RAX, RBX, RCX, RDX, RSI, RDI, RSP, RBP, R8-R15, CR0, CR2, CR3, CR4, RIP, RFLAGS).
