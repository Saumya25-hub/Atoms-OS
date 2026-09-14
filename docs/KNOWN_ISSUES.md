# ATOMS OS — Known Issues & Active Engineering Debt

This document tracks known bugs, architectural edge cases, and ongoing engineering investigations in ATOMS OS. Issues are recorded with technical precision and forensic honesty.

---

## 1. Issue Index

| Issue ID | Subsystem / Component | Severity | Lifecycle Status | Summary |
| :--- | :--- | :--- | :--- | :--- |
| **R3-001** | Ring 3 Process Lifecycle | **High** | In Progress | Process exit and teardown page unmapping requires full PML4 user-half reclamation. |
| **R3-002** | Dynamic Linker (`.sll`) | **Medium** | Investigating | Relocation records for `.sll` shared libraries require complete GOT/PLT patch table support. |
| **SYS-001**| Syscall Pointer Validation | **High** | In Progress | Syscall handler needs strict user-space address range clamping (`< 0x00007FFFFFFFFFFF`). |
| **UI-001**  | Compositor Damage Rects | **Low** | Workaround | Rapid cursor movement across overlapping translucent windows causes micro-damage overdraw. |
| **NET-001** | TCP Retransmission Queue | **Medium** | Identified | TCP state machine drops out-of-order packets instead of queueing for selective ACK (SACK). |
| **FS-001**  | BOFS Concurrent Write Locks | **Medium** | In Progress | Multiple thread writes to the same inode extent tree require fine-grained spinlock isolation. |
| **BR-001**  | ATRIX Browser Guest WebGL | **Low** | Experimental | Software rasterizer fallback active; hardware GPU pass-through under virtualized guest incomplete. |

---

## 2. Detailed Technical Bug Records

### Issue R3-001: Process Exit & Address Space Reclamation
- **Component**: Userspace Process Subsystem (`kernel/core/process/`)
- **Severity**: High
- **Status**: In Progress
- **Reproduction**: Launch a userspace ELF process from the desktop shell, allow it to execute `SYS_EXIT (0)`, and monitor physical page frame consumption in PMM telemetry.
- **Expected Behavior**: Upon process termination, all user-mode physical pages allocated in PML4 entries 0..255 (lower 128 TB) should be freed back to the PMM bitmap, and the PID should be recycled.
- **Current Behavior**: The process halts cleanly and is removed from the active scheduler runqueue, but the PML4 page table hierarchy remains allocated to prevent dangling VMM references during parent notification.
- **Root Cause Analysis**: The VMM lack of an atomic reference-counted page directory destructor. Lower page directory tables (PDPT, PD, PT) remain pinned in RAM until reboot.
- **Next Step**: Implement a recursive page-table walker (`vmm_destroy_address_space()`) that walks user-space PML4 entries, frees physical backing frames to PMM, and unpins the CR3 root table.

---

### Issue R3-002: Dynamic `.sll` Shared Library GOT/PLT Relocation
- **Component**: Dynamic Linker Subsystem (`kernel/core/loader/`)
- **Severity**: Medium
- **Status**: Investigating
- **Reproduction**: Compile a userspace application linking against an external `.sll` shared library using non-PIC code generation.
- **Expected Behavior**: The dynamic loader parses the `.sll` export table, resolves imported function symbols, and patches the application Global Offset Table (GOT).
- **Current Behavior**: Statically linked ELF binaries execute reliably. Dynamically linked `.sll` binaries with complex position-independent relative relocations (`R_X86_64_RELATIVE`) require host pre-binding.
- **Root Cause Analysis**: The in-kernel dynamic linker currently implements basic absolute symbol resolution (`R_X86_64_64`) but lacks full lazy-binding PLT resolution.
- **Next Step**: Extend the userspace ELF dynamic loader (`userspace/runtime/ld.sll`) to perform full runtime PLT binding and copy-on-write relocation tables.

---

### Issue SYS-001: Fast Syscall User-Space Address Range Validation
- **Component**: Syscall Gateway (`kernel/core/syscall/`)
- **Severity**: High
- **Status**: In Progress
- **Reproduction**: Invoke `SYS_WRITE` or `SYS_MAP_SURFACE` passing a buffer pointer situated in canonical kernel space (`> 0xFFFF800000000000`).
- **Expected Behavior**: The kernel syscall dispatcher validates that all user-supplied pointers reside strictly within user space (`< 0x00007FFFFFFFFFFF`) and returns `-EFAULT` if invalid.
- **Current Behavior**: Memory mapping checks verify page presence via `vmm_check_user_accessible()`, but a systematic bounds check against the kernel boundary is enforced ad-hoc per syscall rather than universally at the dispatcher entry point.
- **Root Cause Analysis**: Fast syscall gateway (`IA32_LSTAR`) handles registers directly in assembly (`dispatcher.asm`) and relies on individual C syscall implementations for parameter validation.
- **Next Step**: Add an inline macro `VALIDATE_USER_PTR(ptr, len)` at the head of `syscall_dispatch()` to unconditionally reject any pointer outside user address boundaries.

---

### Issue UI-001: Compositor Cursor Trail Micro-Damage Overdraw
- **Component**: BOS Composition Manager (`kernel/display/bcm/`)
- **Severity**: Low
- **Status**: Workaround Active
- **Reproduction**: Move the hardware USB mouse rapidly across a translucent window with active background blur enabled at 2560x1600 resolution.
- **Expected Behavior**: The mouse cursor is composited as an independent hardware plane (or cached dirty rect) without redrawing the underlying window background.
- **Current Behavior**: Under extreme mouse velocities, dirty rectangle bounding box expansion occasionally invalidates a larger area than necessary, causing a momentary frame drop from 60 FPS to 54 FPS.
- **Root Cause Analysis**: The software cursor dirty bounding box merges adjacent damage rectangles when the cursor crosses a window border, causing the compositor to re-blit the background wallpaper.
- **Next Step**: Separate the cursor rendering plane into a dedicated double-buffered cursor overlay that blits directly to the backbuffer without invalidating window client rectangles.

---

### Issue NET-001: TCP Out-of-Order Packet Handling
- **Component**: Network Stack (`kernel/net/tcp/`)
- **Severity**: Medium
- **Status**: Identified
- **Reproduction**: Transmit large HTTP/TLS payloads over a lossy virtual network interface with packet reordering enabled in QEMU.
- **Expected Behavior**: The TCP receiver queues out-of-order packets in a segment reassembly buffer and delivers contiguous bytes to the socket stream.
- **Current Behavior**: Packets with sequence numbers greater than `rcv_nxt` are discarded, forcing the remote peer to wait for retransmission timeout (RTO).
- **Root Cause Analysis**: The lightweight TCP state machine was implemented with strict in-order sliding window processing to minimize kernel heap fragmentation.
- **Next Step**: Implement a fixed-size priority queue (16-segment capacity) in `tcp_socket_t` to hold out-of-order segments awaiting hole-fill packets.
